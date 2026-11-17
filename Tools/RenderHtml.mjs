#!/usr/bin/env node
/*====================================================================================================================================
                                                      RENDERHTML.MJS
====================================================================================================================================*/
// 🧩 Headless Chromium renderer for standalone HTML card prototypes and live viewer pages

import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { execFileSync, spawnSync } from 'node:child_process';
import { createRequire } from 'node:module';

const RENDER_DIR = '/tmp/render';
const LIB_DIR = '/tmp/al2023/lib';

// 📝 Ensure bundled Amazon Linux shared libs (libnspr4, libnss3, etc.) are extracted for headless Chromium
function EnsureChromiumLibs() {
    if (fs.existsSync(path.join(LIB_DIR, 'libnspr4.so'))) return;
    const ArchivePath = path.join(RENDER_DIR, 'node_modules/@sparticuz/chromium/bin/al2023.tar.br');
    if (!fs.existsSync(ArchivePath)) {
        throw new Error('Headless Chromium runtime missing in /tmp/render — install @sparticuz/chromium and puppeteer-core first.');
    }
    fs.mkdirSync('/tmp/al2023', { recursive: true });
    const TarBytes = zlib.brotliDecompressSync(fs.readFileSync(ArchivePath));
    fs.writeFileSync('/tmp/al2023.tar', TarBytes);
    execFileSync('tar', ['-xf', '/tmp/al2023.tar', '-C', '/tmp/al2023']);
}

// 📝 Re-exec with LD_LIBRARY_PATH when launched directly so dlopen finds bundled shared libs
function EnsureLibraryEnvironment() {
    const CurrentPath = process.env.LD_LIBRARY_PATH || '';
    if (CurrentPath.includes(LIB_DIR)) return;
    EnsureChromiumLibs();
    const NextEnv = {
        ...process.env,
        LD_LIBRARY_PATH: `${LIB_DIR}:/tmp${CurrentPath ? ':' + CurrentPath : ''}`,
    };
    const Result = spawnSync(process.execPath, process.argv.slice(1), { env: NextEnv, stdio: 'inherit' });
    process.exit(Result.status ?? 1);
}

EnsureLibraryEnvironment();

const require = createRequire(path.join(RENDER_DIR, 'package.json'));
const ChromiumMod = require('@sparticuz/chromium');
const Chromium = ChromiumMod.default || ChromiumMod;
const Puppeteer = require('puppeteer-core');

async function RenderTarget() {
    const Args = process.argv.slice(2);
    if (Args.length < 2) {
        console.error('Usage: node Tools/RenderHtml.mjs <url-or-file> <output.png> [width=1280] [height=900] [--selector=.card] [--scale=2] [--wait=600]');
        process.exit(2);
    }

    const TargetArg = Args[0];
    const OutputPath = path.resolve(Args[1]);
    let Width = 1280;
    let Height = 900;
    let Scale = 2;
    let Selector = null;
    let WaitMs = 600;
    let FullPage = false;

    for (let Index = 2; Index < Args.length; Index++) {
        const Token = Args[Index];
        if (Token.startsWith('--selector=')) Selector = Token.slice('--selector='.length);
        else if (Token.startsWith('--scale=')) Scale = Number(Token.slice('--scale='.length)) || 2;
        else if (Token.startsWith('--wait=')) WaitMs = Number(Token.slice('--wait='.length)) || 600;
        else if (Token === '--full-page') FullPage = true;
        else if (/^\d+$/.test(Token)) {
            if (Index === 2) Width = Number(Token);
            else if (Index === 3) Height = Number(Token);
        }
    }

    const TargetUrl = /^https?:\/\//i.test(TargetArg) ? TargetArg : `file://${path.resolve(TargetArg)}`;
    fs.mkdirSync(path.dirname(OutputPath), { recursive: true });

    const ExecutablePath = await Chromium.executablePath();
    const Browser = await Puppeteer.launch({
        args: [
            ...Chromium.args,
            '--no-sandbox',
            '--disable-setuid-sandbox',
            '--use-gl=angle',
            '--use-angle=swiftshader',
            '--enable-webgl',
            '--ignore-gpu-blocklist',
        ],
        executablePath: ExecutablePath,
        headless: true,
    });

    try {
        const Page = await Browser.newPage();
        await Page.setViewport({ width: Width, height: Height, deviceScaleFactor: Scale });
        Page.on('pageerror', (ErrorObj) => console.error('[PAGE ERROR]', ErrorObj.message));
        await Page.goto(TargetUrl, { waitUntil: 'networkidle0', timeout: 60000 });
        await new Promise((Resolve) => setTimeout(Resolve, WaitMs));

        if (Selector) {
            const Handle = await Page.$(Selector);
            if (!Handle) throw new Error(`Selector not found: ${Selector}`);
            await Handle.screenshot({ path: OutputPath });
        } else {
            await Page.screenshot({ path: OutputPath, fullPage: FullPage });
        }
        console.log(`Rendered ${TargetUrl} -> ${OutputPath}`);
    } finally {
        await Browser.close();
    }
}

RenderTarget().catch((ErrorObj) => {
    console.error(ErrorObj);
    process.exit(1);
});
