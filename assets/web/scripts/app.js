/**
 * App — Alpine.js init and hash router
 *
 * Each view may depend on data that changes while it's not visible.
 * When a view becomes active, we trigger a lightweight refresh to
 * ensure stale data doesn't persist (e.g. system went Starting → Ready
 * while the user was on the diagnostics tab).
 *
 * Debounce prevents redundant fetches on rapid tab switching.
 */

// Interval (ms) for the WS-down fallback poll of equipment data.
const EQUIPMENT_FALLBACK_POLL_MS = 30000;

function app() {
    return {
        route: 'dashboard',
        _prevRoute: null,
        _lastRefresh: 0,
        _refreshDebounceMs: 2000,

        _started: false,

        init() {
            // Set initial route from hash
            this.route = this._getRouteFromHash();

            // Listen for hash changes
            window.addEventListener('hashchange', () => {
                this._prevRoute = this.route;
                this.route = this._getRouteFromHash();
                this._onRouteActivated(this.route);
            });

            // Gate data fetches + WebSocket on the auth check: with no token
            // configured this resolves immediately (200) and behaves as before;
            // with a token it shows the login overlay until the user authenticates.
            window.addEventListener('auth:ready', () => this._startApp(), { once: true });
            Alpine.store('auth').check().then((ok) => {
                if (ok) this._startApp();
            });
        },

        // Begin live operation. Idempotent: runs once whether triggered by an
        // already-valid session or a successful login.
        _startApp() {
            if (this._started) return;
            this._started = true;

            Alpine.store('pool').fetchInitial();
            Alpine.store('ws').connectEquipment();

            // Periodic fallback: re-fetch equipment data when WS is down
            setInterval(() => {
                if (!Alpine.store('ws').connected) {
                    Alpine.store('pool')._fetchEquipment();
                }
            }, EQUIPMENT_FALLBACK_POLL_MS);
        },

        _getRouteFromHash() {
            const hash = window.location.hash.replace('#', '');
            const valid = ['dashboard', 'diagnostics', 'settings', 'about'];
            return valid.includes(hash) ? hash : 'dashboard';
        },

        /**
         * Called when a view becomes active. Triggers a lightweight data
         * refresh so the view shows current data, not stale initial state.
         */
        _onRouteActivated(route) {
            const now = Date.now();
            if (now - this._lastRefresh < this._refreshDebounceMs) return;
            this._lastRefresh = now;

            switch (route) {
                case 'dashboard':
                    // Re-fetch equipment state and buttons to pick up
                    // any changes that occurred while on another tab
                    Alpine.store('pool')._fetchEquipment();
                    Alpine.store('pool')._fetchButtons();
                    break;

                case 'diagnostics':
                    // Stats WS lifecycle is managed by the diagnostics view
                    // (initChart/destroyChart). But we re-fetch version info
                    // so uptime is current.
                    Alpine.store('pool')._fetchVersion();
                    break;
            }
        },

        navigate(route) {
            window.location.hash = '#' + route;
        },

        isRoute(route) {
            return this.route === route;
        }
    };
}
