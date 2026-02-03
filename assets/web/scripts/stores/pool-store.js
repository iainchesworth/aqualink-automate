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
 *   ButtonStateChange  → { button_id, status }
 */
document.addEventListener('alpine:init', () => {
    Alpine.store('pool', {
        poolTemp: '--',
        spaTemp: '--',
        airTemp: '--',
        ph: '--',
        orp: '--',
        saltPpm: '--',
        buttons: [],
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

        async _fetchEquipment() {
            try {
                const resp = await fetch('/api/equipment');
                const data = await resp.json();
                if (!data || Object.keys(data).length === 0) return;

                // Temperatures
                if (data.temperatures) {
                    this.poolTemp = data.temperatures.pool ?? '--';
                    this.spaTemp = data.temperatures.spa ?? '--';
                    this.airTemp = data.temperatures.air ?? '--';
                }

                // Chemistry — all under 'chemistry' now
                if (data.chemistry) {
                    if (data.chemistry.ph != null && data.chemistry.ph !== '') this.ph = data.chemistry.ph;
                    if (data.chemistry.orp != null && data.chemistry.orp !== '') this.orp = data.chemistry.orp;
                    if (data.chemistry.salt_in_ppm != null && data.chemistry.salt_in_ppm !== '') this.saltPpm = data.chemistry.salt_in_ppm;
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
                        if (msg.payload.pool_temp != null) this.poolTemp = msg.payload.pool_temp;
                        if (msg.payload.spa_temp != null) this.spaTemp = msg.payload.spa_temp;
                        if (msg.payload.air_temp != null) this.airTemp = msg.payload.air_temp;
                    }
                    break;

                case 'ChemistryUpdate':
                    if (msg.payload) {
                        if (msg.payload.ph != null) this.ph = msg.payload.ph;
                        if (msg.payload.orp != null) this.orp = msg.payload.orp;
                        if (msg.payload.salt_level != null) this.saltPpm = msg.payload.salt_level;
                    }
                    break;

                case 'SystemStatusChange':
                    this.systemStatus = msg.payload?.status || 'unknown';
                    if (this.systemStatus === 'operational' && this.buttons.length === 0) {
                        this._fetchButtons();
                    }
                    break;

                case 'ButtonStateChange':
                    if (msg.payload?.button_id != null) {
                        const idx = this.buttons.findIndex(b => b.id === msg.payload.button_id);
                        if (idx !== -1) {
                            this.buttons[idx] = { ...this.buttons[idx], status: msg.payload.status };
                        }
                    }
                    break;
            }
        },

        async toggleButton(id) {
            try {
                const resp = await fetch(`/api/equipment/buttons/${id}`, { method: 'POST' });
                const data = await resp.json();
                if (data) {
                    const idx = this.buttons.findIndex(b => b.id === id);
                    if (idx !== -1) {
                        this.buttons[idx] = { ...this.buttons[idx], status: data.status };
                    }
                }
            } catch (e) {
                console.error('Failed to toggle button:', e);
            }
        }
    });
});
