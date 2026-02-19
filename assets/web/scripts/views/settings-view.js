/**
 * Settings View — Chemistry target range configuration
 *
 * Persists to localStorage key 'chemistryBands'.
 * Each gauge type (ph, orp, salt) has: goodMin/Max, okayMin/Max, badMin/Max.
 */
function settingsView() {
    const defaults = {
        ph: {
            label: 'pH', goodMin: 7.4, goodMax: 7.6,
            okayMin: 7.2, okayMax: 7.8, badMin: 7.0, badMax: 8.0
        },
        orp: {
            label: 'ORP (mV)', goodMin: 700, goodMax: 750,
            okayMin: 650, okayMax: 700, badMin: 650, badMax: 800
        },
        salt: {
            label: 'Salt (ppm)', goodMin: 3500, goodMax: 4000,
            okayMin: 2700, okayMax: 3500, badMin: 2700, badMax: 4500
        }
    };

    const stored = JSON.parse(localStorage.getItem('chemistryBands') || '{}');

    // Merge stored values over defaults for each gauge
    const values = {};
    for (const [key, def] of Object.entries(defaults)) {
        values[key] = { ...def, ...(stored[key] || {}) };
    }

    return {
        gauges: [
            { key: 'ph', label: 'pH', step: 0.1 },
            { key: 'orp', label: 'ORP (mV)', step: 10 },
            { key: 'salt', label: 'Salt (ppm)', step: 100 }
        ],

        values,

        updateValue(gaugeKey, field, rawValue) {
            const num = parseFloat(rawValue);
            if (isNaN(num)) return;
            this.values[gaugeKey][field] = num;
            this._save();
        },

        resetGauge(gaugeKey) {
            const def = defaults[gaugeKey];
            this.values[gaugeKey] = { ...def };
            this._save();
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
            localStorage.setItem('chemistryBands', JSON.stringify(toStore));
        }
    };
}
