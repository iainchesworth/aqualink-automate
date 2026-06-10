import { test, expect } from '@playwright/test';

/**
 * E2E — user/admin preferences (server-backed Settings).
 *
 * The preferences service is always present (it works in-memory even without
 * --preferences-file), so these run in the default mode.
 */

test('preferences API round-trips and validates', async ({ request }) => {
  const get1 = await request.get('/api/preferences');
  expect(get1.ok()).toBeTruthy();
  const before = await get1.json();
  expect(before).toHaveProperty('alert');
  expect(before).toHaveProperty('history');

  // A valid partial update applies...
  const put = await request.put('/api/preferences', { data: { alert: { salt_low_ppm: 2900 } } });
  expect(put.ok()).toBeTruthy();

  const after = await (await request.get('/api/preferences')).json();
  expect(after.alert.salt_low_ppm).toBe(2900);

  // ...and an out-of-range value is rejected (whole request).
  const bad = await request.put('/api/preferences', { data: { alert: { salt_low_ppm: 99999 } } });
  expect(bad.status()).toBe(400);

  // The rejected value did not change anything.
  const unchanged = await (await request.get('/api/preferences')).json();
  expect(unchanged.alert.salt_low_ppm).toBe(2900);
});

test('Settings view shows and saves server preferences', async ({ page }) => {
  await page.goto('/');
  await page.locator('.nav-link', { hasText: 'Settings' }).click();

  await expect(page.getByText('System Preferences')).toBeVisible();

  // Change the temperature units and save -> a "Saved" confirmation appears.
  await page.locator('select[x-model="prefs.temperature_units"]').selectOption('Fahrenheit');
  await page.getByRole('button', { name: 'Save preferences' }).click();
  await expect(page.getByText('Saved ✓')).toBeVisible({ timeout: 5_000 });

  // The change is reflected by the API.
  const prefs = await (await page.request.get('/api/preferences')).json();
  expect(prefs.temperature_units).toBe('Fahrenheit');
});
