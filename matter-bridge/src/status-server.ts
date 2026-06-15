/**
 * A tiny HTTP server exposing the bridge's commissioning QR + status so the C++ app
 * can embed it in its web UI (GET /api/diagnostics/matter fetches from here).
 *
 *   GET /matter/status -> { running, paired, fabrics, qr_payload, manual_code }
 *   GET /matter/qr     -> the same payload (kept as a stable, descriptive alias)
 *   GET /healthz       -> 200 "ok"
 */

import { createServer, type Server } from "node:http";

import type { MatterBridge } from "./bridge.js";

export interface StatusServerOptions {
  port: number;
  host?: string;
}

export function startStatusServer(bridge: MatterBridge, opts: StatusServerOptions): Server {
  const host = opts.host ?? "127.0.0.1";

  const statusPayload = () => {
    const pairing = bridge.pairing();
    return {
      running: bridge.isRunning,
      paired: bridge.isPaired,
      fabrics: bridge.fabricCount,
      qr_payload: pairing?.qrPairingCode ?? null,
      manual_code: pairing?.manualPairingCode ?? null,
    };
  };

  const server = createServer((req, res) => {
    const url = (req.url ?? "/").split("?")[0];
    if (req.method === "GET" && (url === "/matter/status" || url === "/matter/qr")) {
      const body = JSON.stringify(statusPayload());
      res.writeHead(200, { "Content-Type": "application/json", "Cache-Control": "no-store" });
      res.end(body);
      return;
    }
    if (req.method === "GET" && url === "/healthz") {
      res.writeHead(200, { "Content-Type": "text/plain" });
      res.end("ok");
      return;
    }
    res.writeHead(404, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ error: "not found" }));
  });

  server.listen(opts.port, host, () => {
    // eslint-disable-next-line no-console
    console.log(`[matter-bridge] status server listening on http://${host}:${opts.port}/matter/status`);
  });

  return server;
}
