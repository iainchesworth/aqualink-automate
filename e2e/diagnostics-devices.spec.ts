import { test, expect } from '@playwright/test';

/**
 * Device diagnostics: GET /api/diagnostics/{emulated,actual}-devices always
 * respond (200) with a JSON array; any entries carry the shared
 * EmulatedDeviceDiagnostics shape (device_type/device_id + emulation flags).
 *
 * The Diagnostics page renders BOTH the "Emulated Devices" and "Actual Devices"
 * sections through the SAME shared deviceCard() component (the deviceGroups()
 * loop in index.html), so the two can never drift.
 */

for (const ep of ['emulated', 'actual'] as const) {
  test(`${ep}-devices endpoint responds with an array of device diagnostics`, async ({ request }) => {
    const resp = await request.get(`/api/diagnostics/${ep}-devices`);
    expect(resp.ok()).toBeTruthy();
    const json = await resp.json();
    expect(Array.isArray(json)).toBeTruthy();
    for (const dev of json) {
      expect(typeof dev.device_type).toBe('string');
      expect(typeof dev.device_id).toBe('string');
      expect(typeof dev.is_emulated).toBe('boolean');
      expect(typeof dev.emulation_suppressed).toBe('boolean');
    }
  });
}

test('Diagnostics page renders both device sections via the shared card', async ({ page }) => {
  await page.goto('/');
  await page.locator('.nav-link', { hasText: 'Diagnostics' }).click();

  for (const title of ['Emulated Devices', 'Actual Devices']) {
    // Each section is a .device-group whose .section-title carries the name; the
    // groups are always expanded in the redesign (no accordion toggle).
    const group = page.locator('.device-group', { hasText: title });
    await expect(group.locator('.section-title', { hasText: title })).toBeVisible({ timeout: 10_000 });
    await group.scrollIntoViewIfNeeded();

    // Either tailored device cards render (name + role from the shared component)
    // or the section's empty-state shows. The replay fixture may expose no
    // controller devices, so assert structurally rather than on a fixed count.
    const cards = group.locator('.device-mini-card');
    if ((await cards.count()) > 0) {
      await expect(cards.first().locator('.dmc-name')).toBeVisible();
      await expect(cards.first().locator('.dmc-role')).toBeVisible();
    } else {
      await expect(group.locator('p.text-muted')).toBeVisible();
    }
  }
});
