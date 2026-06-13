/**
 * The Matter bridge: a matter.js ServerNode hosting an Aggregator endpoint with one
 * bridged endpoint per pool device. Cluster command handlers call back into the
 * aqualink-automate command API; live state from /ws/equipment is pushed onto the
 * matching endpoint attributes. Fabric/commissioning state persists to a Docker
 * volume so pairing survives restarts.
 */

import { Endpoint, ServerNode, Environment, VendorId } from "@matter/main";
import { AggregatorEndpoint } from "@matter/main/endpoints";
import {
  OnOffPlugInUnitDevice,
  DimmablePlugInUnitDevice,
  ThermostatDevice,
  TemperatureSensorDevice,
} from "@matter/main/devices";
import { BridgedDeviceBasicInformationServer } from "@matter/main/behaviors/bridged-device-basic-information";
import { ThermostatServer } from "@matter/main/behaviors/thermostat";
import { Thermostat } from "@matter/main/clusters";

import type { BridgeConfig } from "./config.js";
import type { AqualinkClient } from "./aqualink-client.js";
import {
  type BridgedDevice,
  type TemperatureSensorSpec,
  MatterKind,
} from "./device-map.js";

/** Celsius -> Matter centi-degrees (1/100 °C), the wire unit for measured/setpoint. */
const toCenti = (c: number): number => Math.round(c * 100);
const fromCenti = (v: number): number => v / 100;

interface DeviceHandle {
  // Endpoints are heterogeneous (on/off, dimmable, thermostat) so we keep them loosely
  // typed here; the strongly-typed handle in each add* method wires the command events.
  endpoint: Endpoint<any>;
  device: BridgedDevice;
  /** Suppress the command echo while we apply remote (API/WS) state to the endpoint. */
  applyingRemote: boolean;
}

/** matter.js state patches are device-type specific; apply through one loose helper. */
async function setState(endpoint: Endpoint<any>, patch: Record<string, unknown>): Promise<void> {
  await endpoint.set(patch as never);
}

export interface PairingInfo {
  qrPairingCode: string;
  manualPairingCode: string;
}

export class MatterBridge {
  private server?: ServerNode;
  private aggregator?: Endpoint<AggregatorEndpoint>;
  private readonly handles = new Map<string, DeviceHandle>();
  private readonly tempEndpoints = new Map<string, Endpoint<any>>();

  constructor(
    private readonly config: BridgeConfig,
    private readonly client: AqualinkClient,
  ) {}

  /** True once the node is created and serving the commissioning window. */
  get isRunning(): boolean {
    return this.server !== undefined;
  }

  /** The number of fabrics this node is commissioned into. */
  get fabricCount(): number {
    try {
      return this.server?.state.commissioning.fabrics
        ? Object.keys(this.server.state.commissioning.fabrics).length
        : 0;
    } catch {
      return 0;
    }
  }

  get isPaired(): boolean {
    return this.fabricCount > 0;
  }

  /** The QR payload + manual code for the C++ UI to render. */
  pairing(): PairingInfo | undefined {
    if (!this.server) return undefined;
    try {
      const codes = this.server.state.commissioning.pairingCodes;
      return { qrPairingCode: codes.qrPairingCode, manualPairingCode: codes.manualPairingCode };
    } catch {
      return undefined;
    }
  }

  /** Start the Matter node and the Aggregator. */
  async start(): Promise<void> {
    const environment = Environment.default;
    environment.vars.set("storage.path", this.config.storagePath);

    this.server = await ServerNode.create({
      id: this.config.uniqueId,
      environment,
      network: { port: this.config.matterPort },
      commissioning: {
        passcode: this.config.passcode,
        discriminator: this.config.discriminator,
      },
      productDescription: {
        name: this.config.productName,
        deviceType: AggregatorEndpoint.deviceType,
      },
      basicInformation: {
        vendorName: this.config.vendorName,
        vendorId: VendorId(this.config.vendorId),
        productName: this.config.productName,
        productId: this.config.productId,
        // serialNumber must differ from uniqueId per the Matter spec.
        serialNumber: `${this.config.uniqueId}-sn`,
        uniqueId: this.config.uniqueId,
        hardwareVersion: 1,
        hardwareVersionString: "1.0",
        softwareVersion: 1,
        softwareVersionString: this.config.productName,
      },
    });

    this.aggregator = new Endpoint(AggregatorEndpoint, { id: "aggregator" });
    await this.server.add(this.aggregator);

    await this.server.start();
  }

