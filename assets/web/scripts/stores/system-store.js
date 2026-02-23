/**
 * System Store — Global system state machine
 *
 * Derives an effective UI state from backend-reported state + WebSocket connectivity.
 *
 * States:
 *   Ready        — system operational, commands allowed
 *   Starting     — backend has not yet discovered equipment
 *   Disconnected — WebSocket connection lost
 *   ServiceMode  — controller is in service mode
 *   MonitorOnly  — user-toggled read-only mode
 *
 * Priority: ServiceMode > Starting > Disconnected > MonitorOnly > Ready
 */
document.addEventListener('alpine:init', () => {
    Alpine.store('system', {
        // Backend-reported state (from SystemStateUpdate WS event)
        backendState: 'starting',
        poolConfiguration: 'Unknown',
        equipmentMode: 'Normal',

        // Client-side flags
        monitorOnly: JSON.parse(localStorage.getItem('monitorOnly') || 'false'),

        // Server info (from /api/version REST response)
        serverStartTime: null,
        uptimeSeconds: null,

        // Transition tracking for service mode modal
        _prevBackendState: null,
        _serviceModalDismissed: false,

        // Device status tracking (from SystemStatusChange events)
        deviceStatuses: [],

        get state() {
            const wsConnected = Alpine.store('ws').connected;

            if (this.backendState === 'service_mode') return 'ServiceMode';
            if (this.backendState === 'starting') return 'Starting';
            if (!wsConnected) return 'Disconnected';
            if (this.monitorOnly) return 'MonitorOnly';
            return 'Ready';
        },

        get commandsEnabled() {
            return this.state === 'Ready';
        },

        get disableReason() {
            switch (this.state) {
                case 'ServiceMode': return 'Service Mode is active on the controller';
                case 'Starting': return 'System is starting up';
                case 'Disconnected': return 'Connection to server lost';
                case 'MonitorOnly': return 'Monitor-only mode is enabled';
                default: return '';
            }
        },

        get showServiceModal() {
            return this.state === 'ServiceMode' &&
                   this._prevBackendState !== null &&
                   this._prevBackendState !== 'service_mode' &&
                   !this._serviceModalDismissed;
        },

        toggleMonitorOnly() {
            this.monitorOnly = !this.monitorOnly;
            localStorage.setItem('monitorOnly', JSON.stringify(this.monitorOnly));
        },

        dismissServiceModal() {
            this._serviceModalDismissed = true;
        },

        handleEvent(msg) {
            if (!msg || !msg.type) return;

            if (msg.type === 'SystemStateUpdate') {
                const p = msg.payload;
                if (!p) return;

                this._prevBackendState = this.backendState;

                if (p.state) {
                    this.backendState = p.state;
                    // Reset service modal dismiss when leaving service mode
                    if (p.state !== 'service_mode') {
                        this._serviceModalDismissed = false;
                    }
                }
                if (p.pool_configuration) this.poolConfiguration = p.pool_configuration;
                if (p.equipment_mode) this.equipmentMode = p.equipment_mode;
            }

            if (msg.type === 'SystemStatusChange') {
                const p = msg.payload;
                if (!p) return;
                // Device status transitions: Initializing → Normal
                // When any device reports Normal, system is operational
                if (p.status_type === 'Normal') {
                    if (this.backendState === 'starting') {
                        this._prevBackendState = this.backendState;
                        this.backendState = 'ready';
                    }
                }

                // Track per-device status for diagnostics
                if (p.source_name && p.status_type) {
                    const key = p.source_name + ':' + (p.source_type || '');
                    const idx = this.deviceStatuses.findIndex(d => d.key === key);
                    const entry = {
                        key,
                        sourceName: p.source_name,
                        sourceType: p.source_type || '',
                        statusType: p.status_type,
                        lastSeen: new Date().toLocaleTimeString()
                    };
                    if (idx !== -1) {
                        this.deviceStatuses[idx] = entry;
                    } else {
                        this.deviceStatuses = [...this.deviceStatuses, entry];
                    }
                }
            }
        }
    });
});
