import { defineConfig, devices } from '@playwright/test';
import { existsSync } from 'node:fs';
import { join } from 'node:path';

/**
 * Playwright end-to-end UI test configuration for Aqualink Automate.
 *
 * The `webServer` block boots the *real* application binary against a recorded
 * RS-485 replay fixture (an AquaRite salt-water generator session reporting
 * 40% output / 3200 PPM).  The app serves its Alpine.js web UI over plain HTTP
 * on a fixed test port; the tests then drive that UI in a real browser.
 *
 * App launch (see flags below):
 *   --dev-mode                       enable developer mode (gates --replay-filename)
 *   --replay-filename <fixture.cap>  source deterministic RS-485 data from the fixture
 *   --http-port <PORT>               serve the UI + REST/WS API on a fixed test port
 *   --address 127.0.0.1              bind to loopback only
 *   --disable-https                  no TLS in tests (avoids cert provisioning)
 *   --doc-root <assets/web>          serve THIS worktree's web UI assets
 *   --jandy-disable-emulation        skip the equipment-discovery phase so the
 *                                    WebSocket reports the system "ready" (commands
 *                                    enabled) immediately, instead of "starting".
 *   --profiler tracy                 required: --replay-filename has a hard
 *                                    dependency on --profiler.  Tracy is not
 *                                    compiled into the debug build, so the profiler
 *                                    factory falls back to a no-op profiler.
 *   --loglevel-<channel> info        required: --replay-filename also depends on
 *                                    EVERY per-channel log-level option (they carry
 *                                    no default, so the dependency only clears when
 *                                    each is passed explicitly).
 *
 * Conflict / dependency notes (verified against src/core/options/* and a live run):
 *   * --disable-https conflicts with --https-port / --cert / --cert-key, so none
 *     of those are passed.  The cert options carry *default* values, but the
 *     conflict checker ignores defaulted options, so --disable-https is accepted.
 *   * --disable-http conflicts with --http-port, so --disable-http is NOT passed.
 *   * --replay-filename depends on --dev-mode, --profiler and all --loglevel-*
 *     options, and conflicts with --record-serial (not passed).
 */

// Every per-channel log level the developer options expose; --replay-filename
// requires each to be present (see option_dependency_helper.cpp).
const LOG_CHANNELS = [
  'main', 'certificates', 'coroutines', 'devices', 'equipment', 'exceptions',
  'messages', 'mqtt', 'navigation', 'options', 'platform', 'profiling',
  'protocol', 'scraping', 'serial', 'signals', 'web',
];

const HOST = '127.0.0.1';
const PORT = Number(process.env.AQUALINK_TEST_PORT ?? 18080);
const BASE_URL = `http://${HOST}:${PORT}`;

const ROOT = __dirname;

// Resolve the built application binary.  The `wt` CMake preset emits it under
// build/wt/src/.  Allow an override for CI / alternate build dirs.
const APP_EXE =
  process.env.AQUALINK_EXE ?? join(ROOT, 'build', 'wt', 'src', 'aqualink-automate.exe');

const DOC_ROOT = join(ROOT, 'assets', 'web');
const REPLAY_FIXTURE = join(ROOT, 'test', 'fixtures', 'sample_session.cap');

if (!existsSync(APP_EXE)) {
  // Fail loudly with an actionable message rather than letting Playwright emit a
  // generic "webServer failed to start" after a long timeout.
  throw new Error(
    `Application binary not found at ${APP_EXE}.\n` +
      `Build it first (from the VS Dev Shell):\n` +
      `  cmake --preset wt && cmake --build --preset wt --target aqualink-automate\n` +
      `or set AQUALINK_EXE to the binary path.`,
  );
}

export default defineConfig({
  testDir: './e2e',
  // Replay is deterministic but the UI is global mutable state behind one backend;
  // run serially so command-button tests don't race each other's optimistic updates.
  fullyParallel: false,
  workers: 1,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 1 : 0,
  reporter: [['list'], ['html', { open: 'never' }]],

  timeout: 30_000,
  expect: { timeout: 10_000 },

  use: {
    baseURL: BASE_URL,
    trace: 'on-first-retry',
    screenshot: 'only-on-failure',
  },

  projects: [
    {
      name: 'chromium',
      use: { ...devices['Desktop Chrome'] },
    },
  ],

  webServer: {
    command: [
      `"${APP_EXE}"`,
      '--dev-mode',
      `--replay-filename "${REPLAY_FIXTURE}"`,
      `--http-port ${PORT}`,
      `--address ${HOST}`,
      '--disable-https',
      `--doc-root "${DOC_ROOT}"`,
      '--jandy-disable-emulation',
      '--profiler tracy',
      ...LOG_CHANNELS.map((ch) => `--loglevel-${ch} info`),
    ].join(' '),
    url: BASE_URL,
    reuseExistingServer: false,
    timeout: 120_000,
    stdout: 'pipe',
    stderr: 'pipe',
  },
});
