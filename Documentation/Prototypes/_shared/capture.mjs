/*==============================================================================================================
  CAPTURE.MJS — renders the standalone prototypes and the two live viewers to PNG.
  Run from /tmp/renderer with the unpacked Chromium (see Documentation/UI_REDESIGN_WORKFLOW.md).

      LD_LIBRARY_PATH=/tmp/renderer/libs:/tmp/renderer/libs/lib \
      FONTCONFIG_PATH=/tmp/renderer/libs node capture.mjs
==============================================================================================================*/
import { createRequire } from 'node:module';
import fs from 'node:fs';
const require = createRequire(import.meta.url);
// playwright-core lives outside the repo; PLAYWRIGHT_HOME points at the folder that has it installed.
const playwrightHome = process.env.PLAYWRIGHT_HOME || '/tmp/renderer/node_modules';
let chromium;
try {
    ({ chromium } = require('playwright-core'));
} catch (error) {
    ({ chromium } = require(`${playwrightHome}/playwright-core`));
}

const ROOT = new URL('../../../', import.meta.url).pathname.replace(/\/$/, '');
const OUT = process.env.CAPTURE_OUT || '/tmp/renderer/proto';
fs.mkdirSync(OUT, { recursive: true });

const browser = await chromium.launch({
    executablePath: '/tmp/chromium',
    headless: true,
    args: ['--no-sandbox', '--disable-setuid-sandbox', '--force-color-profile=srgb',
        '--font-render-hinting=none', '--hide-scrollbars'],
});

const errors = [];
const open = async (url, width, height, dpr = 2) => {
    const context = await browser.newContext({ viewport: { width, height }, deviceScaleFactor: dpr });
    const page = await context.newPage();
    page.on('pageerror', (e) => errors.push(`PAGEERROR ${url}: ${e.message}`));
    page.on('console', (m) => { if (m.type() === 'error') errors.push(`CONSOLE ${url}: ${m.text()}`); });
    await page.goto(url, { waitUntil: 'load' });
    await page.waitForTimeout(450);
    return page;
};

const drag = async (page, selector, fx1, fy1, fx2, fy2) => {
    const box = await page.locator(selector).boundingBox();
    await page.mouse.move(box.x + box.width * fx1, box.y + box.height * fy1);
    await page.mouse.down();
    await page.mouse.move(box.x + box.width * fx2, box.y + box.height * fy2, { steps: 14 });
    await page.mouse.up();
};

/*------------------------------------------------------------------ Placement Frame */
const frameUrl = `file://${ROOT}/Documentation/Prototypes/PlacementFrame/index.html`;
let page = await open(frameUrl, 620, 1400);
await page.locator('#Deck').screenshot({ path: `${OUT}/placement-frame-01-default.png` });
await page.context().close();

page = await open(frameUrl, 620, 1400);
await drag(page, '#Pad', 0.52, 0.52, 0.68, 0.36);
await drag(page, '#Dial', 0.5, 0.18, 0.86, 0.44);
await drag(page, '.meter[data-axis="x"]', 0.32, 0.5, 0.62, 0.5);
await drag(page, '#HeftPlot', 0.9, 0.4, 0.9, 0.58);
await page.locator('#GripDots button').nth(5).click();
await page.locator('#Slots button').nth(2).click();
await page.waitForTimeout(220);
await page.locator('#Deck').screenshot({ path: `${OUT}/placement-frame-02-engaged.png` });
await page.context().close();

page = await open(frameUrl, 620, 1400);
await drag(page, '#ZRail', 0.5, 0.28, 0.5, 0.74);
await page.waitForTimeout(220);
await page.locator('#Deck').screenshot({ path: `${OUT}/placement-frame-03-warning.png` });
await page.context().close();

/*------------------------------------------------------------------ Lattice Reach */
const latticeUrl = `file://${ROOT}/Documentation/Prototypes/LatticeReach/index.html`;
page = await open(latticeUrl, 560, 1200);
await page.locator('#Deck').screenshot({ path: `${OUT}/lattice-reach-01-default.png` });
await page.context().close();

page = await open(latticeUrl, 560, 1200);
await drag(page, '#Sweep', 0.72, 0.6, 0.95, 0.6);
await page.locator('#Grain button').nth(58).hover();
await page.waitForTimeout(220);
await page.locator('#Deck').screenshot({ path: `${OUT}/lattice-reach-02-swept.png` });
await page.context().close();

/*------------------------------------------------------------------ live Asset Viewer */
page = await open('http://127.0.0.1:8096/', 1520, 940, 1.5);
await page.screenshot({ path: `${OUT}/viewer-01-home.png` });
await page.click('#AddComponentBtn');
await page.waitForTimeout(300);
await page.screenshot({ path: `${OUT}/viewer-02-add-menu.png` });
await page.context().close();

console.log(JSON.stringify({ errors }, null, 2));
await browser.close();
