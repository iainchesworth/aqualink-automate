import { strict as assert } from "node:assert";
import { test } from "node:test";

import { raiseLowerHeatingSetpoint } from "../src/matter-servers.js";

// Setpoints + limits are centi-°C; Amount is in 0.1 °C steps (so 1 step == 10 centi).
const LIMITS = { minCenti: 100, maxCenti: 4000 }; // matches bridge.ts addThermostat (1 °C .. 40 °C)

test("raiseLowerHeatingSetpoint adds a positive amount", () => {
  assert.equal(
    raiseLowerHeatingSetpoint({ currentCenti: 2800, amountSteps: 5, ...LIMITS }),
    2850, // 28.0 °C + 0.5 °C
  );
});

test("raiseLowerHeatingSetpoint subtracts a negative amount", () => {
  assert.equal(
    raiseLowerHeatingSetpoint({ currentCenti: 2800, amountSteps: -10, ...LIMITS }),
    2700, // 28.0 °C - 1.0 °C
  );
});

test("raiseLowerHeatingSetpoint clamps to the max heat limit", () => {
  assert.equal(
    raiseLowerHeatingSetpoint({ currentCenti: 3950, amountSteps: 20, ...LIMITS }),
    4000, // 39.5 + 2.0 = 41.5 -> clamped to 40.0 (not an error per spec)
  );
});

test("raiseLowerHeatingSetpoint clamps to the min heat limit", () => {
  assert.equal(
    raiseLowerHeatingSetpoint({ currentCenti: 150, amountSteps: -20, ...LIMITS }),
    100, // 1.5 - 2.0 = -0.5 -> clamped to 1.0
  );
});

test("raiseLowerHeatingSetpoint is a no-op for amount 0", () => {
  assert.equal(
    raiseLowerHeatingSetpoint({ currentCenti: 2800, amountSteps: 0, ...LIMITS }),
    2800,
  );
});
