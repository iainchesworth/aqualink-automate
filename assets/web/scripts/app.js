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
function app() {
    return {
        route: 'dashboard',
        _prevRoute: null,
        _lastRefresh: 0,
        _refreshDebounceMs: 2000,

        init() {
            // Set initial route from hash
            this.route = this._getRouteFromHash();

            // Listen for hash changes
            window.addEventListener('hashchange', () => {
                this._prevRoute = this.route;
                this.route = this._getRouteFromHash();
                this._onRouteActivated(this.route);
            });

            // Fetch initial data and connect WebSocket
            Alpine.store('pool').fetchInitial();
            Alpine.store('ws').connectEquipment();

            // Periodic fallback: re-fetch equipment data when WS is down
            setInterval(() => {
                if (!Alpine.store('ws').connected) {
                    Alpine.store('pool')._fetchEquipment();
                }
            }, 30000);
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
