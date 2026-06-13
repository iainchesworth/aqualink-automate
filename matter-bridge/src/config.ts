/**
 * Sidecar configuration, resolved from environment variables.
 *
 * Every value has a sensible default so the bridge runs out-of-the-box inside the
 * Docker image alongside the C++ app (which listens on 127.0.0.1:80 by default).
 */

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
  return {
    apiUrl: env("AQUALINK_API_URL", "http://127.0.0.1:80").replace(/\/+$/, ""),
    apiToken: process.env.AQUALINK_API_TOKEN || undefined,

    // Passcode/discriminator default to fixed values for first boot; the C++ side
    // generates and persists random ones and passes them in via the environment so
    // each install is unique (see options_matter_options).
    passcode: envInt("MATTER_PASSCODE", 20202021),
    discriminator: envInt("MATTER_DISCRIMINATOR", 3840),

    matterPort: envInt("MATTER_PORT", 5540),
    storagePath: env("MATTER_STORAGE_PATH", "/data/matter"),
    statusPort: envInt("MATTER_BRIDGE_STATUS_PORT", 8099),

    vendorId: envInt("MATTER_VENDOR_ID", 0xfff1), // 0xFFF1-0xFFF4 are the test vendor IDs
    productId: envInt("MATTER_PRODUCT_ID", 0x8000),
    vendorName: env("MATTER_VENDOR_NAME", "AqualinkAutomate"),
    productName: env("MATTER_PRODUCT_NAME", "Aqualink Pool Bridge"),
    uniqueId: env("MATTER_UNIQUE_ID", "aqualink-bridge-0001"),
  };
}