  /** Stop the node (used on shutdown). */
  async stop(): Promise<void> {
    await this.server?.close();
    this.server = undefined;
  }

  /** A short, stable, endpoint-id-safe slug derived from a device uuid. */
  private static endpointId(prefix: string, uuid: string): string {
    return `${prefix}-${uuid.replace(/[^a-zA-Z0-9]/g, "").slice(0, 24)}`;
  }

  private bridgedBasicInfo(device: BridgedDevice) {
    return {
      nodeLabel: device.label.slice(0, 32),
      productName: device.label.slice(0, 32),
      productLabel: device.label.slice(0, 32),
      serialNumber: device.uuid.slice(0, 32),
      reachable: true,
    };
  }

  /** Add (or replace) all devices from a freshly-parsed list. */
  async syncDevices(devices: BridgedDevice[]): Promise<void> {
    for (const device of devices) {
      if (this.handles.has(device.uuid)) {
        await this.applyDeviceState(device);
      } else {
        await this.addDevice(device);
      }
    }
  }

  private async addDevice(device: BridgedDevice): Promise<void> {
    if (!this.aggregator) throw new Error("Bridge not started");

    switch (device.kind) {
      case MatterKind.OnOffPlugInUnit:
        await this.addOnOff(device);
        break;
      case MatterKind.OnOffLevelChlorinator:
        await this.addChlorinator(device);
        break;
      case MatterKind.Thermostat:
        await this.addThermostat(device);
        break;
      default:
        await this.addOnOff(device);
        break;
    }
  }

  private async addOnOff(device: BridgedDevice): Promise<void> {
    const endpoint = new Endpoint(OnOffPlugInUnitDevice.with(BridgedDeviceBasicInformationServer), {
      id: MatterBridge.endpointId("aux", device.uuid),
      bridgedDeviceBasicInformation: this.bridgedBasicInfo(device),
      onOff: { onOff: device.on },
    });
    await this.aggregator!.add(endpoint);

    const handle: DeviceHandle = { endpoint, device, applyingRemote: false };
    this.handles.set(device.uuid, handle);

    // A commission-side on/off toggle drives the equipment via the C++ command API.
    // Both On and Off map to a single toggle endpoint (POST buttons/{uuid}); we only
    // act on a genuine change and skip the echo from our own applyState.
    endpoint.events.onOff.onOff$Changed.on(async (value: boolean) => {
      if (handle.applyingRemote) return;
      if (value === handle.device.on) return;
      await this.client.toggleButton(device.uuid);
    });
  }

  private async addChlorinator(device: BridgedDevice): Promise<void> {
    const level = Math.max(0, Math.min(254, device.level ?? 0));
    const endpoint = new Endpoint(DimmablePlugInUnitDevice.with(BridgedDeviceBasicInformationServer), {
      id: MatterBridge.endpointId("swg", device.uuid),
      bridgedDeviceBasicInformation: this.bridgedBasicInfo(device),
      onOff: { onOff: device.on },
      levelControl: { currentLevel: level },
    });
    await this.aggregator!.add(endpoint);

    const handle: DeviceHandle = { endpoint, device, applyingRemote: false };
    this.handles.set(device.uuid, handle);

    // Level -> generating percentage (boost = 101 by convention, level 101-254 clamps to 100).
    endpoint.events.levelControl.currentLevel$Changed.on(async (value: number | null) => {
      if (handle.applyingRemote || value == null) return;
      const percentage = Math.max(0, Math.min(100, value));
      await this.client.setChlorinator({ percentage });
    });
  }

