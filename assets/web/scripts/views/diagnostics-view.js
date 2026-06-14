/**
 * Diagnostics View — Charts and stats component
 *
 * Chart.js instances and event listeners are stored outside Alpine's
 * reactive proxy to prevent infinite recursion when Chart.js
 * introspects its own proxied properties.
 */

// Private state kept outside Alpine
const _diag = {
    chart: null,
    statsListener: null,
    windowSeconds: 60,
    emuDeviceTimer: null,
    actualDeviceTimer: null,
    recordingTimer: null,
    // One-shot guards so a persistently-degraded backend warns once per
    // poller instead of flooding the console on every 2s tick.
    warnedOnce: {}
};

/**
 * Handle a poller fetch failure. 404 / absent endpoints are expected (the
 * feature may not be compiled in) and stay quiet; 5xx or thrown errors mean a
 * degraded backend and warn exactly once per poller key.
 *
 * @param {string} key     poller identifier (for the once-guard)
 * @param {Response|null} resp  the fetch Response (null when fetch threw)
 * @param {Error|null} err  the thrown error, if any
 */
function _handlePollFailure(key, resp, err) {
    // Quiet for a plain 404 / not-found — endpoint simply isn't available.
    if (resp && resp.status === 404) return;
    if (_diag.warnedOnce[key]) return;
    _diag.warnedOnce[key] = true;
    if (resp) {
        console.warn(`[diagnostics] ${key} poll failed: HTTP ${resp.status}`);
    } else {
        console.warn(`[diagnostics] ${key} poll failed:`, err);
    }
}

