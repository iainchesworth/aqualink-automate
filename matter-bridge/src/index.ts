/**
 * Aqualink Matter bridge sidecar entry point.
 *
 * Boots the Matter node + status server, waits for the C++ API to come up, mirrors the
 * pool equipment into the Matter Aggregator, and keeps it live from /ws/equipment.
 */

import { loadConfig } from "./config.js";
import { AqualinkClient, type WsEvent } from "./aqualink-client.js";
import { MatterBridge } from "./bridge.js";
import { startStatusServer } from "./status-server.js";
import { parseDevices, parseTemperatureSensors } from "./device-map.js";

const log = (msg: string) => console.log(`[matter-bridge] ${msg}`);

async function waitForApi(client: AqualinkClient): Promise<void> {
  let delay = 1000;
  for (;;) {
    if (await client.isReady()) return;
    log(`aqualink API not ready yet; retrying in ${delay}ms`);
    await new Promise((r) => setTimeout(r, delay));
    delay = Math.min(delay * 2, 15000);
  }
}

async function refresh(client: AqualinkClient, bridge: MatterBridge): Promise<void> {
  try {
    const snapshot = await client.getEquipment();
    await bridge.syncDevices(parseDevices(snapshot.devices));
    await bridge.syncTemperatures(parseTemperatureSensors(snapshot.temperatures));
  } catch (err) {
    log(`refresh failed: ${(err as Error).message}`);
  }
}

async function main(): Promise<void> {
  const config = loadConfig();
  const client = new AqualinkClient({ apiUrl: config.apiUrl, token: config.apiToken });
  const bridge = new MatterBridge(config, client);

  log(`starting Matter bridge; API=${config.apiUrl} storage=${config.storagePath}`);
  await bridge.start();
  const statusServer = startStatusServer(bridge, { port: config.statusPort });

  const pairing = bridge.pairing();
  if (pairing) {
    log("================ Matter commissioning ================");
    log(`  QR payload:  ${pairing.qrPairingCode}`);
    log(`  Manual code: ${pairing.manualPairingCode}`);
    log("  Pair from Apple Home / Google Home / Alexa / SmartThings.");
    log("======================================================");
  }

  // Don't block forever on a slow-starting controller; mirror once it's ready.
  await waitForApi(client);
  await refresh(client, bridge);

  // Debounce WS-triggered refreshes so a burst of events causes one re-sync.
  let refreshTimer: ReturnType<typeof setTimeout> | undefined;
  const scheduleRefresh = () => {
    if (refreshTimer) return;
    refreshTimer = setTimeout(() => {
      refreshTimer = undefined;
      void refresh(client, bridge);
    }, 250);
  };

  const onEvent = (event: WsEvent) => {
    switch (event.type) {
      case "TemperatureUpdate": {
        const payload = event.payload as Record<string, number | null> | undefined;
        if (payload) void bridge.syncTemperatures(parseTemperatureSensors(payload));
        break;
      }
      case "ButtonStateChange":
      case "SystemStatusChange":
      case "ChemistryUpdate":
      case "SystemStateUpdate":
        // State changed somewhere; re-sync the device set (cheap, debounced).
        scheduleRefresh();
        break;
      default:
        break;
    }
  };

  const ws = client.connectWebSocket({
    onEvent,
    onOpen: () => log("connected to /ws/equipment"),
    onClose: () => log("disconnected from /ws/equipment (will reconnect)"),
  });

  // Periodic safety re-sync in case a WS update was missed.
  const periodic = setInterval(() => void refresh(client, bridge), 60000);

  const shutdown = async (signal: string) => {
    log(`received ${signal}; shutting down`);
    clearInterval(periodic);
    ws.close();
    statusServer.close();
    await bridge.stop();
    process.exit(0);
  };
  process.on("SIGINT", () => void shutdown("SIGINT"));
  process.on("SIGTERM", () => void shutdown("SIGTERM"));
}

main().catch((err) => {
  console.error(`[matter-bridge] fatal: ${(err as Error).stack ?? err}`);
  process.exit(1);
});
