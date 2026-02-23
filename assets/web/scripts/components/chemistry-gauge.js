/**
 * Chemistry Gauge — Semi-circular SVG arc gauge component
 *
 * Three-tier bands: Good / Okay / Bad
 * Configurable via localStorage key 'chemistryBands'.
 */
function chemistryGauge(type) {
    const defaults = {
        ph: {
            label: 'pH', min: 6.5, max: 8.5, unit: '', decimals: 1,
            goodMin: 7.4, goodMax: 7.6,
            okayMin: 7.2, okayMax: 7.8,
            badMin:  7.0, badMax:  8.0
        },
        orp: {
            label: 'ORP', min: 400, max: 900, unit: ' mV', decimals: 0,
            goodMin: 700, goodMax: 750,
            okayMin: 650, okayMax: 700,
            badMin:  650, badMax:  800
        },
        salt: {
            label: 'Salt', min: 0, max: 6000, unit: ' ppm', decimals: 0,
            goodMin: 3500, goodMax: 4000,
            okayMin: 2700, okayMax: 3500,
            badMin:  2700, badMax:  4500
        }
    };

    // Load overrides from localStorage
    const stored = JSON.parse(localStorage.getItem('chemistryBands') || '{}');
    const overrides = stored[type] || {};
    const cfg = { ...(defaults[type] || defaults.ph), ...overrides };

    return {
        cfg,

        get value() {
            const store = Alpine.store('pool');
            let raw;
            switch (type) {
                case 'ph':   raw = store.ph; break;
                case 'orp':  raw = store.orp; break;
                case 'salt': raw = store.saltPpm; break;
                default:     return '--';
            }
            // Treat 0 as unknown — real sensors never report exactly 0
            if (raw === 0 || raw === '0' || raw === '0.0') return '--';
            return raw;
        },

        get numericValue() {
            const v = parseFloat(this.value);
            return isNaN(v) ? null : v;
        },

        get displayValue() {
            if (this.numericValue === null) return '--';
            return this.numericValue.toFixed(cfg.decimals) + cfg.unit;
        },

        get percentage() {
            if (this.numericValue === null) return 0;
            return Math.max(0, Math.min(100,
                ((this.numericValue - cfg.min) / (cfg.max - cfg.min)) * 100
            ));
        },

        get dashOffset() {
            // Arc circumference for r=45, 180deg arc = PI * 45 = ~141.37
            const circumference = 141.37;
            return circumference - (this.percentage / 100) * circumference;
        },

        /** Three-tier band: 'good', 'okay', or 'bad' */
        get band() {
            const v = this.numericValue;
            if (v === null) return null;
            if (v >= cfg.goodMin && v <= cfg.goodMax) return 'good';
            if (v >= cfg.okayMin && v <= cfg.okayMax) return 'okay';
            return 'bad';
        },

        get statusColor() {
            switch (this.band) {
                case 'good': return 'var(--gauge-good)';
                case 'okay': return 'var(--gauge-warn)';
                case 'bad':  return 'var(--gauge-bad)';
                default:     return 'var(--text-muted)';
            }
        },

        get statusLabel() {
            switch (this.band) {
                case 'good': return 'Good';
                case 'okay': return 'Okay';
                case 'bad':  return 'Bad';
                default:     return '';
            }
        }
    };
}
