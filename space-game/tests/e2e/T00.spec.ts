import { test, expect, type Page } from '@playwright/test';

function collectErrors(page: Page): string[] {
  const errors: string[] = [];
  page.on('pageerror', (e) => errors.push(e.message));
  page.on('console', (m) => {
    if (m.type() === 'error') errors.push(m.text());
  });
  return errors;
}

test('Hauptmenü zeigt Titel und Buttons (Screenshot T00)', async ({ page }) => {
  const errors = collectErrors(page);
  await page.goto('/');
  const menu = page.locator('[data-scene="menu"]');
  await expect(menu.getByRole('heading', { name: 'ORBITWERK' })).toBeVisible();
  await expect(page.getByRole('button', { name: 'Hangar' })).toBeVisible();
  await expect(page.getByRole('button', { name: 'Flug' })).toBeVisible();
  // Loop läuft: ein paar Physik-Ticks abwarten.
  await page.waitForFunction(() => (window.orbitwerk?.loop.ticks ?? 0) > 10);
  await page.screenshot({ path: 'tests/screenshots/T00.png' });
  expect(errors).toEqual([]);
});

test('Hangar-Szene öffnen und zurück', async ({ page }) => {
  await page.goto('/');
  await page.getByRole('button', { name: 'Hangar' }).click();
  await expect(page.locator('[data-scene="editor"]')).toBeVisible();
  await expect(page.locator('body')).toHaveAttribute('data-active-scene', 'editor');
  await page.getByRole('button', { name: 'Menü' }).click();
  await expect(page.locator('[data-scene="menu"]')).toBeVisible();
});

test('Flug-Szene: Rapier-Kugel fällt und landet (Screenshot T00-flight)', async ({ page }) => {
  const errors = collectErrors(page);
  await page.goto('/');
  await page.getByRole('button', { name: 'Flug' }).click();
  await expect(page.locator('body')).toHaveAttribute('data-active-scene', 'flight');
  const height = () =>
    page.evaluate(() => (window.orbitwerk!.scenes.current as unknown as { ballHeight: number }).ballHeight);
  const start = await height();
  await page.waitForFunction(
    () => {
      const s = window.orbitwerk!.scenes.current as unknown as { ballHeight: number };
      return s.ballHeight < 1.2;
    },
    undefined,
    { timeout: 15_000 },
  );
  expect(start).toBeGreaterThan(5);
  await page.waitForTimeout(1500); // ausprallen lassen
  await expect(page.getByTestId('altitude')).toContainText('Höhe:');
  await page.screenshot({ path: 'tests/screenshots/T00-flight.png' });
  expect(errors).toEqual([]);
});
