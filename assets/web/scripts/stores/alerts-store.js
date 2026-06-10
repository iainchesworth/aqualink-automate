/**
 * Alerts Store — tracks active fault conditions raised by the backend
 * AlertMonitor (WS3) and surfaces them as toasts + a persistent nav badge.
 *
 * WebSocket payload mapping (/ws/equipment):
 *   AlertTransition -> { condition, state: "raised"|"cleared", ts, detail }
 */

// Friendly labels for the known condition keys (falls back to the raw key).
const ALERT_LABELS = {
    chlorinator_fault: 'Chlorinator fault',
    salt_low: 'Salt low',
    service_mode: 'Service mode',
    serial_comms_loss: 'Serial comms lost',
};

document.addEventListener('alpine:init', () => {
    Alpine.store('alerts', {
        // condition key -> { detail, ts }
        active: {},

        label(condition) {
            return ALERT_LABELS[condition] || condition;
        },

        get count() {
            return Object.keys(this.active).length;
        },

        get hasAlerts() {
            return this.count > 0;
        },

        // List form for rendering (sorted by key for stable display).
        get list() {
            return Object.keys(this.active)
                .sort()
                .map((key) => ({ condition: key, label: this.label(key), ...this.active[key] }));
        },

        handleEvent(msg) {
            if (!msg || msg.type !== 'AlertTransition') return;

            const p = msg.payload || {};
            const condition = p.condition;
            if (!condition) return;

            if (p.state === 'raised') {
                // Reassign the object so Alpine's reactivity sees the change.
                this.active = { ...this.active, [condition]: { detail: p.detail || '', ts: p.ts || 0 } };
                Alpine.store('toast').show(`${this.label(condition)}: ${p.detail || 'fault detected'}`, 'error', 8000);
            } else {
                const next = { ...this.active };
                delete next[condition];
                this.active = next;
                Alpine.store('toast').show(`${this.label(condition)} cleared`, 'info', 4000);
            }
        },
    });
});
