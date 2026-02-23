/**
 * Pool Store — Equipment state (temperatures, chemistry, buttons, system status)
 *
 * REST API field mapping:
 *   /api/equipment          → temperatures, chemistry, buttons, devices, stats, version
 *   /api/equipment/buttons  → { buttons: [{ id, label, status }] }
 *   /api/equipment/version  → { fields: [{ label, value }], model_number, fw_revision }
 *   /api/version            → { software_version: { name, version, description, homepage }, git_info: { ... } }
 *
 * WebSocket payload mapping:
 *   TemperatureUpdate → { pool_temp, spa_temp, air_temp }  (string values from Localised formatter)
 *   ChemistryUpdate   → { ph, orp, salt_level }            (numeric values)
 *   SystemStatusChange → serialised IStatus
 *   ButtonStateChange  → { button_id, status, label? }
 */
document.addEventListener('alpine:init', () => {
    Alpine.store('pool', {
        poolTemp: '--',
        spaTemp: '--',
        airTemp: '--',
        poolSetpoint: '--',
        spaSetpoint: '--',
        ph: '--',
        orp: '--',
        saltPpm: '--',

        // Per-value timestamps for freshness tracking
        _timestamps: {},
        _stalenessThreshold: 60,  // seconds

        buttons: [],
        commandStates: {},   // { [buttonId|'setpoint:pool'|'setpoint:spa']: { state, timer } }
        systemStatus: 'unknown',
        equipmentVersion: [],
        softwareVersion: '--',
        gitCommit: '',
        gitDate: '',
        gitUncommitted: false,
        projectName: '',
        projectDescription: '',
        projectHomepage: '',
        lastUpdate: null,
        buttonsLoading: true,

        async fetchInitial() {
            await Promise.all([
                this._fetchEquipment(),
                this._fetchButtons(),
                this._fetchVersion()
            ]);
        },

        _formatTemp(val) {
            if (val == null) return '--';
            if (typeof val === 'object' && val.celsius != null) return Math.round(val.celsius) + '\u00B0C';
            return String(val);
        },

        _touch(field) {
            this._timestamps = { ...this._timestamps, [field]: Date.now() };
        },

        age(field) {
            const ts = this._timestamps[field];
            if (!ts) return null;
            return Math.floor((Date.now() - ts) / 1000);
        },

        isStale(field) {
            const a = this.age(field);
            return a !== null && a > this._stalenessThreshold;
        },

        async _fetchEquipment() {
            try {
                const resp = await fetch('/api/equipment');
                const data = await resp.json();
                if (!data || Object.keys(data).length === 0) return;

                // Temperatures
                if (data.temperatures) {
                    this.poolTemp = this._formatTemp(data.temperatures.pool);
                    this.spaTemp = this._formatTemp(data.temperatures.spa);
                    this.airTemp = this._formatTemp(data.temperatures.air);
                    if (data.temperatures.pool_setpoint) this.poolSetpoint = data.temperatures.pool_setpoint;
                    if (data.temperatures.spa_setpoint) this.spaSetpoint = data.temperatures.spa_setpoint;
                }

                // Chemistry — all under 'chemistry' now (filter 0 as unknown)
                if (data.chemistry) {
                    if (data.chemistry.ph != null && data.chemistry.ph !== '' && data.chemistry.ph !== 0) this.ph = data.chemistry.ph;
                    if (data.chemistry.orp != null && data.chemistry.orp !== '' && data.chemistry.orp !== 0) this.orp = data.chemistry.orp;
                    if (data.chemistry.salt_in_ppm != null && data.chemistry.salt_in_ppm !== '' && data.chemistry.salt_in_ppm !== 0) this.saltPpm = data.chemistry.salt_in_ppm;
                }

                // Equipment version — use generic fields array if available
                if (data.version) {
                    if (Array.isArray(data.version.fields) && data.version.fields.length > 0) {
                        this.equipmentVersion = data.version.fields.filter(f => f.value);
                    } else {
                        // Back-compat: build fields from flat keys
                        const fields = [];
                        if (data.version.model_number) fields.push({ label: 'Model', value: data.version.model_number });
                        if (data.version.fw_revision) fields.push({ label: 'Revision', value: data.version.fw_revision });
                        this.equipmentVersion = fields;
                    }
                }

                // Seed diagnostics stats from initial load
                if (data.stats) {
                    Alpine.store('stats').handleEvent({
                        type: 'StatisticsUpdate',
                        payload: data.stats
                    });
                }

                this.systemStatus = 'operational';

                // Update system store if pool configuration is known
                if (data.configuration && data.configuration.pool_configuration &&
                    data.configuration.pool_configuration !== 'Unknown') {
                    const sys = Alpine.store('system');
                    if (sys.backendState === 'starting') {
                        sys._prevBackendState = sys.backendState;
                        sys.backendState = 'ready';
                    }
                    sys.poolConfiguration = data.configuration.pool_configuration;
                }
            } catch (e) {
                console.error('Failed to fetch equipment:', e);
            }
        },

        async _fetchButtons() {
            this.buttonsLoading = true;
            try {
                const resp = await fetch('/api/equipment/buttons');
                const data = await resp.json();
                if (data && Array.isArray(data.buttons)) {
                    this.buttons = data.buttons;
                } else if (data && data.buttons === null) {
                    setTimeout(() => this._fetchButtons(), 5000);
                    return;
                }
            } catch (e) {
                console.error('Failed to fetch buttons:', e);
            }
            this.buttonsLoading = false;
        },

        async _fetchVersion() {
            try {
                const resp = await fetch('/api/version');
                const data = await resp.json();
                if (data.software_version) {
                    this.softwareVersion = data.software_version.version || '--';
                    this.projectName = data.software_version.name || '';
                    this.projectDescription = data.software_version.description || '';
                    this.projectHomepage = data.software_version.homepage || '';
                }
                if (data.git_info) {
                    this.gitCommit = data.git_info.commit_sha1 || '';
                    this.gitDate = data.git_info.commit_date || '';
                    this.gitUncommitted = data.git_info.uncommitted_changes || false;
                }
                // Forward server timing to system store
                const sys = Alpine.store('system');
                if (data.server_start_time) sys.serverStartTime = data.server_start_time;
                if (data.uptime_seconds != null) sys.uptimeSeconds = data.uptime_seconds;
            } catch (e) {
                console.error('Failed to fetch version:', e);
            }
        },

        handleEvent(msg) {
            if (!msg || !msg.type) return;
            this.lastUpdate = new Date();

            switch (msg.type) {
                case 'TemperatureUpdate':
                    if (msg.payload) {
                        if (msg.payload.pool_temp != null) { this.poolTemp = this._formatTemp(msg.payload.pool_temp); this._touch('poolTemp'); }
                        if (msg.payload.spa_temp != null) { this.spaTemp = this._formatTemp(msg.payload.spa_temp); this._touch('spaTemp'); }
                        if (msg.payload.air_temp != null) { this.airTemp = this._formatTemp(msg.payload.air_temp); this._touch('airTemp'); }
                        if (msg.payload.pool_setpoint != null) this.poolSetpoint = msg.payload.pool_setpoint;
                        if (msg.payload.spa_setpoint != null) this.spaSetpoint = msg.payload.spa_setpoint;
                    }
                    break;

                case 'ChemistryUpdate':
                    if (msg.payload) {
                        if (msg.payload.ph != null && msg.payload.ph !== 0) { this.ph = msg.payload.ph; this._touch('ph'); }
                        if (msg.payload.orp != null && msg.payload.orp !== 0) { this.orp = msg.payload.orp; this._touch('orp'); }
                        if (msg.payload.salt_level != null && msg.payload.salt_level !== 0) { this.saltPpm = msg.payload.salt_level; this._touch('saltPpm'); }
                    }
                    break;

                case 'SystemStatusChange':
                    this.systemStatus = msg.payload?.status_type || 'unknown';
                    if (this.systemStatus === 'Normal' && this.buttons.length === 0) {
                        this._fetchButtons();
                    }
                    break;

                case 'ButtonStateChange':
                    if (msg.payload?.button_id != null) {
                        const idx = this.buttons.findIndex(b => b.id === msg.payload.button_id);
                        if (idx !== -1) {
                            const updates = { status: msg.payload.status };
                            if (msg.payload.label) updates.label = msg.payload.label;
                            this.buttons[idx] = { ...this.buttons[idx], ...updates };
                        } else {
                            // New button discovered — re-fetch to get full data (device_type, etc.)
                            this._fetchButtons();
                        }
                    }
                    break;
            }
        },

        _setCommandState(key, state) {
            const prev = this.commandStates[key];
            if (prev?.timer) clearTimeout(prev.timer);

            let timer = null;
            if (state !== 'sending') {
                timer = setTimeout(() => {
                    delete this.commandStates[key];
                    this.commandStates = { ...this.commandStates };
                }, 3000);
            } else {
                // Timeout for in-flight commands
                timer = setTimeout(() => {
                    if (this.commandStates[key]?.state === 'sending') {
                        this.commandStates[key] = { state: 'timedout' };
                        this.commandStates = { ...this.commandStates };
                        setTimeout(() => {
                            delete this.commandStates[key];
                            this.commandStates = { ...this.commandStates };
                        }, 3000);
                    }
                }, 5000);
            }

            this.commandStates = { ...this.commandStates, [key]: { state, timer } };
        },

        getCommandState(key) {
            return this.commandStates[key]?.state || null;
        },

        async adjustSetpoint(target, delta) {
            const key = `setpoint:${target}`;
            if (!Alpine.store('system').commandsEnabled) {
                this._setCommandState(key, 'blocked');
                return;
            }

            try {
                const current = target === 'pool' ? this.poolSetpoint : this.spaSetpoint;
                const currentCelsius = typeof current === 'object' ? current.celsius : parseFloat(current);
                if (isNaN(currentCelsius)) return;
                const newTemp = currentCelsius + delta;
                const body = {};
                body[target] = newTemp;

                this._setCommandState(key, 'sending');

                const resp = await fetch('/api/equipment/setpoints', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(body)
                });

                if (resp.ok) {
                    const data = await resp.json();
                    if (data) {
                        if (target === 'pool' && typeof this.poolSetpoint === 'object') {
                            this.poolSetpoint = { ...this.poolSetpoint, celsius: newTemp };
                        } else if (target === 'spa' && typeof this.spaSetpoint === 'object') {
                            this.spaSetpoint = { ...this.spaSetpoint, celsius: newTemp };
                        }
                    }
                    this._setCommandState(key, 'applied');
                } else {
                    this._setCommandState(key, 'rejected');
                }
            } catch (e) {
                console.error('Failed to adjust setpoint:', e);
                this._setCommandState(key, 'rejected');
            }
        },

        async toggleButton(id) {
            if (!Alpine.store('system').commandsEnabled) {
                this._setCommandState(id, 'blocked');
                return;
            }

            try {
                this._setCommandState(id, 'sending');

                const resp = await fetch(`/api/equipment/buttons/${id}`, { method: 'POST' });

                if (resp.ok) {
                    const data = await resp.json();
                    if (data) {
                        const idx = this.buttons.findIndex(b => b.id === id);
                        if (idx !== -1) {
                            this.buttons[idx] = { ...this.buttons[idx], status: data.status };
                        }
                    }
                    this._setCommandState(id, 'applied');
                } else {
                    this._setCommandState(id, 'rejected');
                }
            } catch (e) {
                console.error('Failed to toggle button:', e);
                this._setCommandState(id, 'rejected');
            }
        }
    });
});
