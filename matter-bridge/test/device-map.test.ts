import { strict as assert } from "node:assert";
import { test } from "node:test";

import {
  MatterKind,
  stateIsOn,
  mapDeviceKind,
  parseDevices,
  parseTemperatureSensors,
} from "../src/device-map.js";

test("stateIsOn treats active enum names as on", () => {
  for (const s of ["On", "on", "Running", "Heating", "Enabled", "Generating"]) {
    assert.equal(stateIsOn(s), true, `${s} should be on`);
  }
  for (const s of ["Off", "off", "NotRunning", "Unknown", "", undefined]) {
    assert.equal(stateIsOn(s as string | undefined), false, `${s} should be off`);
  }
});

test("mapDeviceKind maps buckets and chlorinator heuristic", () => {
  assert.equal(mapDeviceKind("heater", {}), MatterKind.Thermostat);
  assert.equal(mapDeviceKind("pump", {}), MatterKind.OnOffPlugInUnit);
  assert.equal(mapDeviceKind("auxillary", {}), MatterKind.OnOffPlugInUnit);
  assert.equal(
    mapDeviceKind("auxillary", { generating_percentage: 50 }),
    MatterKind.OnOffLevelChlorinator,
  );
});

test("parseDevices flattens buckets, maps state and skips id-less entries", () => {
  const devices = parseDevices({
    auxillaries: [
      { id: "a1", label: "Spillway", state: "On", body_of_water: "Shared" },
      { id: "a2", label: "AquaPure", state: "Generating", generating_percentage: 75 },
      { label: "no id - dropped", state: "On" }, // skipped: no id
    ],
    heaters: [{ id: "h1", label: "Pool Heater", state: "Heating", body_of_water: "Pool" }],
    pumps: [{ id: "p1", label: "Filter Pump", state: "Running" }],
  });

  assert.equal(devices.length, 4);

  const spillway = devices.find((d) => d.uuid === "a1");
  assert.ok(spillway);
  assert.equal(spillway!.kind, MatterKind.OnOffPlugInUnit);
  assert.equal(spillway!.on, true);
  assert.equal(spillway!.bodyOfWater, "Shared");

  const swg = devices.find((d) => d.uuid === "a2");
  assert.ok(swg);
  assert.equal(swg!.kind, MatterKind.OnOffLevelChlorinator);
  assert.equal(swg!.level, 75);
  assert.equal(swg!.on, true);

  const heater = devices.find((d) => d.uuid === "h1");
  assert.equal(heater!.kind, MatterKind.Thermostat);
  assert.equal(heater!.bodyOfWater, "Pool");

  const pump = devices.find((d) => d.uuid === "p1");
  assert.equal(pump!.kind, MatterKind.OnOffPlugInUnit);
  assert.equal(pump!.on, true);
});

test("parseDevices tolerates missing buckets", () => {
  assert.deepEqual(parseDevices(undefined), []);
  assert.deepEqual(parseDevices({}), []);
});

test("parseTemperatureSensors only emits sensors with finite values", () => {
  const specs = parseTemperatureSensors({ pool: 26.5, spa: null, air: 22 });
  assert.equal(specs.length, 2);
  assert.deepEqual(
    specs.map((s) => s.id),
    ["pool", "air"],
  );
  assert.equal(specs[0]!.celsius, 26.5);
});
