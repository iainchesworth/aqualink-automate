/**
 * WebSocket Store — Manages both WS connections with auto-reconnect
 */
document.addEventListener('alpine:init', () => {
    Alpine.store('ws', {
        connected: false,
        statsConnected: false,
        _equipmentWs: null,
        _statsWs: null,
        _reconnectDelay: 1000,
        _maxReconnectDelay: 30000,
        _currentDelay: 1000,
        _statsCurrentDelay: 1000,

        connectEquipment() {
            if (this._equipmentWs && this._equipmentWs.readyState <= WebSocket.OPEN) return;

            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const url = `${protocol}//${window.location.host}/ws/equipment`;

            try {
                this._equipmentWs = new WebSocket(url);

                this._equipmentWs.onopen = () => {
                    this.connected = true;
                    this._currentDelay = this._reconnectDelay;
                };

                this._equipmentWs.onmessage = (event) => {
                    try {
                        const msg = JSON.parse(event.data);
                        console.debug('[WS /equipment]', msg.type, msg.payload);
                        Alpine.store('pool').handleEvent(msg);
                    } catch (e) {
                        console.error('WS parse error:', e);
                    }
                };

                this._equipmentWs.onclose = () => {
                    this.connected = false;
                    this._scheduleReconnect('equipment');
                };

                this._equipmentWs.onerror = () => {
                    this.connected = false;
                };
            } catch (e) {
                this.connected = false;
                this._scheduleReconnect('equipment');
            }
        },

        connectStats() {
            if (this._statsWs && this._statsWs.readyState <= WebSocket.OPEN) return;

            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const url = `${protocol}//${window.location.host}/ws/equipment/stats`;

            try {
                this._statsWs = new WebSocket(url);

                this._statsWs.onopen = () => {
                    this.statsConnected = true;
                    this._statsCurrentDelay = this._reconnectDelay;
                };

                this._statsWs.onmessage = (event) => {
                    try {
                        const msg = JSON.parse(event.data);
                        Alpine.store('stats').handleEvent(msg);
                    } catch (e) {
                        console.error('Stats WS parse error:', e);
                    }
                };

                this._statsWs.onclose = () => {
                    this.statsConnected = false;
                    this._scheduleReconnect('stats');
                };

                this._statsWs.onerror = () => {
                    this.statsConnected = false;
                };
            } catch (e) {
                this.statsConnected = false;
                this._scheduleReconnect('stats');
            }
        },

        disconnectStats() {
            if (this._statsWs) {
                this._statsWs.onclose = null;
                this._statsWs.close();
                this._statsWs = null;
                this.statsConnected = false;
            }
        },

        _scheduleReconnect(type) {
            if (type === 'equipment') {
                setTimeout(() => this.connectEquipment(), this._currentDelay);
                this._currentDelay = Math.min(this._currentDelay * 2, this._maxReconnectDelay);
            } else {
                setTimeout(() => this.connectStats(), this._statsCurrentDelay);
                this._statsCurrentDelay = Math.min(this._statsCurrentDelay * 2, this._maxReconnectDelay);
            }
        }
    });
});
