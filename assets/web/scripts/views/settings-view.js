/**
 * Settings View — user/admin preferences.
 *
 * Two groups:
 *   - Server-backed preferences (units, alert thresholds, webhook URL, history
 *     retention) loaded from and saved to /api/preferences (persisted on the
 *     server, shared across devices, and read live by the backend services).
 *   - Chemistry target ranges: still mirrored to localStorage (so the unchanged
 *     chemistry-gauge component keeps working) AND synced to the server under
 *     preferences.ui.chemistryBands so they survive a cache clear / new device.
 */
function settingsView() {
    const ui = (typeof window !== 'undefined' && window.AquaUI) || {};
    const bandsKey = ui.CHEMISTRY_BANDS_KEY || 'chemistryBands';
    const bandDefaults = (ui.CHEMISTRY_BAND_DEFAULTS) || {
        ph:   { goodMin: 7.4,  goodMax: 7.6,  okayMin: 7.2,  okayMax: 7.8,  badMin: 7.0,  badMax: 8.0 },
        orp:  { goodMin: 700,  goodMax: 750,  okayMin: 650,  okayMax: 700,  badMin: 650,  badMax: 800 },
        salt: { goodMin: 3500, goodMax: 4000, okayMin: 2700, okayMax: 3500, badMin: 2700, badMax: 4500 }
    };

    const stored = JSON.parse(localStorage.getItem(bandsKey) || '{}');
    const values = {};
    for (const [key, def] of Object.entries(bandDefaults)) {
        values[key] = { ...def, ...(stored[key] || {}) };
    }

    function bandsForStorage(vals) {
        const out = {};
        for (const [key, val] of Object.entries(vals)) {
            out[key] = {
                goodMin: val.goodMin, goodMax: val.goodMax,
                okayMin: val.okayMin, okayMax: val.okayMax,
                badMin: val.badMin, badMax: val.badMax
            };
        }
        return out;
    }

    return {
        gauges: [
            { key: 'ph', label: 'pH', step: 0.1 },
            { key: 'orp', label: 'ORP (mV)', step: 10 },
            { key: 'salt', label: 'Salt (ppm)', step: 100 }
        ],
        values,
        errors: {},

        // Server-backed preferences.
        prefs: {
            temperature_units: 'Celsius',
            salt_low_ppm: 2600,
            comms_timeout_seconds: 60,
            webhook_url: '',
            retention_days: 90,
        },
        prefsError: '',
        savedFlash: false,

        // Friendly display-name overrides keyed by canonical device label.
        labelOverrides: {},

        // Alpine auto-calls init() on the component.
        async init() {
            try {
                const resp = await fetch('/api/preferences');
                if (!resp.ok) { return; }
                const p = await resp.json();
                this.prefs.temperature_units = p.temperature_units || 'Celsius';
                this.prefs.salt_low_ppm = (p.alert && p.alert.salt_low_ppm) ?? 2600;
                this.prefs.comms_timeout_seconds = (p.alert && p.alert.comms_timeout_seconds) ?? 60;
                this.prefs.webhook_url = (p.alert && p.alert.webhook_url) || '';
                this.prefs.retention_days = (p.history && p.history.retention_days) ?? 90;
                this.labelOverrides = (p.label_overrides && typeof p.label_overrides === 'object') ? p.label_overrides : {};

                // Server-stored chemistry bands take precedence (cross-device).
                if (p.ui && p.ui.chemistryBands) {
                    for (const [key, def] of Object.entries(bandDefaults)) {
                        this.values[key] = { ...def, ...(p.ui.chemistryBands[key] || {}) };
                    }
                    localStorage.setItem(bandsKey, JSON.stringify(bandsForStorage(this.values)));
                }
            } catch (_) { /* offline / disabled: keep defaults */ }
        },

        // ---- Chemistry bands (localStorage + server) ----
        updateValue(gaugeKey, field, rawValue) {
            const num = parseFloat(rawValue);
            if (isNaN(num)) return;
            this.values[gaugeKey][field] = num;
            if (this._validateGauge(gaugeKey)) {
                this._saveLocal();
                this._syncBandsToServer();
            }
        },

        resetGauge(gaugeKey) {
            this.values[gaugeKey] = { ...bandDefaults[gaugeKey] };
            this.errors[gaugeKey] = '';
            this._saveLocal();
            this._syncBandsToServer();
        },

        _validateGauge(gaugeKey) {
            const v = this.values[gaugeKey];
            let error = '';
            if (v.goodMin > v.goodMax || v.okayMin > v.okayMax || v.badMin > v.badMax) {
                error = 'Each tier minimum must be less than or equal to its maximum.';
            }
            this.errors[gaugeKey] = error;
            return error === '';
        },

        _saveLocal() {
            localStorage.setItem(bandsKey, JSON.stringify(bandsForStorage(this.values)));
        },

        _syncBandsToServer() {
            this._putPrefs({ ui: { chemistryBands: bandsForStorage(this.values) } });
        },

        // ---- Device display names ----
        // The canonical-labelled controllable devices come straight from the store.
        deviceButtons() {
            return (this.$store.pool && this.$store.pool.buttons) ? this.$store.pool.buttons : [];
        },

        async saveLabels() {
            // Drop blank entries so an empty field falls back to the canonical label.
            const cleaned = {};
            for (const [canonical, display] of Object.entries(this.labelOverrides)) {
                if (display && String(display).trim() !== '') { cleaned[canonical] = String(display).trim(); }
            }
            this.labelOverrides = cleaned;
            await this._putPrefs({ label_overrides: cleaned });
            // Refresh the dashboard so the new display names take effect immediately.
            if (this.$store.pool && typeof this.$store.pool._fetchButtons === 'function') {
                this.$store.pool._fetchButtons();
            }
        },

        // ---- Server-backed preferences ----
        async saveServerPrefs() {
            await this._putPrefs({
                temperature_units: this.prefs.temperature_units,
                alert: {
                    salt_low_ppm: Number(this.prefs.salt_low_ppm),
                    comms_timeout_seconds: Number(this.prefs.comms_timeout_seconds),
                    webhook_url: this.prefs.webhook_url || '',
                },
                history: { retention_days: Number(this.prefs.retention_days) },
            });
        },

        async _putPrefs(payload) {
            this.prefsError = '';
            try {
                const resp = await fetch('/api/preferences', {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload),
                });
                if (!resp.ok) {
                    let detail = '';
                    try { detail = await resp.text(); } catch (_) { /* ignore */ }
                    this.prefsError = `Save failed (${resp.status})${detail ? ': ' + detail : ''}`;
                    return;
                }
                this.savedFlash = true;
                setTimeout(() => { this.savedFlash = false; }, 1500);
            } catch (e) {
                this.prefsError = 'Save failed (network).';
            }
        },
    };
}
