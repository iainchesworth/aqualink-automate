/**
 * Dashboard View — Component logic for the main dashboard
 */
function dashboardView() {
    return {
        get lastUpdateText() {
            const lu = Alpine.store('pool').lastUpdate;
            if (!lu) return '';
            return lu.toLocaleTimeString();
        }
    };
}
