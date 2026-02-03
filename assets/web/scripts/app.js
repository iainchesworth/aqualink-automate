/**
 * App — Alpine.js init and hash router
 */
function app() {
    return {
        route: 'dashboard',

        init() {
            // Set initial route from hash
            this.route = this._getRouteFromHash();

            // Listen for hash changes
            window.addEventListener('hashchange', () => {
                this.route = this._getRouteFromHash();
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
            const valid = ['dashboard', 'diagnostics', 'about'];
            return valid.includes(hash) ? hash : 'dashboard';
        },

        navigate(route) {
            window.location.hash = '#' + route;
        },

        isRoute(route) {
            return this.route === route;
        }
    };
}
