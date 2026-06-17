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
  // Exact match: the Trends view also has a "Water chemistry" group label, so a loose
  // (case-insensitive substring) match would resolve to two elements across the SPA.
  await expect(page.getByText('Water Chemistry', { exact: true })).toBeVisible();
  await expect(page.getByText('Chemistry / Chlorinator')).toHaveCount(0);
});

test('chlorinator output control: API validates, and the slider renders when present', async ({ page, request }) => {
  // A valid percentage is handled (200 toggled, or 503 when no commandable
  // chlorinator/iAQ device is present in this minimal fixture).
  const ok = await request.post('/api/equipment/chlorinator', { data: { percentage: 60 } });
  expect([200, 503]).toContain(ok.status());

  // Out-of-range is rejected BEFORE dispatch.
  expect((await request.post('/api/equipment/chlorinator', { data: { percentage: 150 } })).status()).toBe(400);
  expect((await request.post('/api/equipment/chlorinator', { data: { boost: 'yes' } })).status()).toBe(400);

  // Boost is accepted/handled.
  expect([200, 503]).toContain((await request.post('/api/equipment/chlorinator', { data: { boost: true } })).status());

  // The target control is integrated into the SWG Output tile (the fixture
  // materialises a chlorinator).
  await page.goto('/');
  await expect(page.getByText('SWG Output')).toBeVisible();
  await expect(page.locator('.swg-card .swg-slider')).toBeVisible();
  await expect(page.locator('.swg-card').getByRole('button', { name: 'Boost' })).toBeVisible();
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
