// Progressive Web App service-worker registration.
//
// Service workers require a secure context.  Browsers already restrict
// registration to HTTPS and to localhost/127.0.0.1, so we simply attempt
// registration when the API is available and let the platform enforce the
// transport rule (an insecure non-localhost origin will reject it, which we
// log and otherwise ignore — the app continues to work without the SW).
(function registerServiceWorker() {
  if (!('serviceWorker' in navigator)) {
    return;
  }

  window.addEventListener('load', function onLoad() {
    navigator.serviceWorker.register('/sw.js').catch(function onError(err) {
      console.warn('Service worker registration failed:', err);
    });
  });
})();