  private async addThermostat(device: BridgedDevice): Promise<void> {
    // A heat-only thermostat: localTemperature (read) + occupiedHeatingSetpoint (write).
    const endpoint = new Endpoint(
      ThermostatDevice.with(BridgedDeviceBasicInformationServer, ThermostatServer.with("Heating")),
      {
        id: MatterBridge.endpointId("heat", device.uuid),
        bridgedDeviceBasicInformation: this.bridgedBasicInfo(device),
        thermostat: {
          localTemperature: null,
          occupiedHeatingSetpoint: toCenti(28),
          minHeatSetpointLimit: toCenti(1),
          maxHeatSetpointLimit: toCenti(40),
          controlSequenceOfOperation: Thermostat.ControlSequenceOfOperation.HeatingOnly,
          systemMode: Thermostat.SystemMode.Heat,
        },
      },
    );
    await this.aggregator!.add(endpoint);

    const handle: DeviceHandle = { endpoint, device, applyingRemote: false };
    this.handles.set(device.uuid, handle);

    // A heater is bound to its body (pool/spa); a setpoint edit goes to that body.
    const isSpa = (device.bodyOfWater ?? "").toLowerCase().includes("spa");
    (endpoint.events as any).thermostat.occupiedHeatingSetpoint$Changed.on(async (value: number) => {
      if (handle.applyingRemote) return;
      const celsius = Math.round(fromCenti(value));
      await this.client.setSetpoints(isSpa ? { spa: celsius } : { pool: celsius });
    });
  }

  /** Push the latest known state of a device onto its endpoint attributes. */
  async applyDeviceState(device: BridgedDevice): Promise<void> {
    const handle = this.handles.get(device.uuid);
    if (!handle) return;
    handle.device = device;
    handle.applyingRemote = true;
    try {
      switch (device.kind) {
        case MatterKind.OnOffPlugInUnit:
          await setState(handle.endpoint, { onOff: { onOff: device.on } });
          break;
        case MatterKind.OnOffLevelChlorinator:
          await setState(handle.endpoint, {
            onOff: { onOff: device.on },
            levelControl: { currentLevel: Math.max(0, Math.min(254, device.level ?? 0)) },
          });
          break;
        case MatterKind.Thermostat:
          // Reflect the body temperature; the setpoint is owned by the commissioner.
          break;
        default:
          await setState(handle.endpoint, { onOff: { onOff: device.on } });
          break;
      }
    } finally {
      handle.applyingRemote = false;
    }
  }

  /** Toggle the cached on/off of a device by uuid (from a ButtonStateChange event). */
  async applyButtonState(uuid: string, on: boolean): Promise<void> {
    const handle = this.handles.get(uuid);
    if (!handle) return;
    await this.applyDeviceState({ ...handle.device, on });
  }

  /** Update / create read-only temperature sensors from the temperatures section. */
  async syncTemperatures(specs: TemperatureSensorSpec[]): Promise<void> {
    if (!this.aggregator) return;
    for (const spec of specs) {
      const existing = this.tempEndpoints.get(spec.id);
      const measured = spec.celsius == null ? null : toCenti(spec.celsius);
      if (existing) {
        await setState(existing, { temperatureMeasurement: { measuredValue: measured } });
        continue;
      }
      const endpoint = new Endpoint(TemperatureSensorDevice.with(BridgedDeviceBasicInformationServer), {
        id: `temp-${spec.id}`,
        bridgedDeviceBasicInformation: {
          nodeLabel: spec.label,
          productName: spec.label,
          productLabel: spec.label,
          serialNumber: `temp-${spec.id}`,
          reachable: true,
        },
        temperatureMeasurement: { measuredValue: measured },
      });
      await this.aggregator.add(endpoint);
      this.tempEndpoints.set(spec.id, endpoint);
    }
  }
}