function diagnosticsView() {
    return {
        windowSeconds: 60,
        windowOptions: [
            { label: '1s', value: 1 },
            { label: '5s', value: 5 },
            { label: '10s', value: 10 },
            { label: '30s', value: 30 },
            { label: '1m', value: 60 },
            { label: '5m', value: 300 },
            { label: 'All', value: 0 }
        ],

        get chartReady() { return _diag.chart != null; },

        _resolveColor(varName, fallback) {
            const val = getComputedStyle(document.documentElement).getPropertyValue(varName).trim();
            return val || fallback;
        },

        // Collapsible section state (power-user sections collapsed by default)
        showSerialHealth: false,
        showMessageErrors: false,
        showMessageStats: false,
        showLogLevels: false,
        showDeviceStatus: false,
        showEmulatedDevices: true,
        showActualDevices: true,
        showRecording: false,
        showSpaside: false,
        showMqtt: false,
        showMatter: false,

        // MQTT broker status diagnostics
        mqtt: { enabled: false },

        // Matter bridge status diagnostics (sidecar status + commissioning QR)
        matter: { enabled: false },

        // Serial recording control state
        recording: { recording: false, file: '', bytes: 0 },
        recordingFilename: 'capture.cap',
        recordingBusy: false,

        // Spa-side remotes (Dual Spa Switch / Spa Link): decoded LEDs + last press, plus
        // button-press injection on emulated remotes.
        spasideRemotes: [],
        spasideBusy: false,

        // Decoded spa-switch button->function assignments (from the iAQ/OneTouch config page),
        // keyed by the controller's switch numbering. [{switch,button,function}, ...]
        spasideAssignments: [],

        // "Set assignment" form state: program switch:button -> function over the bus.
        spasideAssign: { switch: 1, button: 1, function: '' },

        // User-requested (desired-state) assignments persisted server-side. [{switch,button,function}]
        spasideRequested: [],

        // Emulated device diagnostics
        emulatedDevices: [],

        // Actual (real, non-emulated) device diagnostics
        actualDevices: [],

        // Log level control state
        logChannels: {},
        severityLevels: [],
        globalLevel: '',
        logLevelsLoaded: false,

        initChart() {
            Alpine.store('ws').connectStats();

            if (!_diag.chart) {
                this.$nextTick(() => this._createChart());
            }

            this._fetchLogLevels();
            this.fetchEmulatedDevices();
            this.fetchActualDevices();
            this.fetchRecordingStatus();
            this.fetchMqtt();
            // Guard against a leaked interval if initChart() runs again before destroyChart().
            if (!_diag.emuDeviceTimer) {
                _diag.emuDeviceTimer = setInterval(() => this.fetchEmulatedDevices(), 2000);
            }
            if (!_diag.actualDeviceTimer) {
                _diag.actualDeviceTimer = setInterval(() => this.fetchActualDevices(), 2000);
            }
            if (!_diag.recordingTimer) {
                _diag.recordingTimer = setInterval(() => this.fetchRecordingStatus(), 2000);
            }
            this.fetchSpasideRemotes();
            if (!_diag.spasideTimer) {
                _diag.spasideTimer = setInterval(() => this.fetchSpasideRemotes(), 2000);
            }
            if (!_diag.mqttTimer) {
                _diag.mqttTimer = setInterval(() => this.fetchMqtt(), 2000);
            }
            this.fetchMatter();
            if (!_diag.matterTimer) {
                _diag.matterTimer = setInterval(() => this.fetchMatter(), 2000);
            }
        },

        _createChart() {
            const ctx = this.$refs.utilizationChart;
            if (!ctx || typeof Chart === 'undefined') return;

            const textColor = this._resolveColor('--text-secondary', '#94a3b8');
            const gridColor = this._resolveColor('--grid-color', 'rgba(148,163,184,0.15)');
            const legendColor = this._resolveColor('--text-primary', '#e2e8f0');

            _diag.chart = new Chart(ctx, {
                type: 'line',
                data: {
                    datasets: [
                        {
                            label: 'Read %',
                            data: [],
                            borderColor: '#10b981',
                            backgroundColor: 'rgba(16, 185, 129, 0.1)',
                            borderWidth: 2,
                            tension: 0.4,
                            fill: true,
                            pointRadius: 0
                        },
                        {
                            label: 'Write %',
                            data: [],
                            borderColor: '#3b82f6',
                            backgroundColor: 'rgba(59, 130, 246, 0.1)',
                            borderWidth: 2,
                            tension: 0.4,
                            fill: true,
                            pointRadius: 0
                        }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    interaction: { mode: 'index', intersect: false },
                    scales: {
                        x: {
                            type: 'time',
                            time: {
                                displayFormats: {
                                    second: 'HH:mm:ss',
                                    minute: 'HH:mm',
                                    hour: 'HH:mm'
                                }
                            },
                            ticks: { maxRotation: 0, autoSkip: true, maxTicksLimit: 8, color: textColor },
                            grid: { color: gridColor }
                        },
                        y: {
                            beginAtZero: true,
                            max: 100,
                            ticks: {
                                callback: v => v + '%',
                                color: textColor
                            },
                            grid: { color: gridColor }
                        }
                    },
                    plugins: {
                        legend: { display: true, position: 'top', labels: { color: legendColor } },
                        tooltip: { callbacks: { label: item => item.dataset.label + ': ' + item.parsed.y.toFixed(2) + '%' } }
                    },
                    animation: { duration: 0 }
                }
            });

            // Read windowSeconds directly from _diag — no Alpine proxy involved
            _diag.statsListener = () => {
                _updateChartData(_diag.windowSeconds);
            };
            window.addEventListener('stats-updated', _diag.statsListener);

            // Apply initial data
            _updateChartData(_diag.windowSeconds);
        },

        setWindow(seconds) {
            this.windowSeconds = seconds;
            _diag.windowSeconds = seconds;
            _updateChartData(seconds);
        },

        destroyChart() {
            Alpine.store('ws').disconnectStats();

            if (_diag.emuDeviceTimer) {
                clearInterval(_diag.emuDeviceTimer);
                _diag.emuDeviceTimer = null;
            }

            if (_diag.actualDeviceTimer) {
                clearInterval(_diag.actualDeviceTimer);
                _diag.actualDeviceTimer = null;
            }

            if (_diag.recordingTimer) {
                clearInterval(_diag.recordingTimer);
                _diag.recordingTimer = null;
            }

            if (_diag.spasideTimer) {
                clearInterval(_diag.spasideTimer);
                _diag.spasideTimer = null;
            }

            if (_diag.mqttTimer) {
                clearInterval(_diag.mqttTimer);
                _diag.mqttTimer = null;
            }

            if (_diag.matterTimer) {
                clearInterval(_diag.matterTimer);
                _diag.matterTimer = null;
            }

            if (_diag.statsListener) {
                window.removeEventListener('stats-updated', _diag.statsListener);
                _diag.statsListener = null;
            }

            if (_diag.chart) {
                _diag.chart.destroy();
                _diag.chart = null;
            }
        },

        async fetchEmulatedDevices() {
            try {
                const resp = await fetch('/api/diagnostics/emulated-devices');
                if (!resp.ok) { _handlePollFailure('emulated-devices', resp, null); return; }
                this.emulatedDevices = await resp.json();
                _diag.warnedOnce['emulated-devices'] = false;
            } catch (e) {
                _handlePollFailure('emulated-devices', null, e);
            }
        },

        async fetchActualDevices() {
            try {
                const resp = await fetch('/api/diagnostics/actual-devices');
                if (!resp.ok) { _handlePollFailure('actual-devices', resp, null); return; }
                this.actualDevices = await resp.json();
                _diag.warnedOnce['actual-devices'] = false;
            } catch (e) {
                _handlePollFailure('actual-devices', null, e);
            }
        },

        async fetchRecordingStatus() {
            try {
                const resp = await fetch('/api/diagnostics/recording');
                if (!resp.ok) { _handlePollFailure('recording', resp, null); return; }
                this.recording = await resp.json();
                _diag.warnedOnce['recording'] = false;
            } catch (e) {
                _handlePollFailure('recording', null, e);
            }
        },

        async fetchSpasideRemotes() {
            try {
                const resp = await fetch('/api/equipment/spaside-remotes');
                if (!resp.ok) { _handlePollFailure('spaside-remotes', resp, null); return; }
                const data = await resp.json();
                this.spasideRemotes = (data && Array.isArray(data.remotes)) ? data.remotes : [];
                this.spasideAssignments = (data && Array.isArray(data.assignments)) ? data.assignments : [];
                this.spasideRequested = (data && Array.isArray(data.requested)) ? data.requested : [];
                _diag.warnedOnce['spaside-remotes'] = false;
            } catch (e) {
                _handlePollFailure('spaside-remotes', null, e);
            }
        },

        // Inject a momentary press of `button` on the emulated remote at `address`. No-op for a
        // real (observed) remote -- the server rejects it and we surface the reason.
        async pressSpasideButton(address, button) {
            if (this.spasideBusy) return;
            this.spasideBusy = true;
            try {
                // The list reports address as a hex string ("0x20") for display; the API expects a
                // numeric byte. parseInt auto-detects the 0x prefix.
                const addr = (typeof address === 'string') ? parseInt(address, 16) : address;
                const resp = await fetch('/api/equipment/spaside-remotes', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ action: 'press', address: addr, button: button })
                });
                const data = await resp.json().catch(() => ({}));
                if (resp.ok) {
                    if (data && Array.isArray(data.remotes)) { this.spasideRemotes = data.remotes; }
                    Alpine.store('toast').show('Spa-side button ' + button + ' pressed', 'info');
                } else {
                    Alpine.store('toast').show(data.error || 'Failed to press spa-side button', 'error');
                }
            } catch (e) {
                Alpine.store('toast').show('Failed to press spa-side button', 'error');
            } finally {
                this.spasideBusy = false;
            }
        },

        // Program a spa-side switch button's function over the bus (drives the controller's Spa
        // Switch / Spa Remotes config menu). Takes effect on whichever controller can write it
        // (OneTouch today; an iAQ that can't reports unavailable and we fall through). The live
        // assignment list refreshes on the next poll once the controller re-reports it.
        async setSpasideAssignment() {
            if (this.spasideBusy) return;
            const sw = parseInt(this.spasideAssign.switch, 10);
            const btn = parseInt(this.spasideAssign.button, 10);
            const fn = (this.spasideAssign.function || '').trim();
            if (!Number.isInteger(sw) || sw < 1 || !Number.isInteger(btn) || btn < 1 || fn === '') {
                Alpine.store('toast').show('Enter a switch, button and function to assign', 'error');
                return;
            }
            this.spasideBusy = true;
            try {
                const resp = await fetch('/api/equipment/spaside-remotes', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ action: 'assign', switch: sw, button: btn, function: fn })
                });
                const data = await resp.json().catch(() => ({}));
                if (resp.ok) {
                    Alpine.store('toast').show('Programming switch ' + sw + ' button ' + btn + ' → ' + fn + '…', 'info');
                } else {
                    Alpine.store('toast').show(data.error || 'Failed to program spa-switch assignment', 'error');
                }
            } catch (e) {
                Alpine.store('toast').show('Failed to program spa-switch assignment', 'error');
            } finally {
                this.spasideBusy = false;
            }
        },

        // A requested assignment is "pending" until the controller's live decoded map reports the
        // same function for that switch:button (a OneTouch programs asynchronously; an iAQ-only
        // system never confirms, so it stays a pending annotation).
        spasideRequestedPending(r) {
            const live = this.spasideAssignments.find(a => a.switch === r.switch && a.button === r.button);
            return !live || live.function !== r.function;
        },

        // The distinct function names the controller currently reports, to seed the assign datalist.
        spasideKnownFunctions() {
            const seen = [];
            for (const a of this.spasideAssignments) {
                if (a.function && !seen.includes(a.function)) { seen.push(a.function); }
            }
            return seen;
        },

        // Button layout for a remote. A "Dual Spa Switch" is the 6588 Dual Spa Side Interface
        // board, which bridges two physical spa-side switches onto the bus: Switch 2 = button
        // codes 1-4, Switch 3 = codes 5-8 (the "2x4"). Any other class is a single keypad.
        spasideButtonGroups(remote) {
            const range = (lo, hi) => { const a = []; for (let b = lo; b <= hi; b++) a.push(b); return a; };
            if (remote.device_class === 'DualSpaSwitch' && remote.button_count >= 8) {
                return [
                    { label: 'Spa Switch 2', buttons: range(1, 4) },
                    { label: 'Spa Switch 3', buttons: range(5, 8) },
                ];
            }
            return [{ label: '', buttons: range(1, remote.button_count || 0) }];
        },

        // Map an indicator state to an existing badge class (green=on, amber=blink, grey=off).
        spasideLedBadgeClass(state) {
            if (state === 'on') return 'badge-freq';
            if (state === 'blink') return 'badge-status-warn';
            return 'badge-status-off';
        },

        async fetchMqtt() {
            try {
                const resp = await fetch('/api/diagnostics/mqtt');
                if (!resp.ok) { _handlePollFailure('mqtt', resp, null); return; }
                this.mqtt = await resp.json();
                _diag.warnedOnce['mqtt'] = false;
            } catch (e) {
                _handlePollFailure('mqtt', null, e);
            }
        },

        async fetchMatter() {
            try {
                const resp = await fetch('/api/diagnostics/matter');
                if (!resp.ok) { _handlePollFailure('matter', resp, null); return; }
                this.matter = await resp.json();
                _diag.warnedOnce['matter'] = false;
                this.$nextTick(() => this._renderMatterQr());
            } catch (e) {
                _handlePollFailure('matter', null, e);
            }
        },

        // Render the commissioning QR into the panel canvas when a QR library is
        // vendored (window.QRCode, davidshimjs/qrcodejs). Without one we still show the
        // manual pairing code + QR payload text, which pair every ecosystem.
        _renderMatterQr() {
            const payload = this.matter && this.matter.qr_payload;
            const el = this.$refs && this.$refs.matterQr;
            if (!el) return;
            if (!payload || typeof window.QRCode === 'undefined') {
                el.innerHTML = '';
                return;
            }
            if (_diag.matterQrPayload === payload && el.childElementCount > 0) return;
            _diag.matterQrPayload = payload;
            el.innerHTML = '';
            try {
                // eslint-disable-next-line no-new
                new window.QRCode(el, { text: payload, width: 200, height: 200, correctLevel: window.QRCode.CorrectLevel.M });
            } catch (e) {
                _handlePollFailure('matter-qr', null, e);
            }
        },

        async startRecording() {
            if (this.recordingBusy || !this.recordingFilename) return;
            this.recordingBusy = true;
            try {
                const resp = await fetch('/api/diagnostics/recording', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ action: 'start', filename: this.recordingFilename })
                });
                const data = await resp.json().catch(() => ({}));
                if (resp.ok) {
                    this.recording = data;
                    Alpine.store('toast').show('Serial recording started', 'info');
                } else {
                    Alpine.store('toast').show(data.error || 'Failed to start recording', 'error');
                }
            } catch (e) {
                Alpine.store('toast').show('Failed to start recording', 'error');
            } finally {
                this.recordingBusy = false;
            }
        },

        async stopRecording() {
            if (this.recordingBusy) return;
            this.recordingBusy = true;
            try {
                const resp = await fetch('/api/diagnostics/recording', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ action: 'stop' })
                });
                const data = await resp.json().catch(() => ({}));
                if (resp.ok) {
                    this.recording = data;
                    Alpine.store('toast').show('Serial recording stopped', 'info');
                } else {
                    Alpine.store('toast').show(data.error || 'Failed to stop recording', 'error');
                }
            } catch (e) {
                Alpine.store('toast').show('Failed to stop recording', 'error');
            } finally {
                this.recordingBusy = false;
            }
        },

        formatBytes(bytes) {
            if (!bytes || bytes === 0) return '0 B';
            const k = 1024;
            const sizes = ['B', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        },

        formatMicros(us) {
            if (us == null) return '--';
            if (us < 1000) return us.toFixed(0) + ' \u00B5s';
            if (us < 1000000) return (us / 1000).toFixed(2) + ' ms';
            return (us / 1000000).toFixed(2) + ' s';
        },

        utilColor(val) {
            const v = parseFloat(val) || 0;
            if (v < 50) return 'var(--gauge-good)';
            if (v < 80) return 'var(--gauge-warn)';
            return 'var(--gauge-bad)';
        },

        // Maps a device operating_state to a badge severity class.
        // NOTE: these string values are C++ enum names emitted verbatim by
        // magic_enum::enum_name() over OperatingStates in DescribeDiagnostics().
        // They form a cross-boundary contract — keep in sync if the C++ enum
        // names ever change.
        operatingStateClass(state) {
            switch (state) {
                case 'NormalOperation':
                case 'Scraping':
                    return 'badge-status-normal';
                case 'StartUp':
                case 'ColdStart':
                    return 'badge-status-warn';
                case 'FaultHasOccurred':
                case 'ScrapingFaulted':
                    return 'badge-status-danger';
                default:
                    return '';
            }
        },

        formatUptime(secs) {
            if (secs == null) return '--';
            const h = Math.floor(secs / 3600);
            const m = Math.floor((secs % 3600) / 60);
            if (h > 0) return h + 'h ' + m + 'm';
            return m + 'm';
        },

        async _fetchLogLevels() {
            try {
                const resp = await fetch('/api/diagnostics/logging');
                if (!resp.ok) { _handlePollFailure('logging', resp, null); return; }
                const data = await resp.json();
                this.logChannels = data.channels || {};
                this.severityLevels = data.severity_levels || [];
                this.globalLevel = this._computeGlobalLevel();
                this.logLevelsLoaded = true;
                _diag.warnedOnce['logging'] = false;
            } catch (e) {
                _handlePollFailure('logging', null, e);
            }
        },

        // Derive the global indicator: the shared level when every channel
        // agrees, otherwise '' (Mixed). Guards against an empty channel map
        // ([].every() is true, which would otherwise yield undefined).
        _computeGlobalLevel() {
            const values = Object.values(this.logChannels);
            return values.length > 0 && values.every(v => v === values[0]) ? values[0] : '';
        },

        async setChannelLevel(channel, level) {
            const previous = this.logChannels[channel];
            this.logChannels[channel] = level;            // optimistic
            this.globalLevel = this._computeGlobalLevel();
            try {
                const resp = await fetch('/api/diagnostics/logging', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ channel, level })
                });
                if (!resp.ok) {
                    throw new Error(`HTTP ${resp.status}`);
                }
            } catch (e) {
                // Revert the optimistic mutation and surface the failure.
                if (previous === undefined) {
                    delete this.logChannels[channel];
                } else {
                    this.logChannels[channel] = previous;
                }
                this.globalLevel = this._computeGlobalLevel();
                console.error(`[diagnostics] failed to set log level for ${channel}:`, e);
                Alpine.store('toast').show(`Failed to set log level for ${channel}`, 'error');
            }
        },

        async setGlobalLevel(level) {
            const previous = { ...this.logChannels };
            const previousGlobal = this.globalLevel;
            this.globalLevel = level;                     // optimistic
            for (const ch in this.logChannels) {
                this.logChannels[ch] = level;
            }
            try {
                const resp = await fetch('/api/diagnostics/logging', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ global: level })
                });
                if (!resp.ok) {
                    throw new Error(`HTTP ${resp.status}`);
                }
            } catch (e) {
                // Revert the optimistic mutation and surface the failure.
                this.logChannels = previous;
                this.globalLevel = previousGlobal;
                console.error('[diagnostics] failed to set global log level:', e);
                Alpine.store('toast').show('Failed to set global log level', 'error');
            }
        }
    };
}

