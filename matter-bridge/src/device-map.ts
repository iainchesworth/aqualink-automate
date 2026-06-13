/**
 * Pure mapping from the aqualink-automate equipment JSON onto Matter device kinds.
 *
 * This module is intentionally free of any matter.js import so the mapping rules can
 * be unit-tested in isolation; bridge.ts translates a MatterKind into the concrete
 * matter.js device type + behaviours.
 *
 * The device JSON (GET /api/equipment/devices, json_data_hub.cpp) groups devices into
 * three buckets - auxillaries[], heaters[], pumps[] - and does NOT carry the raw
 * AuxillaryTypes enum on each device. We therefore derive the Matter kind from the
 * bucket plus a couple of field heuristics (generating_percentage => chlorinator).
 */

export enum MatterKind {
  /** Generic on/off equipment (aux, cleaner, spillover, sprinkler, pump, light-v1). */
  OnOffPlugInUnit = "OnOffPlugInUnit",
  /** Heater: Matter Thermostat (heat) driven by the per-body setpoint. */
  Thermostat = "Thermostat",
  /** Salt chlorinator: On/Off + Level Control (generating percentage; boost = 101). */
  OnOffLevelChlorinator = "OnOffLevelChlorinator",
  /** Read-only pool/spa/air temperature. */
  TemperatureSensor = "TemperatureSensor",
}

export type DeviceBucket = "auxillary" | "heater" | "pump";

/** One device as the bridge will expose it (after flattening + mapping). */
export interface BridgedDevice {
  readonly uuid: string;
  readonly label: string;
  readonly bucket: DeviceBucket;
  readonly kind: MatterKind;
  readonly on: boolean;
  /** Chlorinator generating percentage (0-100, or 101 for boost), when present. */
  readonly level?: number;
  /** "Pool" | "Spa" | "Shared" | ... when known. */
  readonly bodyOfWater?: string;
}

/** A read-only temperature sensor the bridge exposes from the temperatures section. */
export interface TemperatureSensorSpec {
  readonly id: string; // "pool" | "spa" | "air"
  readonly label: string;
  readonly celsius: number | null;
}

/** The raw shape of one device object in the equipment JSON (only fields we use). */
export interface RawDevice {
  id?: string;
  label?: string;
  state?: string;
  generating_percentage?: number;
  body_of_water?: string;
}

export interface RawDevices {
  auxillaries?: RawDevice[];
  heaters?: RawDevice[];
  pumps?: RawDevice[];
}

/**
 * Whether a device status string means "on". Status strings are C++ enum names
 * (magic_enum), e.g. AuxillaryStatuses::On -> "On", PumpStatuses::Running -> "Running",
 * HeaterStatuses -> "Off"/"Heating"/"Enabled". Treat the active states as on.
 */
export function stateIsOn(state: string | undefined): boolean {
  if (!state) return false;
  return /^(on|running|heating|enabled|generating)$/i.test(state.trim());
}

/** Derive the Matter kind for a device given its bucket and raw fields. */
export function mapDeviceKind(bucket: DeviceBucket, raw: RawDevice): MatterKind {
  if (bucket === "heater") {
    return MatterKind.Thermostat;
  }
  if (bucket === "pump") {
    return MatterKind.OnOffPlugInUnit;
  }
  // auxillary bucket: a chlorinator advertises a generating percentage.
  if (typeof raw.generating_percentage === "number") {
    return MatterKind.OnOffLevelChlorinator;
  }
  // Everything else (aux/cleaner/spillover/sprinkler/light) ships as On/Off in v1.
  return MatterKind.OnOffPlugInUnit;
}

function flattenBucket(bucket: DeviceBucket, devices: RawDevice[] | undefined): BridgedDevice[] {
  if (!Array.isArray(devices)) return [];
  return devices
    .filter((d) => typeof d.id === "string" && d.id.length > 0)
    .map((d) => {
      const kind = mapDeviceKind(bucket, d);
      const device: BridgedDevice = {
        uuid: d.id as string,
        label: (d.label && d.label.trim()) || "Aqualink Device",
        bucket,
        kind,
        on: stateIsOn(d.state),
        ...(typeof d.generating_percentage === "number" ? { level: d.generating_percentage } : {}),
        ...(d.body_of_water ? { bodyOfWater: d.body_of_water } : {}),
      };
      return device;
    });
}

/** Flatten the three device buckets into one mapped list. */
export function parseDevices(devices: RawDevices | undefined): BridgedDevice[] {
  if (!devices) return [];
  return [
    ...flattenBucket("auxillary", devices.auxillaries),
    ...flattenBucket("heater", devices.heaters),
    ...flattenBucket("pump", devices.pumps),
  ];
}

/**
 * Build the temperature-sensor specs from the /api/equipment temperatures section.
 * Only sensors that have ever reported (non-null) are exposed; HomeKit cannot show a
 * sensor that never has a value.
 */
export function parseTemperatureSensors(temperatures: Record<string, unknown> | undefined): TemperatureSensorSpec[] {
  if (!temperatures) return [];
  const specs: Array<{ id: string; label: string }> = [
    { id: "pool", label: "Pool Temperature" },
    { id: "spa", label: "Spa Temperature" },
    { id: "air", label: "Air Temperature" },
  ];
  const out: TemperatureSensorSpec[] = [];
  for (const s of specs) {
    const v = temperatures[s.id];
    if (typeof v === "number" && Number.isFinite(v)) {
      out.push({ id: s.id, label: s.label, celsius: v });
    }
  }
  return out;
}
