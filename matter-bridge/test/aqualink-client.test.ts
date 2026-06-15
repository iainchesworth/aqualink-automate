import { strict as assert } from "node:assert";
import { test } from "node:test";

import { AqualinkClient } from "../src/aqualink-client.js";

interface Captured {
  url: string;
  method: string;
  headers: Record<string, string>;
  body?: string;
}

function makeFetch(captured: Captured[], response: { ok?: boolean; status?: number; json?: unknown }): typeof fetch {
  return (async (input: unknown, init?: { method?: string; headers?: Record<string, string>; body?: string }) => {
    captured.push({
      url: String(input),
      method: init?.method ?? "GET",
      headers: init?.headers ?? {},
      body: init?.body,
    });
    return {
      ok: response.ok ?? true,
      status: response.status ?? 200,
      json: async () => response.json ?? {},
    };
  }) as unknown as typeof fetch;
}

test("getDevices GETs /api/equipment/devices and returns parsed JSON", async () => {
  const captured: Captured[] = [];
  const client = new AqualinkClient({
    apiUrl: "http://127.0.0.1:80/",
    fetchImpl: makeFetch(captured, { json: { auxillaries: [{ id: "a1" }] } }),
  });

  const devices = await client.getDevices();
  assert.deepEqual(devices, { auxillaries: [{ id: "a1" }] });
  assert.equal(captured[0]!.url, "http://127.0.0.1:80/api/equipment/devices");
  assert.equal(captured[0]!.method, "GET");
});

test("toggleButton POSTs to the uuid path", async () => {
  const captured: Captured[] = [];
  const client = new AqualinkClient({
    apiUrl: "http://127.0.0.1:80",
    fetchImpl: makeFetch(captured, { ok: true, status: 200 }),
  });

  const outcome = await client.toggleButton("550e8400-e29b-41d4-a716-446655440000");
  assert.equal(outcome.ok, true);
  assert.equal(outcome.status, 200);
  assert.equal(
    captured[0]!.url,
    "http://127.0.0.1:80/api/equipment/buttons/550e8400-e29b-41d4-a716-446655440000",
  );
  assert.equal(captured[0]!.method, "POST");
});

test("setSetpoints sends Celsius body", async () => {
  const captured: Captured[] = [];
  const client = new AqualinkClient({
    apiUrl: "http://127.0.0.1:80",
    fetchImpl: makeFetch(captured, { ok: true, status: 200 }),
  });

  await client.setSetpoints({ pool: 26, spa: 38 });
  assert.equal(captured[0]!.url, "http://127.0.0.1:80/api/equipment/setpoints");
  assert.equal(captured[0]!.method, "POST");
  assert.deepEqual(JSON.parse(captured[0]!.body!), { pool: 26, spa: 38 });
});

test("setChlorinator sends percentage/boost body", async () => {
  const captured: Captured[] = [];
  const client = new AqualinkClient({
    apiUrl: "http://127.0.0.1:80",
    fetchImpl: makeFetch(captured, { ok: true, status: 200 }),
  });

  await client.setChlorinator({ percentage: 75, boost: true });
  assert.deepEqual(JSON.parse(captured[0]!.body!), { percentage: 75, boost: true });
});

test("a bearer token is attached when configured", async () => {
  const captured: Captured[] = [];
  const client = new AqualinkClient({
    apiUrl: "http://127.0.0.1:80",
    token: "s3cr3t",
    fetchImpl: makeFetch(captured, { json: {} }),
  });

  await client.getEquipment();
  assert.equal(captured[0]!.headers.Authorization, "Bearer s3cr3t");
});

test("isReady reflects API reachability", async () => {
  const okClient = new AqualinkClient({
    apiUrl: "http://127.0.0.1:80",
    fetchImpl: makeFetch([], { ok: true, status: 200, json: {} }),
  });
  assert.equal(await okClient.isReady(), true);

  const failFetch = (async () => {
    throw new Error("ECONNREFUSED");
  }) as unknown as typeof fetch;
  const downClient = new AqualinkClient({ apiUrl: "http://127.0.0.1:80", fetchImpl: failFetch });
  assert.equal(await downClient.isReady(), false);
});