// Standalone function — no Alpine proxy involvement
function _updateChartData(windowSeconds) {
    if (!_diag.chart) return;

    const h = window.__statsChartHistory;
    if (!h) return;

    const now = Date.now();
    let startIdx = 0;

    if (windowSeconds > 0 && h.times.length > 0) {
        const cutoff = now - windowSeconds * 1000;
        startIdx = h.times.length;
        for (let i = 0; i < h.times.length; i++) {
            if (h.times[i] >= cutoff) { startIdx = i; break; }
        }
    }

    // Build {x,y} point arrays so each point carries its own timestamp
    const slicedTimes = h.times.slice(startIdx);
    const slicedReads = h.reads.slice(startIdx);
    const slicedWrites = h.writes.slice(startIdx);

    _diag.chart.data.labels = undefined;
    _diag.chart.data.datasets[0].data = slicedTimes.map((t, i) => ({ x: t, y: slicedReads[i] }));
    _diag.chart.data.datasets[1].data = slicedTimes.map((t, i) => ({ x: t, y: slicedWrites[i] }));

    // Set explicit axis range so the chart always shows the full selected window
    const xAxis = _diag.chart.options.scales.x;
    if (windowSeconds > 0) {
        xAxis.min = now - windowSeconds * 1000;
        xAxis.max = now;
    } else {
        xAxis.min = undefined;
        xAxis.max = undefined;
    }

    _diag.chart.update('none');
}
