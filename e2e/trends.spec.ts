import { test, expect } from '@playwright/test';

/**
 * E2E — Trends / history view (WS2).
 *
 * Two modes, selected by AQUALINK_HISTORY_DB in playwright.config.ts:
 *   - default (no history db): the Trends view shows a graceful "disabled"
 *     message (the /api/history/series 503 path).
 *   - history enabled: the replayed AquaRite salt reading is recorded, so the
 *     Trends view lists at least one series and draws the chart canvas.
 */

const HISTORY_ENABLED = !!process.env.AQUALINK_HISTORY_DB;

test('Trends shows a disabled message when history is off', async ({ page }) => {
  test.skip(HISTORY_ENABLED, 'history is enabled in this run');

  await page.goto('/');
  await page.locator('.nav-link', { hasText: 'Trends' }).click();
  await expect(page.getByText(/History is disabled/i)).toBeVisible();
});

test('Trends lists series and renders the chart when history is on', async ({ page }) => {
  test.skip(!HISTORY_ENABLED, 'history is disabled in this run');

  await page.goto('/');

  // Give the replayed AquaRite salt reading time to be recorded (a chemistry
  // event -> chem/salt_ppm sample), then open the Trends view.
  await page.waitForTimeout(4000);
  await page.locator('.nav-link', { hasText: 'Trends' }).click();

  // At least one series toggle appears and the chart canvas is present.
  await expect(page.locator('.trends-series-toggle').first()).toBeVisible({ timeout: 15_000 });
  await expect(page.locator('.trends-chart-wrap canvas')).toBeVisible();
});
