import { test, expect } from '@playwright/test';

/**
 * Dashboard UX redesign:
 *   - chemistry consolidated into one section (no duplicate chlorinator card),
 *   - equipment controls limited to controllable devices, rendered as switches,
 *   - a `controllable` flag on the buttons API drives the split.
 */

function isHeater(b: any): boolean {
  return b.device_type === 'Heater' || (b.label && String(b.label).toLowerCase().includes('heat'));
}

test('buttons API exposes a controllable flag; chlorinator/unknown are not controllable', async ({ request }) => {
  const buttons = (await (await request.get('/api/equipment/buttons')).json()).buttons;
  test.skip(!Array.isArray(buttons) || buttons.length === 0, 'no devices discovered in this fixture');

  for (const b of buttons) {
    expect(typeof b.controllable).toBe('boolean');
  }
  for (const b of buttons.filter((x: any) => x.device_type === 'Chlorinator' || x.device_type === 'Unknown')) {
    expect(b.controllable).toBe(false);
  }
});

test('dashboard has one consolidated chemistry section — no duplicate chlorinator card', async ({ page }) => {
  await page.goto('/');
  await expect(page.getByText('Water Chemistry')).toBeVisible();
  await expect(page.getByText('Chemistry / Chlorinator')).toHaveCount(0);
});

test('equipment controls render as switches for controllable devices only', async ({ page, request }) => {
  const buttons = (await (await request.get('/api/equipment/buttons')).json()).buttons;
  const controllable = buttons.filter((b: any) => b.controllable && !isHeater(b));
  test.skip(controllable.length === 0, 'no controllable non-heater devices in this fixture');

  await page.goto('/');
  await expect(page.locator('.eq-control').first()).toBeVisible();
  await expect(page.locator('.eq-switch').first()).toBeVisible();
  // The legacy icon-tile control is gone.
  await expect(page.locator('.eq-button')).toHaveCount(0);
});
