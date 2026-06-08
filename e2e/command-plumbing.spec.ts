import { test, expect, type Request } from '@playwright/test';

/**
 * E2E test 2 — command plumbing through the UI.
 *
 * The replay fixture (AquaRite SWG) causes the backend to materialise a
 * chlorinator auxiliary device labelled "AquaPure" (see
 * src/jandy/devices/aquarite_device.cpp -> EnsureChlorinatorDeviceExists()).
 * That device is surfaced by GET /api/equipment/buttons and rendered by the
 * dashboard as an `.eq-button` control.
 *
 * Clicking the control invokes Alpine.store('pool').toggleButton(id), which
 * issues POST /api/equipment/buttons/{id}.  In replay mode no physical device
 * responds, so we do NOT assert a hardware state change; we assert that the UI
 * click actually reaches the backend command endpoint and that the backend
 * returns a *documented* command response (200 toggled, or 503 if the pool
 * configuration has not been discovered from the replay) — i.e. the request was
 * accepted/handled, not dropped — plus the corresponding optimistic UI badge.
 *
 * Command buttons are only clickable when the system store reports "Ready"
 * (commandsEnabled). In dev/replay mode the backend marks EmulationDisabled, so
 * the /ws/equipment SystemStateUpdate reports "ready" on connect.
 */

const BUTTONS_COMMAND_RE = /\/api\/equipment\/buttons\/[^/]+$/;

test('clicking an equipment control sends a command request to the backend', async ({ page }) => {
  const consoleErrors: string[] = [];
  page.on('console', (msg) => {
    if (msg.type() === 'error') consoleErrors.push(msg.text());
  });

  await page.goto('/');

  // Wait for the equipment controls to load (the AquaPure chlorinator button).
  const equipmentSection = page.locator('.section-title', { hasText: 'Equipment Controls' });
  await expect(equipmentSection).toBeVisible();

  // The chlorinator renders as a non-heater equipment button labelled "AquaPure".
  const aquaPureButton = page
    .locator('.eq-button')
    .filter({ has: page.locator('.eq-button-label', { hasText: /AquaPure/i }) });

  await expect(aquaPureButton).toBeVisible({ timeout: 15_000 });

  // Sanity-check the command gate: the UI must consider commands enabled,
  // otherwise the click is short-circuited to a "blocked" badge and never
  // reaches the backend. (This asserts the dev/replay "ready" wiring.)
  const commandsEnabled = await page.evaluate(() => {
    // @ts-expect-error Alpine is a global injected by alpine.min.js
    return window.Alpine?.store('system')?.commandsEnabled;
  });
  expect(commandsEnabled, 'system store should report commands enabled in replay mode').toBe(true);

  // Arm the request/response interception BEFORE clicking so we capture the POST
  // the click triggers.  This is the load-bearing assertion: the UI->backend
  // command wiring fired.
  const commandRequestPromise = page.waitForRequest(
    (req: Request) => req.method() === 'POST' && BUTTONS_COMMAND_RE.test(req.url()),
    { timeout: 10_000 },
  );
  const commandResponsePromise = page.waitForResponse(
    (resp) => resp.request().method() === 'POST' && BUTTONS_COMMAND_RE.test(resp.url()),
    { timeout: 10_000 },
  );

  await aquaPureButton.click();

  const commandRequest = await commandRequestPromise;
  const commandResponse = await commandResponsePromise;

  // The POST targeted a concrete button UUID, i.e. the wiring resolved a device id.
  const commandUrl = commandRequest.url();
  expect(commandUrl).toMatch(BUTTONS_COMMAND_RE);
  const buttonId = commandUrl.split('/').pop() as string;

  // The backend must HANDLE the command (not 404/405/parse error). 200 = toggled
  // and accepted; 503 = accepted-but-system-not-yet-configured (PoolConfiguration
  // Unknown in this minimal fixture). Both are documented command responses in
  // swagger.yaml and prove the endpoint received and processed the click.
  const status = commandResponse.status();
  expect([200, 503]).toContain(status);

  // Optimistic / acknowledged UI feedback via the Alpine store (the source of
  // truth that drives the on-button badge).  toggleButton() sets 'applied' on a
  // 2xx response and 'rejected' otherwise — either proves the UI round-tripped
  // the command through the store rather than silently no-op'ing.  Poll because
  // the state is cleared again ~3s after it is set.
  await expect
    .poll(
      async () => {
        return page.evaluate((id) => {
          // @ts-expect-error Alpine is a global injected by alpine.min.js
          return window.Alpine?.store('pool')?.getCommandState(id);
        }, buttonId);
      },
      { timeout: 10_000, message: 'pool store should record a terminal command state' },
    )
    .toMatch(/applied|rejected/);

  // And the corresponding badge must actually render in the button.  Note the
  // applied/rejected spans BOTH exist in the DOM (x-show toggles visibility), so
  // we must assert on the *visible* one rather than .first() in DOM order.
  const expectedBadge = status === 200 ? 'cmd-applied' : 'cmd-rejected';
  const visibleBadge = aquaPureButton
    .locator(`.cmd-badge.${expectedBadge}`)
    .filter({ visible: true });
  await expect(visibleBadge).toBeVisible({ timeout: 10_000 });

  // A 503 command response (accepted, but PoolConfiguration is Unknown in this
  // minimal replay fixture) makes the browser log "Failed to load resource ... 503".
  // That is the expected replay-mode outcome (503 is asserted as a valid status
  // above), not a UI defect — exclude it; any OTHER console error still fails.
  const unexpectedErrors = consoleErrors.filter(
    (e) => !/Failed to load resource.*\b503\b/.test(e),
  );
  expect(unexpectedErrors, `unexpected browser console errors: ${unexpectedErrors.join(' | ')}`)
    .toEqual([]);
});
