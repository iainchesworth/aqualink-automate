/**
 * Sidecar configuration, resolved from environment variables.
 *
 * Every value has a sensible default so the bridge runs out-of-the-box inside the
 * Docker image alongside the C++ app (which listens on 127.0.0.1:80 by default).
 *
 * The Matter commissioning passcode/discriminator/uniqueId are the exception: they are
 * NOT hardcoded. When unset, they are generated cryptographically-randomly per install
 * and persisted under MATTER_STORAGE_PATH (see ./credentials.ts) -- see the security note
 * there for why a fixed default would be a remote-control vulnerability.
 */

import {
  EXAMPLE_PASSCODE,
  resolveCredentials,
} from "./credentials.js";

function env(name: string, fallback: string): string {
  const v = process.env[name];
  return v === undefined || v === "" ? fallback : v;
}

function envInt(name: string, fallback: number): number {
  const v = process.env[name];
  if (v === undefined || v === "") return fallback;
  const n = Number.parseInt(v, 10);
  return Number.isFinite(n) ? n : fallback;
}

/** Parse an optional integer env var; returns undefined when unset/empty/non-numeric. */
function envIntOpt(name: string): number | undefined {
  const v = process.env[name];
  if (v === undefined || v === "") return undefined;
  const n = Number.parseInt(v, 10);
  return Number.isFinite(n) ? n : undefined;
}

/** An optional string env var; returns undefined when unset/empty. */
function envOpt(name: string): string | undefined {
  const v = process.env[name];
  return v === undefined || v === "" ? undefined : v;
}

export interface BridgeConfig {
  /** Base URL of the aqualink-automate HTTP API (REST + WS share host:port). */
  readonly apiUrl: string;
  /** Optional bearer token if the C++ API has --api-auth-token set. */
  readonly apiToken: string | undefined;

  /** Matter commissioning passcode (8 digits, must not be a trivial value). */
  readonly passcode: number;
  /** Matter discriminator (12-bit, 0-4095). */
  readonly discriminator: number;

  /** UDP port the Matter node listens on (5540 is the matter.js default). */
  readonly matterPort: number;

  /** Directory where matter.js persists fabric/commissioning state (a Docker volume). */
  readonly storagePath: string;

  /** Port for the small status HTTP server that exposes the pairing QR to the C++ UI. */
  readonly statusPort: number;

  /** Human-facing identifiers surfaced to the commissioner. */
  readonly vendorId: number;
  readonly productId: number;
  readonly vendorName: string;
  readonly productName: string;
  /** Stable serial number; persisted so the same node identity survives restarts. */
  readonly uniqueId: string;
}

export function loadConfig(): BridgeConfig {
  const storagePath = env("MATTER_STORAGE_PATH", "/data/matter");

  // Resolve commissioning credentials: explicit env override > persisted value >
  // freshly-generated cryptographically-random value (persisted for next boot). There is
  // deliberately NO fixed default here -- a shared default passcode would let any LAN
  // device commission the bridge with a publicly-known value.
  const creds = resolveCredentials(storagePath, {
    passcode: envIntOpt("MATTER_PASSCODE"),
    discriminator: envIntOpt("MATTER_DISCRIMINATOR"),
    uniqueId: envOpt("MATTER_UNIQUE_ID"),
  });

  // Defence in depth: refuse to start on the well-known example passcode even if an
  // operator explicitly forces it via MATTER_PASSCODE. The example value is published in
  // every matter.js sample, so starting with it would expose pool control to the LAN.
  if (creds.passcode === EXAMPLE_PASSCODE) {
    throw new Error(
      `Refusing to start: MATTER_PASSCODE is the well-known example value ${EXAMPLE_PASSCODE}. ` +
        "Unset MATTER_PASSCODE to auto-generate a secure per-install passcode, or set a non-trivial value.",
    );
  }

  return {
    apiUrl: env("AQUALINK_API_URL", "http://127.0.0.1:80").replace(/\/+$/, ""),
    apiToken: process.env.AQUALINK_API_TOKEN || undefined,

    passcode: creds.passcode,
    discriminator: creds.discriminator,

    matterPort: envInt("MATTER_PORT", 5540),
    storagePath,
    statusPort: envInt("MATTER_BRIDGE_STATUS_PORT", 8099),

    vendorId: envInt("MATTER_VENDOR_ID", 0xfff1), // 0xFFF1-0xFFF4 are the test vendor IDs
    productId: envInt("MATTER_PRODUCT_ID", 0x8000),
    vendorName: env("MATTER_VENDOR_NAME", "AqualinkAutomate"),
    productName: env("MATTER_PRODUCT_NAME", "Aqualink Pool Bridge"),
    uniqueId: creds.uniqueId,
  };
}
