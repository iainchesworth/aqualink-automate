/**
 * Settings View — Chemistry target range configuration
 *
 * Persists to localStorage (see ui-constants.js CHEMISTRY_BANDS_KEY).
 * Each gauge type (ph, orp, salt) has: goodMin/Max, okayMin/Max, badMin/Max.
 *
 * Band defaults + the localStorage key are the single source of truth in
 * scripts/config/ui-constants.js (shared with chemistry-gauge.js).
 */
function settingsView() {
    // Shared band defaults + key (fall back to local copies if the shared
    // config script is not loaded).
    const ui = (typeof window !== 'undefined' && window.AquaUI) || {};
    const bandsKey = ui.CHEMISTRY_BANDS_KEY || 'chemistryBands';
    const bandDefaults = (ui.CHEMISTRY_BAND_DEFAULTS) || {
        ph:   { goodMin: 7.4,  goodMax: 7.6,  okayMin: 7.2,  okayMax: 7.8,  badMin: 7.0,  badMax: 8.0 },
        orp:  { goodMin: 700,  goodMax: 750,  okayMin: 650,  okayMax: 700,  badMin: 650,  badMax: 800 },
        salt: { goodMin: 3500, goodMax: 4000, okayMin: 2700, okayMax: 3500, badMin: 2700, badMax: 4500 }
    };

    const stored = JSON.parse(localStorage.getItem(bandsKey) || '{}');

    // Merge stored values over defaults for each gauge
    const values = {};
    for (const [key, def] of Object.entries(bandDefaults)) {
        values[key] = { ...def, ...(stored[key] || {}) };
    }

    return {
        gauges: [
            { key: 'ph', label: 'pH', step: 0.1 },
            { key: 'orp', label: 'ORP (mV)', step: 10 },
            { key: 'salt', label: 'Salt (ppm)', step: 100 }
        ],

        values,

        // Per-gauge validation messages (empty string = valid). Surfaced in
        // the UI; also used to gate persistence.
        errors: {},

        updateValue(gaugeKey, field, rawValue) {
            const num = parseFloat(rawValue);
            if (isNaN(num)) return;
            this.values[gaugeKey][field] = num;
            // Validate this gauge; only persist when the band ranges are sane
            // so an inverted/overlapping edit never reaches the gauge component.
            if (this._validateGauge(gaugeKey)) {
                this._save();
            }
        },

        resetGauge(gaugeKey) {
            const def = bandDefaults[gaugeKey];
            this.values[gaugeKey] = { ...def };
            this.errors[gaugeKey] = '';
            this._save();
        },

        /**
         * Validate a single gauge's band ranges. Each tier must be ordered
         * (Min <= Max) so an inverted range is never persisted.
         *
         * NOTE: enclosure rules ("okay encloses good", "bad encloses okay")
         * are deliberately NOT enforced — the shipped defaults (and the gauge's
         * priority-order band lookup in chemistry-gauge.js) treat the three
         * tiers as a precedence sequence, not nested intervals (e.g. salt
         * good=3500-4000, okay=2700-3500 sit side by side). Enforcing
         * enclosure would reject the defaults themselves.
         *
         * Sets this.errors[gaugeKey] and returns true when valid.
         */
        _validateGauge(gaugeKey) {
            const v = this.values[gaugeKey];
            let error = '';
            if (v.goodMin > v.goodMax || v.okayMin > v.okayMax || v.badMin > v.badMax) {
                error = 'Each tier minimum must be less than or equal to its maximum.';
            }
            this.errors[gaugeKey] = error;
            return error === '';
        },

        _save() {
            // Only persist the band fields, not label
            const toStore = {};
            for (const [key, val] of Object.entries(this.values)) {
                toStore[key] = {
                    goodMin: val.goodMin, goodMax: val.goodMax,
                    okayMin: val.okayMin, okayMax: val.okayMax,
                    badMin: val.badMin, badMax: val.badMax
                };
            }
            localStorage.setItem(bandsKey, JSON.stringify(toStore));
        }
    };
}
