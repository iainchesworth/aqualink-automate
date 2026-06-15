/**
 * Client for the aqualink-automate HTTP + WebSocket equipment API.
 *
 * REST is used for discovery + initial state and for sending commands; the
 * /ws/equipment WebSocket carries live state changes (ButtonStateChange,
 * TemperatureUpdate, ChemistryUpdate, ...). The client reconnects the WebSocket with
 * exponential backoff and degrades gracefully while the C++ system reports "starting".
 */

import WebSocket from "ws";

import type { RawDevices } from "./device-map.js";

export interface EquipmentSnapshot {
  temperatures?: Record<string, number | null>;
  chemistry?: Record<string, unknown>;
  configuration?: Record<string, unknown>;
  devices?: RawDevices;
}

/** A parsed /ws/equipment event: { type, payload }. */
export interface WsEvent {
  type: string;
  payload: unknown;
}

export interface AqualinkClientOptions {
  /** Base URL, e.g. http://127.0.0.1:80 (no trailing slash). */
  apiUrl: string;
  /** Optional bearer token for --api-auth-token deployments. */
  token?: string | undefined;
  /** Injectable fetch (defaults to global fetch) - used by unit tests. */
  fetchImpl?: typeof fetch;
}

export type CommandOutcome = { ok: boolean; status: number };

export class AqualinkClient {
  private readonly apiUrl: string;
  private readonly token: string | undefined;
  private readonly fetchImpl: typeof fetch;

  constructor(opts: AqualinkClientOptions) {
    this.apiUrl = opts.apiUrl.replace(/\/+$/, "");
    this.token = opts.token;
    this.fetchImpl = opts.fetchImpl ?? globalThis.fetch;
  }

  private headers(extra?: Record<string, string>): Record<string, string> {
    const h: Record<string, string> = { Accept: "application/json", ...extra };
    if (this.token) h.Authorization = `Bearer ${this.token}`;
    return h;
  }

  // ---- REST: discovery + state -------------------------------------------

  /** GET /api/equipment - the full snapshot (temperatures, chemistry, devices). */
  async getEquipment(): Promise<EquipmentSnapshot> {
    const res = await this.fetchImpl(`${this.apiUrl}/api/equipment`, { headers: this.headers() });
    if (!res.ok) throw new Error(`GET /api/equipment failed: ${res.status}`);
    return (await res.json()) as EquipmentSnapshot;
  }

  /** GET /api/equipment/devices - just the device buckets. */
  async getDevices(): Promise<RawDevices> {
    const res = await this.fetchImpl(`${this.apiUrl}/api/equipment/devices`, { headers: this.headers() });
    if (!res.ok) throw new Error(`GET /api/equipment/devices failed: ${res.status}`);
    return (await res.json()) as RawDevices;
  }

  /** True once the system is past "starting" and the API serves equipment. */
  async isReady(): Promise<boolean> {
    try {
      await this.getEquipment();
      return true;
    } catch {
      return false;
    }
  }

  // ---- REST: commands -----------------------------------------------------

  /** POST /api/equipment/buttons/{uuid} - toggle an aux/pump/light on/off. */
  async toggleButton(uuid: string): Promise<CommandOutcome> {
    const res = await this.fetchImpl(`${this.apiUrl}/api/equipment/buttons/${encodeURIComponent(uuid)}`, {
      method: "POST",
      headers: this.headers({ "Content-Type": "application/json" }),
    });
    return { ok: res.ok, status: res.status };
  }

  /** POST /api/equipment/setpoints - body is Celsius { pool?, spa? }. */
  async setSetpoints(body: { pool?: number; spa?: number }): Promise<CommandOutcome> {
    const res = await this.fetchImpl(`${this.apiUrl}/api/equipment/setpoints`, {
      method: "POST",
      headers: this.headers({ "Content-Type": "application/json" }),
      body: JSON.stringify(body),
    });
    return { ok: res.ok, status: res.status };
  }

  /** POST /api/equipment/chlorinator - { percentage?, boost? }. */
  async setChlorinator(body: { percentage?: number; boost?: boolean }): Promise<CommandOutcome> {
    const res = await this.fetchImpl(`${this.apiUrl}/api/equipment/chlorinator`, {
      method: "POST",
      headers: this.headers({ "Content-Type": "application/json" }),
      body: JSON.stringify(body),
    });
    return { ok: res.ok, status: res.status };
  }

  // ---- WebSocket: live updates -------------------------------------------

  /**
   * Open /ws/equipment and stream parsed events to onEvent until close() on the
   * returned handle. Reconnects with exponential backoff (capped) on drop/error.
   */
  connectWebSocket(handlers: {
    onEvent: (event: WsEvent) => void;
    onOpen?: () => void;
    onClose?: () => void;
  }): { close: () => void } {
    const wsUrl = this.apiUrl.replace(/^http/, "ws") + "/ws/equipment";
    let closed = false;
    let socket: WebSocket | undefined;
    let backoffMs = 1000;
    const backoffMaxMs = 30000;
    let reconnectTimer: ReturnType<typeof setTimeout> | undefined;

    const wsHeaders: Record<string, string> = {};
    if (this.token) wsHeaders.Authorization = `Bearer ${this.token}`;

    const scheduleReconnect = () => {
      if (closed) return;
      reconnectTimer = setTimeout(connect, backoffMs);
      backoffMs = Math.min(backoffMs * 2, backoffMaxMs);
    };

    const connect = () => {
      if (closed) return;
      socket = new WebSocket(wsUrl, { headers: wsHeaders });

      socket.on("open", () => {
        backoffMs = 1000; // reset backoff on a successful connect
        handlers.onOpen?.();
      });

      socket.on("message", (data: WebSocket.RawData) => {
        try {
          const parsed = JSON.parse(data.toString()) as WsEvent;
          if (parsed && typeof parsed.type === "string") {
            handlers.onEvent(parsed);
          }
        } catch {
          // Ignore malformed frames; the next valid one will refresh state.
        }
      });

      socket.on("close", () => {
        handlers.onClose?.();
        scheduleReconnect();
      });

      socket.on("error", () => {
        // 'close' fires after 'error'; let it drive the reconnect.
        socket?.close();
      });
    };

    connect();

    return {
      close: () => {
        closed = true;
        if (reconnectTimer) clearTimeout(reconnectTimer);
        socket?.close();
      },
    };
  }
}
