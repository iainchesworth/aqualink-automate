import { test, expect } from '@playwright/test';

/**
 * E2E — UI authentication (WS5).
 *
 * Runs ONLY when playwright.config.ts is invoked with AQUALINK_AUTH_TOKEN set,
 * which boots the app with `--api-auth-token <token>`. This is the regression
 * test for the WebSocket transport gap: a browser proves it can authenticate
 * `/ws/equipment` (which a browser cannot do with an Authorization header) by
 * receiving the live replayed salt reading after login.
 */

const TOKEN = process.env.AQUALINK_AUTH_TOKEN ?? '';

test.describe('UI authentication', () => {
  test('login required; wrong token rejected; right token reaches the live dashboard', async ({ page }) => {
    // Each Playwright test gets a fresh, empty-storage context, so no token is
    // present until we log in below.
    await page.goto('/');

    // Static assets loaded (proving the static exemption) and the login overlay
    // is shown because a token is required.
    const overlay = page.locator('.login-overlay');
    await expect(overlay).toBeVisible();

    // Wrong token -> error, still on the login overlay.
    await page.locator('.login-input').fill('definitely-wrong-token');
    await page.locator('.login-submit').click();
    await expect(page.locator('.login-error')).toBeVisible();
    await expect(overlay).toBeVisible();

    // Correct token -> overlay hides and the dashboard renders.
    await page.locator('.login-input').fill(TOKEN);
    await page.locator('.login-submit').click();
    await expect(overlay).toBeHidden();
    await expect(page.locator('.section-title', { hasText: 'Water Chemistry' })).toBeVisible();

    // The replayed salt (3200 ppm) arrives over the authenticated /ws/equipment
    // socket — the heart of the WS-subprotocol auth regression.
    const saltCard = page
      .locator('.gauge-card')
      .filter({ has: page.locator('.gauge-label', { hasText: 'Salt' }) });
    await expect(saltCard.locator('.gauge-value')).toHaveText(/3200\s*ppm/i, { timeout: 15_000 });
  });

  test('reload stays logged in; logout returns to the login overlay', async ({ page }) => {
    // Fresh context => empty storage; "remember on this device" must then survive
    // a reload (an addInitScript that cleared storage here would wrongly wipe it).
    await page.goto('/');

    // Log in, choosing "remember on this device" so the token persists a reload.
    await page.locator('.login-input').fill(TOKEN);
    await page.locator('.login-remember input').check();
    await page.locator('.login-submit').click();
    await expect(page.locator('.login-overlay')).toBeHidden();

    // Reload -> still authenticated, no overlay.
    await page.reload();
    await expect(page.locator('.login-overlay')).toBeHidden();
    await expect(page.locator('.section-title', { hasText: 'Water Chemistry' })).toBeVisible();

    // Logout -> overlay returns.
    await page.locator('button[title="Sign out"]').click();
    await expect(page.locator('.login-overlay')).toBeVisible();
  });
});
