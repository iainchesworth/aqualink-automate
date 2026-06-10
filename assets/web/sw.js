/*
 * Aqualink Automate — service worker.
 *
 * Scope: progressive-web-app install + offline app shell ONLY.  Explicitly out
 * of scope (see WS7): offline command queueing and push notifications.
 *
 * Caching strategy:
 *   - Static shell (HTML / JS / CSS / icons / fonts-from-same-origin): cache
 *     first, falling back to network and populating the cache on a miss.
 *   - Navigations: serve the cached app shell (index.html) when the network is
 *     unavailable so the SPA still boots offline.
 *   - /api/*  : NETWORK ONLY.  Never cached and never served from cache — API
 *     responses (equipment state, auth checks, etc.) must always be live, and a
 *     stale cached /api/auth/check could wrongly keep an unauthenticated client
 *     "logged in".
 *   - /ws/*   : never intercepted (WebSocket upgrades are not cacheable and must
 *     reach the backend untouched).
 *
 * The cache name is version-stamped.  Bump CACHE_VERSION whenever the static
 * shell assets change so returning clients pick up the new files (old caches are
 * purged on activate).
 */

const CACHE_VERSION = 'v1';
const CACHE_NAME = `aqualink-shell-${CACHE_VERSION}`;

// Core app-shell assets to precache so the UI boots offline.  Kept deliberately
// small and tolerant of individual failures (see install) — runtime caching
// below fills in everything else on first use.
const PRECACHE_URLS = [
  '/',
  '/index.html',
  '/manifest.json',
  '/favicon.svg',
  '/styles/app.css',
  '/styles/components.css',
  '/vendor/alpine.min.js',
  '/vendor/chart.umd.min.js',
  '/icons/icon-192.png',
  '/icons/icon-512.png',
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    (async () => {
      const cache = await caches.open(CACHE_NAME);
      // Cache entries individually so one missing optional asset does not abort
      // the whole install (addAll is all-or-nothing).
      await Promise.allSettled(PRECACHE_URLS.map((url) => cache.add(url)));
      await self.skipWaiting();
    })(),
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    (async () => {
      const names = await caches.keys();
      await Promise.all(
        names
          .filter((name) => name.startsWith('aqualink-shell-') && name !== CACHE_NAME)
          .map((name) => caches.delete(name)),
      );
      await self.clients.claim();
    })(),
  );
});

self.addEventListener('fetch', (event) => {
  const { request } = event;

  // Only ever handle GET; let everything else go straight to the network.
  if (request.method !== 'GET') {
    return;
  }

  const url = new URL(request.url);

  // Same-origin only for our caching logic; cross-origin (e.g. Google Fonts)
  // is left to the browser's own handling.
  if (url.origin !== self.location.origin) {
    return;
  }

  // Never intercept WebSocket endpoints or the live API.
  if (url.pathname.startsWith('/ws')) {
    return;
  }
  if (url.pathname.startsWith('/api')) {
    // Network only — do not consult or populate the cache.
    event.respondWith(fetch(request));
    return;
  }

  // App-shell navigations: try the network, fall back to the cached shell so
  // the SPA still loads offline.
  if (request.mode === 'navigate') {
    event.respondWith(
      (async () => {
        try {
          return await fetch(request);
        } catch {
          const cache = await caches.open(CACHE_NAME);
          const cached = (await cache.match(request)) || (await cache.match('/index.html'));
          return cached || Response.error();
        }
      })(),
    );
    return;
  }

  // Static assets: cache first, then network (populating the cache on a miss).
  event.respondWith(
    (async () => {
      const cache = await caches.open(CACHE_NAME);
      const cached = await cache.match(request);
      if (cached) {
        return cached;
      }
      try {
        const response = await fetch(request);
        // Only cache successful, basic (same-origin) responses.
        if (response && response.ok && response.type === 'basic') {
          cache.put(request, response.clone());
        }
        return response;
      } catch (err) {
        // Offline and not cached — surface the network failure.
        throw err;
      }
    })(),
  );
});
