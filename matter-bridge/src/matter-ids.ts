/**
 * Derivation of the Matter BasicInformation `uniqueId` and `serialNumber` from the
 * sidecar's per-install unique id.
 *
 * Matter's BasicInformation cluster bounds both attributes at 32 characters, and the
 * spec (Core §11.1.6, "UniqueID ... shall not be identical to the SerialNumber
 * attribute") requires the two to differ. Our `config.uniqueId` is
 * `aqualink-bridge-<uuid>` (52 chars), which overflows both bounds -- supplying it
 * verbatim makes the BasicInformationServer behavior fail its constraint check at node
 * init, surfacing as `AggregateError: Behaviors have errors`.
 *
 * These helpers produce two short, distinct, deterministic values from the raw id so the
 * node's identity stays stable across restarts. Pure (no matter.js state) so they can be
 * unit-tested in isolation.
 */

/** Matter BasicInformation maxLength for serialNumber / uniqueId (Core §11.1.6). */
const MATTER_ID_MAX_LEN = 32;

/** Strip everything but ASCII alphanumerics (the safe subset for these identifiers). */
function alphanumeric(raw: string): string {
  return raw.replace(/[^a-zA-Z0-9]/g, "");
}

/**
 * A <=32-char BasicInformation.uniqueId derived from the raw per-install id.
 *
 * Takes the trailing alphanumerics, which preserves the random UUID tail (the entropy)
 * while dropping the fixed `aqualink-bridge` prefix when the id overflows 32 chars.
 */
export function matterUniqueId(rawUniqueId: string): string {
  return alphanumeric(rawUniqueId).slice(-MATTER_ID_MAX_LEN);
}

/**
 * A <=32-char BasicInformation.serialNumber that is guaranteed to differ from
 * {@link matterUniqueId} for the same input.
 *
 * Prefixing with `sn` and taking the *leading* 32 chars yields a value with a different
 * prefix and different content from the uniqueId's trailing-32 derivation, satisfying the
 * "must not be identical" rule even for short inputs.
 */
export function matterSerialNumber(rawUniqueId: string): string {
  return `sn${alphanumeric(rawUniqueId)}`.slice(0, MATTER_ID_MAX_LEN);
}
