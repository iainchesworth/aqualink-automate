import { strict as assert } from "node:assert";
import { test } from "node:test";

import { matterUniqueId, matterSerialNumber } from "../src/matter-ids.js";

// The real per-install id format: "aqualink-bridge-<uuid>" (52 chars), which overflows
// the BasicInformation 32-char bound and previously crashed node init with
// "AggregateError: Behaviors have errors".
const RAW = "aqualink-bridge-0e9a6762-e8dd-4ad2-af23-0bce896a2a31";

test("derived uniqueId / serialNumber respect the 32-char BasicInformation bound", () => {
  assert.ok(matterUniqueId(RAW).length <= 32, "uniqueId must be <= 32 chars");
  assert.ok(matterSerialNumber(RAW).length <= 32, "serialNumber must be <= 32 chars");
});

test("derived serialNumber differs from uniqueId (spec forbids identical values)", () => {
  assert.notEqual(matterSerialNumber(RAW), matterUniqueId(RAW));
});

test("uniqueId preserves the random UUID tail", () => {
  // The fixed "aqualink-bridge" prefix is dropped; the UUID hex (the entropy) is kept.
  assert.equal(matterUniqueId(RAW), "0e9a6762e8dd4ad2af230bce896a2a31");
});

test("derivation is deterministic and alphanumeric-only", () => {
  assert.equal(matterUniqueId(RAW), matterUniqueId(RAW));
  assert.match(matterUniqueId(RAW), /^[a-zA-Z0-9]*$/);
  assert.match(matterSerialNumber(RAW), /^[a-zA-Z0-9]*$/);
});

test("short custom ids stay distinct and within bounds", () => {
  // An operator-supplied MATTER_UNIQUE_ID that is already short must still yield two
  // distinct, valid values.
  const short = "abc";
  assert.notEqual(matterSerialNumber(short), matterUniqueId(short));
  assert.ok(matterUniqueId(short).length <= 32);
  assert.ok(matterSerialNumber(short).length <= 32);
});
