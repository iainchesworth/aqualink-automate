import { test, expect } from '@playwright/test';

/**
 * E2E — Progressive Web App polish (WS7).
 *
 * The app is booted (by playwright.config.ts `webServer`) on a plain-HTTP
 * loopback origin (127.0.0.1), which browsers treat as a secure context, so the
 * service worker registers exactly as it would over HTTPS.
 *
 * Asserted:
 *   - the manifest is served, parses, and is installable-shaped (icons + display)
 *   - index.html links the manifest and the service worker reaches "active"
 *   - the service-worker cache holds the static shell but NEVER an /api response
 */

test('manifest is served, valid JSON, and installable-shaped', async ({ request }) => {
  const resp = await request.get('/manifest.json');
  expect(resp.ok()).toBeTruthy();

  const manifest = await resp.json();
  expect(manifest.name).toBe('Aqualink Automate');
  expect(manifest.display).toBe('standalone');
  expect(manifest.start_url).toBe('/');

  expect(Array.isArray(manifest.icons)).toBeTruthy();
  const sizes = manifest.icons.map((i: { sizes: string }) => i.sizes);
  expect(sizes).toContain('192x192');
  expect(sizes).toContain('512x512');
  // At least one maskable icon for adaptive install surfaces.
  expect(
    manifest.icons.some((i: { purpose?: string }) => (i.purpose ?? '').includes('maskable')),
  ).toBeTruthy();
});

test('index.html links the manifest and the service worker becomes active', async ({ page }) => {
  await page.goto('/');

  await expect(page.locator('link[rel="manifest"]')).toHaveAttribute('href', '/manifest.json');

  // navigator.serviceWorker.ready resolves once a worker is active for this scope.
  const hasActiveWorker = await page.evaluate(async () => {
    if (!('serviceWorker' in navigator)) return false;
    const reg = await navigator.serviceWorker.ready;
    return !!reg.active;
  });
  expect(hasActiveWorker).toBeTruthy();
});

test('service worker caches the static shell but never the API', async ({ page }) => {
  await page.goto('/');
  await page.evaluate(() => navigator.serviceWorker.ready);

  // Issue an /api request through the (now controlling) service worker; it must
  // be served network-only and must not be added to the shell cache.
  await page.evaluate(async () => {
    await fetch('/api/version').catch(() => {});
  });

  const cacheState = await page.evaluate(async () => {
    const names = await caches.keys();
    const shell = names.find((n) => n.startsWith('aqualink-shell-')) ?? null;
    if (!shell) {
      return { shell: null as string | null, apiCached: null, iconCached: null };
    }
    const cache = await caches.open(shell);
    const keys = await cache.keys();
    const paths = keys.map((r) => new URL(r.url).pathname);
    return {
      shell,
      apiCached: paths.some((p) => p.startsWith('/api')),
      iconCached: paths.some((p) => p === '/icons/icon-192.png'),
    };
  });

  expect(cacheState.shell).not.toBeNull();
  // The precached app-shell icon proves the SW populated its cache...
  expect(cacheState.iconCached).toBe(true);
  // ...but no /api response may ever be cached (network-only contract).
  expect(cacheState.apiCached).toBe(false);
});
