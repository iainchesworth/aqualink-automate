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

    // Scope field/button lookups to the overlay as a defensive measure: even
    // though `.login-input` is now unique to the overlay (the Schedules form
    // uses `.sched-input`), scoping keeps these locators robust against any
    // future class reuse and avoids strict-mode surprises.
    // Wrong token -> error, still on the login overlay.
    await overlay.locator('.login-input').fill('definitely-wrong-token');
    await overlay.locator('.login-submit').click();
    await expect(overlay.locator('.login-error')).toBeVisible();
    await expect(overlay).toBeVisible();

    // Correct token -> overlay hides and the dashboard renders.
    await overlay.locator('.login-input').fill(TOKEN);
    await overlay.locator('.login-submit').click();
    await expect(overlay).toBeHidden();
    await expect(page.locator('.section-title', { hasText: 'Water Chemistry' })).toBeVisible();

    // The replayed salt (3200 ppm) arrives over the authenticated /ws/equipment
    // socket — the heart of the WS-subprotocol auth regression.
    const saltCard = page
      .locator('.gauge-card')
      .filter({ has: page.locator('.gauge-label', { hasText: 'Salt' }) });
    await expect(saltCard.locator('.chem-dial-num')).toHaveText(/3200/, { timeout: 15_000 });
    await expect(saltCard.locator('.chem-dial-unit')).toHaveText(/ppm/i);
  });

  test('reload stays logged in; logout returns to the login overlay', async ({ page }) => {
    // Fresh context => empty storage; "remember on this device" must then survive
    // a reload (an addInitScript that cleared storage here would wrongly wipe it).
    await page.goto('/');

    // Log in, choosing "remember on this device" so the token persists a reload.
    const overlay = page.locator('.login-overlay');
    await overlay.locator('.login-input').fill(TOKEN);
    await overlay.locator('.login-remember input').check();
    await overlay.locator('.login-submit').click();
    await expect(overlay).toBeHidden();

    // Reload -> still authenticated, no overlay.
    await page.reload();
    await expect(overlay).toBeHidden();
    await expect(page.locator('.section-title', { hasText: 'Water Chemistry' })).toBeVisible();

    // Logout -> overlay returns.
    await page.locator('button[title="Sign out"]').click();
    await expect(overlay).toBeVisible();
  });
});
