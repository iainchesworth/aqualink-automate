import { strict as assert } from "node:assert";
import { test } from "node:test";
import { mkdtempSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import {
  EXAMPLE_PASSCODE,
  generateDiscriminator,
  generatePasscode,
  isValidPasscode,
  resolveCredentials,
} from "../src/credentials.js";

/** The full spec-invalid / example set the generated passcode must never land on. */
const FORBIDDEN = new Set<number>([
  0, 11111111, 22222222, 33333333, 44444444, 55555555, 66666666, 77777777, 88888888,
  99999999, 12345678, 87654321, EXAMPLE_PASSCODE,
]);

function tempStorage(): string {
  return mkdtempSync(join(tmpdir(), "matter-creds-"));
}

test("generatePasscode never returns a forbidden or example passcode", () => {
  // Many draws: the generator must reject the forbidden set on every iteration.
  for (let i = 0; i < 5000; i++) {
    const p = generatePasscode();
    assert.equal(FORBIDDEN.has(p), false, `passcode ${p} is forbidden`);
    assert.equal(isValidPasscode(p), true, `passcode ${p} should be valid`);
    assert.ok(p >= 1 && p <= 99999998, `passcode ${p} out of Matter range`);
    assert.notEqual(p, EXAMPLE_PASSCODE);
  }
});

test("generateDiscriminator stays within the 12-bit range 0..4095", () => {
  for (let i = 0; i < 5000; i++) {
    const d = generateDiscriminator();
    assert.ok(Number.isInteger(d), `discriminator ${d} not an integer`);
    assert.ok(d >= 0 && d <= 4095, `discriminator ${d} out of range`);
  }
});

test("isValidPasscode rejects the example and spec-invalid values", () => {
  for (const bad of FORBIDDEN) assert.equal(isValidPasscode(bad), false, `${bad} should be invalid`);
  assert.equal(isValidPasscode(0), false);
  assert.equal(isValidPasscode(99999999), false); // above the valid max as well as forbidden
  assert.equal(isValidPasscode(100000000), false); // out of range
  assert.equal(isValidPasscode(-1), false);
});

test("resolveCredentials generates a non-forbidden passcode and a valid discriminator", () => {
  const dir = tempStorage();
  try {
    const creds = resolveCredentials(dir, {
      passcode: undefined,
      discriminator: undefined,
      uniqueId: undefined,
    });
    assert.equal(FORBIDDEN.has(creds.passcode), false);
    assert.notEqual(creds.passcode, EXAMPLE_PASSCODE);
    assert.ok(creds.discriminator >= 0 && creds.discriminator <= 4095);
    assert.ok(creds.uniqueId.length > 0);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("resolveCredentials persists generated values and reuses them on a second load", () => {
  const dir = tempStorage();
  try {
    const first = resolveCredentials(dir, {
      passcode: undefined,
      discriminator: undefined,
      uniqueId: undefined,
    });

    // The credentials file must exist under the storage path after the first resolve.
    const stored = JSON.parse(readFileSync(join(dir, "bridge-credentials.json"), "utf8"));
    assert.equal(stored.passcode, first.passcode);
    assert.equal(stored.discriminator, first.discriminator);
    assert.equal(stored.uniqueId, first.uniqueId);

    // A second load (no env overrides) must return the SAME persisted values, not new ones.
    const second = resolveCredentials(dir, {
      passcode: undefined,
      discriminator: undefined,
      uniqueId: undefined,
    });
    assert.deepEqual(second, first);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test("resolveCredentials honours explicit env overrides and persists them", () => {
  const dir = tempStorage();
  try {
    const creds = resolveCredentials(dir, {
      passcode: 31415926,
      discriminator: 1234,
      uniqueId: "pinned-id",
    });
    assert.equal(creds.passcode, 31415926);
    assert.equal(creds.discriminator, 1234);
    assert.equal(creds.uniqueId, "pinned-id");

    // A restart WITHOUT the env vars keeps the same identity (persisted on first run).
    const reloaded = resolveCredentials(dir, {
      passcode: undefined,
      discriminator: undefined,
      uniqueId: undefined,
    });
    assert.deepEqual(reloaded, creds);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});
