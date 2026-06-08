import { test, expect } from '@playwright/test';

/**
 * E2E test 1 — UI render from the replay fixture.
 *
 * The app is booted (by playwright.config.ts `webServer`) against
 * test/fixtures/sample_session.cap, which replays an AquaRite salt-water
 * generator reporting 40% output / 3200 PPM salt.  The chlorinator chemistry
 * reaches the browser via the /ws/equipment WebSocket (ChemistryUpdate ->
 * salt_level) and/or the GET /api/equipment REST snapshot (chemistry.salt_in_ppm).
 *
 * The dashboard renders the salt value in a "Salt" chemistry gauge whose
 * displayed text is "<ppm> ppm" (see assets/web/scripts/components/chemistry-gauge.js
 * -> displayValue = numericValue.toFixed(0) + ' ppm').
 */
test('dashboard renders the replayed AquaRite salt reading (3200 ppm)', async ({ page }) => {
  // Surface backend/UI console errors in the test log to aid debugging.
  page.on('console', (msg) => {
    if (msg.type() === 'error') console.log('[browser console error]', msg.text());
  });

  await page.goto('/');

  // The dashboard view is the default route.
  await expect(page.locator('.section-title', { hasText: 'Water Chemistry' })).toBeVisible();

  // Locate the Salt gauge card by its label, then read its value cell.
  const saltCard = page
    .locator('.gauge-card')
    .filter({ has: page.locator('.gauge-label', { hasText: 'Salt' }) });
  await expect(saltCard).toBeVisible();

  const saltValue = saltCard.locator('.gauge-value');

  // The replayed PPM (3200) must appear once the WS/REST feed has been applied.
  // chemistry-gauge formats it as "3200 ppm".
  await expect(saltValue).toHaveText(/3200\s*ppm/i, { timeout: 15_000 });
});

/**
 * Cross-check the data path at the source: the REST snapshot the UI fetches on
 * load must itself carry salt_in_ppm = 3200.  This isolates a UI-rendering
 * failure from a backend-decode failure if test 1 ever regresses.
 */
test('REST /api/equipment exposes the replayed salt concentration', async ({ request }) => {
  const resp = await request.get('/api/equipment');
  expect(resp.ok()).toBeTruthy();

  const body = await resp.json();
  expect(body).toHaveProperty('chemistry');
  expect(body.chemistry.salt_in_ppm).toBe(3200);
});
