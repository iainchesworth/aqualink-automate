/**
 * Self-generated, persisted Matter commissioning credentials.
 *
 * SECURITY: The Matter passcode + discriminator are the only secrets gating who may
 * commission this bridge. Matter commissioning runs over the LAN (UDP 5540 + mDNS) and
 * the container uses host networking, so ANY device on the network that knows the
 * passcode can pair and actuate pool equipment. Shipping the well-known matter.js/CHIP
 * example passcode (20202021) and discriminator (3840) would let anyone on the LAN
 * commission the bridge with publicly documented values -- a remote-control takeover.
 *
 * Therefore, when the operator has not explicitly supplied MATTER_PASSCODE /
 * MATTER_DISCRIMINATOR / MATTER_UNIQUE_ID, we generate cryptographically-random,
 * per-install values on first boot and PERSIST them under MATTER_STORAGE_PATH (a mounted
 * Docker volume) so the same identity + pairing secret survive restarts. We use Node's
 * crypto primitives (crypto.randomInt / crypto.randomUUID), never Math.random.
 */

import { randomInt, randomUUID } from "node:crypto";
import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";

/**
 * The matter.js/CHIP example passcode. It is published in countless examples, so it
 * provides no security at all -- treat it as a forbidden sentinel and never start with it.
 */
export const EXAMPLE_PASSCODE = 20202021;

/**
 * Passcodes the Matter spec (section 5.1.7.1) declares invalid: the all-same-digit
 * repdigits plus two trivial sequences. matter.js rejects these, and they are easily
 * guessed, so we must never generate (or accept) one. The example passcode is added so a
 * generated value can never collide with the value we explicitly refuse to start on.
 */
const FORBIDDEN_PASSCODES = new Set<number>([
  0, 11111111, 22222222, 33333333, 44444444, 55555555, 66666666, 77777777, 88888888,
  99999999, 12345678, 87654321, EXAMPLE_PASSCODE,
]);

/** The Matter passcode is a 27-bit value in the inclusive range 1..0x5F5E0FE (99999998). */
const PASSCODE_MIN = 1;
const PASSCODE_MAX = 99999998;

/** The discriminator is a 12-bit value (0..4095). */
const DISCRIMINATOR_MAX = 4096; // exclusive upper bound for randomInt

/** The persisted credential record, versioned so the format can evolve safely. */
interface StoredCredentials {
  version: 1;
  passcode: number;
  discriminator: number;
  uniqueId: string;
}

/** True if a passcode is in-range and not one of the spec-forbidden / example values. */
export function isValidPasscode(passcode: number): boolean {
  return (
    Number.isInteger(passcode) &&
    passcode >= PASSCODE_MIN &&
    passcode <= PASSCODE_MAX &&
    !FORBIDDEN_PASSCODES.has(passcode)
  );
}

/** Draw a cryptographically-random passcode, retrying until it clears the forbidden set. */
export function generatePasscode(): number {
  // randomInt(min, max) is uniform over [min, max); the forbidden values are a tiny
  // fraction of the ~100M-value space, so this almost always succeeds on the first draw.
  for (;;) {
    const candidate = randomInt(PASSCODE_MIN, PASSCODE_MAX + 1);
    if (isValidPasscode(candidate)) return candidate;
  }
}

/** Draw a cryptographically-random 12-bit discriminator (0..4095). */
export function generateDiscriminator(): number {
  return randomInt(0, DISCRIMINATOR_MAX);
}

/** A per-install unique node id; random so two installs never share a node identity. */
export function generateUniqueId(): string {
  return `aqualink-bridge-${randomUUID()}`;
}

/** Where the generated credentials live inside the persisted storage volume. */
function credentialsPath(storagePath: string): string {
  return join(storagePath, "bridge-credentials.json");
}

function readStored(path: string): StoredCredentials | undefined {
  if (!existsSync(path)) return undefined;
  try {
    const parsed = JSON.parse(readFileSync(path, "utf8")) as Partial<StoredCredentials>;
    if (
      parsed.version === 1 &&
      typeof parsed.passcode === "number" &&
      typeof parsed.discriminator === "number" &&
      typeof parsed.uniqueId === "string" &&
      isValidPasscode(parsed.passcode) &&
      parsed.discriminator >= 0 &&
      parsed.discriminator < DISCRIMINATOR_MAX &&
      parsed.uniqueId.length > 0
    ) {
      return parsed as StoredCredentials;
    }
  } catch {
    // A corrupt/unreadable file is treated as "no credentials"; we regenerate below so a
    // bad file can never wedge startup (a re-pair is the worst case).
  }
  return undefined;
}

function writeStored(path: string, creds: StoredCredentials): void {
  mkdirSync(dirname(path), { recursive: true });
  // 0o600: the pairing passcode is a secret; keep it readable only by the owning user.
  writeFileSync(path, `${JSON.stringify(creds, null, 2)}\n`, { mode: 0o600 });
}

export interface ResolvedCredentials {
  passcode: number;
  discriminator: number;
  uniqueId: string;
}

/**
 * Resolve the commissioning credentials, in priority order:
 *   1. Explicit env override (MATTER_PASSCODE / MATTER_DISCRIMINATOR / MATTER_UNIQUE_ID).
 *   2. Previously-persisted generated values under the storage path.
 *   3. Freshly generated cryptographically-random values, persisted for next boot.
 *
 * Env overrides are merged per-field over the persisted/generated set, so an operator can
 * pin just the discriminator (for example) and still get a generated random passcode.
 *
 * @param storagePath  The directory matter.js persists state to (a mounted volume).
 * @param overrides    Parsed env values; `undefined` means "not supplied".
 */
export function resolveCredentials(
  storagePath: string,
  overrides: {
    passcode: number | undefined;
    discriminator: number | undefined;
    uniqueId: string | undefined;
  },
): ResolvedCredentials {
  const path = credentialsPath(storagePath);
  const stored = readStored(path);

  // Start from the persisted values, falling back to freshly-generated ones for any field
  // not already on disk.
  const base: StoredCredentials = stored ?? {
    version: 1,
    passcode: generatePasscode(),
    discriminator: generateDiscriminator(),
    uniqueId: generateUniqueId(),
  };

  // Layer any explicit env overrides on top.
  const resolved: StoredCredentials = {
    version: 1,
    passcode: overrides.passcode ?? base.passcode,
    discriminator: overrides.discriminator ?? base.discriminator,
    uniqueId: overrides.uniqueId && overrides.uniqueId.length > 0 ? overrides.uniqueId : base.uniqueId,
  };

  // Persist whenever the resolved set differs from what is on disk (first boot, an env
  // override changed a value, or a missing field was just generated). Env-override values
  // are persisted too, so a later restart without the env var keeps the same identity --
  // the least-surprising behaviour for a stable Matter node.
  if (
    !stored ||
    stored.passcode !== resolved.passcode ||
    stored.discriminator !== resolved.discriminator ||
    stored.uniqueId !== resolved.uniqueId
  ) {
    writeStored(path, resolved);
  }

  return {
    passcode: resolved.passcode,
    discriminator: resolved.discriminator,
    uniqueId: resolved.uniqueId,
  };
}
