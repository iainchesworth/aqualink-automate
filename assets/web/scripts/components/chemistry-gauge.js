/**
 * Chemistry Gauge — Semi-circular SVG arc gauge component
 */
function chemistryGauge(type) {
    const configs = {
        ph:   { label: 'pH',       min: 6.8, max: 8.0, targetMin: 7.2, targetMax: 7.8, unit: '',    decimals: 1 },
        orp:  { label: 'ORP',      min: 400, max: 900, targetMin: 650, targetMax: 750, unit: ' mV', decimals: 0 },
        salt: { label: 'Salt',     min: 0,   max: 6000, targetMin: 2700, targetMax: 3400, unit: ' ppm', decimals: 0 }
    };

    const cfg = configs[type] || configs.ph;

    return {
        cfg,

        get value() {
            const store = Alpine.store('pool');
            switch (type) {
                case 'ph':   return store.ph;
                case 'orp':  return store.orp;
                case 'salt': return store.saltPpm;
                default:     return '--';
            }
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

        get statusColor() {
            if (this.numericValue === null) return 'var(--text-muted)';
            if (this.numericValue >= cfg.targetMin && this.numericValue <= cfg.targetMax) {
                return 'var(--gauge-good)';
            }
            const range = cfg.targetMax - cfg.targetMin;
            if (this.numericValue < cfg.targetMin - range * 0.5 || this.numericValue > cfg.targetMax + range * 0.5) {
                return 'var(--gauge-bad)';
            }
            return 'var(--gauge-warn)';
        }
    };
}
