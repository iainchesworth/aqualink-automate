/**
 * Pool Store — Equipment state (temperatures, chemistry, buttons, system status)
 *
 * REST API field mapping:
 *   /api/equipment          → temperatures, chemistry, buttons, devices, stats, version
 *                             chemistry = { salt_ppm, orp_mv, ph,
 *                                           chlorinator: { generating_percent, duty_cycle,
 *                                                          pool_setpoint_percent, spa_setpoint_percent,
 *                                                          setpoint_percent, status, health } }
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

// How long a terminal command state ('applied'/'rejected'/'blocked'/'timedout')
// stays visible before it auto-clears, and how long an in-flight 'sending'
// command waits before it is considered timed out.
const COMMAND_STATE_CLEAR_MS = 3000;
const COMMAND_SENDING_TIMEOUT_MS = 5000;
// After the server accepts a button command (HTTP 200), the equipment has NOT
// changed yet — the command still has to traverse the RS-485 bus, the controller
// has to actuate the relay, and the new state has to be decoded back and pushed as
// a 'ButtonStateChange'. We hold the command 'sending' until that authoritative
// confirmation arrives, or until this (longer) window elapses and we surface an
// honest 'timedout'. Sized for the controller round-trip (the RSSA status-poll
// rotation / a OneTouch menu re-scrape); tune if confirmations routinely arrive
// later than this on real hardware.
const COMMAND_CONFIRM_TIMEOUT_MS = 20000;

// Re-fetch interval when the buttons endpoint reports it is not yet ready.
const BUTTONS_RETRY_MS = 5000;

// Normalise the several system-status vocabularies (REST 'operational',
// WS SystemStatusChange status_type like 'Normal'/'Initializing', and the
// initial 'unknown') down to one human-readable label for display.
function _normaliseSystemStatus(raw) {
    switch (String(raw || '').toLowerCase()) {
        case 'operational':
        case 'normal':
            return 'Operational';
        case 'initializing':
        case 'starting':
            return 'Starting';
        case 'service':
        case 'service_mode':
            return 'Service Mode';
        case '':
        case 'unknown':
            return 'Unknown';
        default:
            return String(raw);
    }
}

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

        // Chlorinator (SWG) state — sourced from the nested chemistry.chlorinator
        // block of /api/equipment. Null/unknown until a chlorinator is discovered.
        chlorinatorPresent: false,
        swgGeneratingPercent: '--',
        swgDutyCycle: '--',
        // Configured output setpoint(s) — distinct from the instantaneous generating %.
        // swgSetpointPercent is the headline target for the active body; pool/spa are the
        // per-body configured values. A setpoint of 0 is valid (treated as known, not '--').
        swgSetpointPercent: '--',
        swgPoolSetpoint: '--',
        swgSpaSetpoint: '--',
        chlorinatorStatus: '--',
        chlorinatorHealth: '--',

        // Per-value timestamps for freshness tracking
        _timestamps: {},
        _stalenessThreshold: 60,  // seconds

        buttons: [],
        commandStates: {},   // { [buttonId|'setpoint:pool'|'setpoint:spa']: { state, timer } }
        systemStatus: 'Unknown',
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
            this._timestamps[field] = Date.now();
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

                // Chemistry — nested structure: { salt_ppm, orp_mv, ph,
                // chlorinator: { generating_percent, duty_cycle, status, health } }.
                // ORP/pH are null when no sensor is present (placeholders today);
                // a value of 0 is also treated as unknown. The chlorinator block
                // is null until a SWG is discovered on the wire.
                if (data.chemistry) {
                    if (data.chemistry.ph != null && data.chemistry.ph !== '' && data.chemistry.ph !== 0) this.ph = data.chemistry.ph;
                    if (data.chemistry.orp_mv != null && data.chemistry.orp_mv !== '' && data.chemistry.orp_mv !== 0) this.orp = data.chemistry.orp_mv;
                    if (data.chemistry.salt_ppm != null && data.chemistry.salt_ppm !== '' && data.chemistry.salt_ppm !== 0) this.saltPpm = data.chemistry.salt_ppm;

                    const swg = data.chemistry.chlorinator;
                    if (swg != null) {
                        this.chlorinatorPresent = true;
                        this.swgGeneratingPercent = (swg.generating_percent != null) ? swg.generating_percent : '--';
                        this.swgDutyCycle = (swg.duty_cycle != null) ? swg.duty_cycle : '--';
                        // Setpoints may legitimately be 0, so test against null only.
                        this.swgSetpointPercent = (swg.setpoint_percent != null) ? swg.setpoint_percent : '--';
                        this.swgPoolSetpoint = (swg.pool_setpoint_percent != null) ? swg.pool_setpoint_percent : '--';
                        this.swgSpaSetpoint = (swg.spa_setpoint_percent != null) ? swg.spa_setpoint_percent : '--';
                        this.chlorinatorStatus = swg.status || '--';
                        this.chlorinatorHealth = swg.health || '--';
                    } else {
                        this.chlorinatorPresent = false;
                    }
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

                this.systemStatus = _normaliseSystemStatus('operational');

                // Update system store if pool configuration is known
                if (data.configuration && data.configuration.pool_configuration &&
                    data.configuration.pool_configuration !== 'Unknown') {
                    const sys = Alpine.store('system');
                    sys.markReadyIfStarting();
                    sys.poolConfiguration = data.configuration.pool_configuration;
                }
            } catch (e) {
                console.error('Failed to fetch equipment:', e);
                Alpine.store('toast').show('Failed to load equipment data', 'error');
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
                    setTimeout(() => this._fetchButtons(), BUTTONS_RETRY_MS);
                    return;
                }
            } catch (e) {
                console.error('Failed to fetch buttons:', e);
                Alpine.store('toast').show('Failed to load equipment controls', 'error');
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
                Alpine.store('toast').show('Failed to load version info', 'warn');
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

                case 'SystemStatusChange': {
                    const statusType = msg.payload?.status_type;
                    this.systemStatus = _normaliseSystemStatus(statusType);
                    if (statusType === 'Normal' && this.buttons.length === 0) {
                        this._fetchButtons();
                    }
                    break;
                }

                case 'ButtonStateChange':
                    if (msg.payload?.button_id != null) {
                        const idx = this.buttons.findIndex(b => b.id === msg.payload.button_id);
                        if (idx !== -1) {
                            const updates = { status: msg.payload.status };
                            if (msg.payload.label) updates.label = msg.payload.label;
                            this.buttons[idx] = { ...this.buttons[idx], ...updates };
                            // If a command for this button was in flight, the controller has
                            // now reported the resulting state — confirm it as 'applied'
                            // (this is the authoritative success signal, not the HTTP 200).
                            if (this.commandStates[msg.payload.button_id]?.state === 'sending') {
                                this._setCommandState(msg.payload.button_id, 'applied');
                            }
                        } else {
                            // New button discovered — re-fetch to get full data (device_type, etc.)
                            this._fetchButtons();
                        }
                    }
                    break;
            }
        },

        // Clear a command state after COMMAND_STATE_CLEAR_MS and notify Alpine.
        _scheduleClear(key) {
            return setTimeout(() => this._clearCommandState(key), COMMAND_STATE_CLEAR_MS);
        },

        _clearCommandState(key) {
            const { [key]: _removed, ...rest } = this.commandStates;
            this.commandStates = rest;
        },

        _setCommandState(key, state, sendingTimeoutMs = COMMAND_SENDING_TIMEOUT_MS) {
            const prev = this.commandStates[key];
            if (prev?.timer) clearTimeout(prev.timer);

            let timer;
            if (state === 'sending') {
                // In-flight command: if it has not resolved by the timeout (either no
                // HTTP response, or — after a 200 — no authoritative ButtonStateChange
                // confirming the controller actually changed the equipment), surface a
                // distinct 'timedout' state (then auto-clear it).
                timer = setTimeout(() => {
                    if (this.commandStates[key]?.state === 'sending') {
                        console.warn(`Command '${key}' timed out after ${sendingTimeoutMs}ms`);
                        Alpine.store('toast').show('Command timed out — no confirmation received from the controller', 'warn');
                        this.commandStates = { ...this.commandStates, [key]: { state: 'timedout', timer: this._scheduleClear(key) } };
                    }
                }, sendingTimeoutMs);
            } else {
                // Terminal state ('applied'/'rejected'/'blocked'/'timedout'): auto-clear.
                timer = this._scheduleClear(key);
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
                // The backend quantizes setpoints to whole degrees (std::round in
                // webroute_equipment_setpoints.cpp); round the optimistic value the
                // same way so the UI never shows a fractional value the controller
                // will not honour.
                const newTemp = Math.round(currentCelsius + delta);
                const body = {};
                body[target] = newTemp;

                this._setCommandState(key, 'sending');

                const resp = await fetch('/api/equipment/setpoints', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(body)
                });

                if (!resp.ok) {
                    const reason = await this._readErrorReason(resp);
                    // A rejected command is an expected operational outcome (e.g.
                    // controller not ready), not a UI fault — warn, don't error.
                    console.warn(`Setpoint change rejected (HTTP ${resp.status}):`, reason);
                    this._setCommandState(key, 'rejected');
                    Alpine.store('toast').show(`Setpoint change failed — ${reason}`, 'error');
                    return;
                }

                // The POST always returns 200; the per-target result carries the
                // real outcome. Treat the command as applied only when the server
                // confirms status === 'success' for this target.
                let data = null;
                try { data = await resp.json(); } catch (_) { /* tolerate empty/non-JSON body */ }
                const targetResult = data?.[target];
                if (targetResult?.status === 'success') {
                    // Prefer the server's authoritative (quantized) celsius value.
                    const applied = targetResult.celsius != null ? targetResult.celsius : newTemp;
                    if (target === 'pool' && typeof this.poolSetpoint === 'object') {
                        this.poolSetpoint = { ...this.poolSetpoint, celsius: applied };
                    } else if (target === 'spa' && typeof this.spaSetpoint === 'object') {
                        this.spaSetpoint = { ...this.spaSetpoint, celsius: applied };
                    }
                    this._setCommandState(key, 'applied');
                } else {
                    console.warn(`Setpoint change for '${target}' was not applied:`, targetResult);
                    this._setCommandState(key, 'rejected');
                    Alpine.store('toast').show('Setpoint change was not applied by the controller', 'error');
                }
            } catch (e) {
                console.error('Failed to adjust setpoint:', e);
                this._setCommandState(key, 'rejected');
                Alpine.store('toast').show('Setpoint change failed — check connection', 'error');
            }
        },

        // Best-effort extraction of a human-readable failure reason from a
        // non-ok response (structured JSON {error|message} or plain text),
        // falling back to the HTTP status text.
        async _readErrorReason(resp) {
            try {
                const text = await resp.text();
                if (text) {
                    try {
                        const j = JSON.parse(text);
                        if (j && (j.error || j.message)) return String(j.error || j.message);
                    } catch (_) { /* not JSON — use raw text */ }
                    return text;
                }
            } catch (_) { /* body unreadable */ }
            return resp.statusText || `HTTP ${resp.status}`;
        },

        async toggleButton(id) {
            if (!Alpine.store('system').commandsEnabled) {
                this._setCommandState(id, 'blocked');
                return;
            }

            try {
                this._setCommandState(id, 'sending');

                const resp = await fetch(`/api/equipment/buttons/${id}`, { method: 'POST' });

                if (!resp.ok) {
                    const reason = await this._readErrorReason(resp);
                    // A rejected command is an expected operational outcome (e.g.
                    // controller not ready), not a UI fault — warn, don't error.
                    console.warn(`Command for button '${id}' rejected (HTTP ${resp.status}):`, reason);
                    this._setCommandState(id, 'rejected');
                    Alpine.store('toast').show(`Command failed — ${reason}`, 'error');
                    return;
                }

                // HTTP 200 means the controller ACCEPTED the command for transmission, NOT
                // that the equipment has changed yet. The response body's status is the
                // PRE-toggle state, so we deliberately do NOT paint it as the new status and
                // do NOT mark the command 'applied' here — doing so was the optimistic bug
                // that showed false success when a command silently did nothing. Instead we
                // keep the command in-flight ('sending') and wait for the authoritative
                // 'ButtonStateChange' WS event to confirm the real new state (-> 'applied').
                // If no confirmation arrives within COMMAND_CONFIRM_TIMEOUT_MS the command
                // honestly resolves to 'timedout'.
                try { await resp.json(); } catch (_) { /* body intentionally ignored */ }
                this._setCommandState(id, 'sending', COMMAND_CONFIRM_TIMEOUT_MS);
            } catch (e) {
                console.error('Failed to toggle button:', e);
                this._setCommandState(id, 'rejected');
                Alpine.store('toast').show('Command failed — check connection', 'error');
            }
        }
    });
});
