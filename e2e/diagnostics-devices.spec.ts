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

  for (const section of [
    { title: 'Emulated Devices', id: '#diag-emulated-devices' },
    { title: 'Actual Devices', id: '#diag-actual-devices' },
  ]) {
    const toggle = page.locator('.section-toggle', { hasText: section.title });
    await expect(toggle).toBeVisible();
    await toggle.scrollIntoViewIfNeeded();

    // Sections default to expanded; open if a prior interaction collapsed it.
    const panel = page.locator(section.id);
    if (!(await panel.isVisible())) {
      await toggle.click();
    }
    await expect(panel).toBeVisible({ timeout: 10_000 });

    // Either tailored device cards render (icon + role from the shared component)
    // or the section's empty-state shows. The replay fixture may expose no
    // controller devices, so assert structurally rather than on a fixed count.
    const cards = panel.locator('.emu-device-card');
    if ((await cards.count()) > 0) {
      await expect(cards.first().locator('.device-card-icon')).toBeVisible();
      await expect(cards.first().locator('.device-card-role')).toBeVisible();
    } else {
      await expect(panel.locator('p.text-muted')).toBeVisible();
    }
  }
});
