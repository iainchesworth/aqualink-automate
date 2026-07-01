import { test, expect } from '@playwright/test';

/**
 * MQTT diagnostics: GET /api/diagnostics/mqtt always responds (200), returning
 * { enabled: false } when MQTT is disabled (the default replay fixture), and the
 * Diagnostics page surfaces an always-visible MQTT broker panel.
 */

test('MQTT diagnostics endpoint responds and reports disabled in replay', async ({ request }) => {
  const resp = await request.get('/api/diagnostics/mqtt');
  expect(resp.ok()).toBeTruthy();
  const json = await resp.json();
  expect(typeof json.enabled).toBe('boolean');
  expect(json.enabled).toBe(false); // MQTT is not enabled in the replay fixture
});

test('Diagnostics page shows the MQTT panel', async ({ page }) => {
  await page.goto('/');
  await page.locator('.nav-link', { hasText: 'Diagnostics' }).click();

  // The MQTT broker card is always visible in the redesign (no accordion toggle).
  const panel = page.locator('.mqtt-card');
  await expect(panel).toBeVisible({ timeout: 10_000 });
  await expect(panel.locator('.mqtt-status')).toHaveText(/Disabled/);
});
