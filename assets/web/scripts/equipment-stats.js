/**
 * Aqualink Automate - Equipment Statistics
 * JavaScript for statistics and monitoring page
 */

class EquipmentStats {
    constructor() {
        this.apiBaseUrl = window.location.origin;
        this.wsSocket = null;
        this.chart = null;
        this.messageStats = new Map();
        
        // Chart data
        this.timeData = [];
        this.readUtilData = [];
        this.writeUtilData = [];
        this.maxDataPoints = 500;
        
        this.init();
    }

    init() {
        console.log('Initializing Equipment Statistics...');
        
        // Load version info
        this.loadVersionInfo();
        
        // Initialize chart
        this.initializeChart();
        
        // Setup WebSocket
        this.connectWebSocket();
        
        // Setup event listeners
        this.setupEventListeners();
    }

    // ==================== Chart Setup ====================
    
    initializeChart() {
        const ctx = document.getElementById('utilizationChart');
        if (!ctx) return;
        
        this.chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: this.timeData,
                datasets: [
                    {
                        label: 'Read Utilization %',
                        data: this.readUtilData,
                        borderColor: 'rgb(40, 167, 69)',
                        backgroundColor: 'rgba(40, 167, 69, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true
                    },
                    {
                        label: 'Write Utilization %',
                        data: this.writeUtilData,
                        borderColor: 'rgb(0, 123, 255)',
                        backgroundColor: 'rgba(0, 123, 255, 0.1)',
                        borderWidth: 2,
                        tension: 0.4,
                        fill: true
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: true,
                interaction: {
                    mode: 'index',
                    intersect: false
                },
                scales: {
                    x: {
                        type: 'time',
                        time: {
                            unit: 'second',
                            displayFormats: {
                                second: 'HH:mm:ss'
                            }
                        },
                        title: {
                            display: true,
                            text: 'Time'
                        },
                        ticks: {
                            maxRotation: 0,
                            autoSkip: true,
                            maxTicksLimit: 10
                        }
                    },
                    y: {
                        beginAtZero: true,
                        max: 100,
                        title: {
                            display: true,
                            text: 'Utilization %'
                        },
                        ticks: {
                            callback: function(value) {
                                return value + '%';
                            }
                        }
                    }
                },
                plugins: {
                    legend: {
                        display: true,
                        position: 'top'
                    },
                    tooltip: {
                        callbacks: {
                            label: function(context) {
                                return context.dataset.label + ': ' + context.parsed.y.toFixed(2) + '%';
                            }
                        }
                    },
                    decimation: {
                        enabled: true,
                        algorithm: 'lttb',
                        samples: 100
                    }
                },
                animation: {
                    duration: 300
                }
            }
        });
    }

    updateChart(readUtil, writeUtil) {
        const now = new Date();
        
        this.timeData.push(now);
        this.readUtilData.push(parseFloat(readUtil) || 0);
        this.writeUtilData.push(parseFloat(writeUtil) || 0);
        
        // Keep only recent data
        if (this.timeData.length > this.maxDataPoints) {
            this.timeData.shift();
            this.readUtilData.shift();
            this.writeUtilData.shift();
        }
        
        if (this.chart) {
            this.chart.update('none');
        }
    }

    // ==================== API Methods ====================
    
    async loadVersionInfo() {
        try {
            const response = await fetch(`${this.apiBaseUrl}/api/version`);
            const data = await response.json();
            
            if (data.software_version) {
                document.getElementById('appVersion').textContent = data.software_version.version || '-';
            }
        } catch (error) {
            console.error('Failed to load version info:', error);
        }
    }

    async loadDeviceInfo() {
        try {
            const response = await fetch(`${this.apiBaseUrl}/api/equipment/version`);
            const data = await response.json();
            
            document.getElementById('deviceModel').textContent = data.type || 'Unknown';
            document.getElementById('deviceFirmware').textContent = data.firmware || '-';
        } catch (error) {
            console.error('Failed to load device info:', error);
        }
    }

    // ==================== WebSocket ====================
    
    connectWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws/equipment/stats`;
        
        console.log('Connecting to Statistics WebSocket:', wsUrl);
        
        try {
            this.wsSocket = new WebSocket(wsUrl);
            
            this.wsSocket.onopen = () => {
                console.log('Statistics WebSocket connected');
                this.updateConnectionStatus(true);
            };
            
            this.wsSocket.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    this.handleWebSocketMessage(data);
                } catch (error) {
                    console.error('WebSocket message parse error:', error);
                }
            };
            
            this.wsSocket.onerror = (error) => {
                console.error('WebSocket error:', error);
                this.updateConnectionStatus(false);
            };
            
            this.wsSocket.onclose = () => {
                console.log('Statistics WebSocket disconnected');
                this.updateConnectionStatus(false);
                setTimeout(() => this.connectWebSocket(), 5000);
            };
        } catch (error) {
            console.error('WebSocket connection error:', error);
            this.updateConnectionStatus(false);
        }
    }

    handleWebSocketMessage(data) {
        if (!data || !data.type) return;
        
        if (data.type === 'StatisticsUpdate' && data.payload) {
            this.updateStatistics(data.payload);
        }
        
        this.updateLastUpdateTime();
    }

    updateStatistics(payload) {
        // Update bandwidth metrics
        if (payload.bandwidth_read) {
            this.updateBandwidthMetrics('read', payload.bandwidth_read);
        }
        
        if (payload.bandwidth_write) {
            this.updateBandwidthMetrics('write', payload.bandwidth_write);
        }
        
        // Update chart
        const readUtil1s = payload.bandwidth_read?.average_utilisation_1sec || 0;
        const writeUtil1s = payload.bandwidth_write?.average_utilisation_1sec || 0;
        this.updateChart(readUtil1s, writeUtil1s);
        
        // Update message counts
        if (payload.message_counts) {
            this.updateMessageStats(payload.message_counts);
        }
    }

    updateBandwidthMetrics(direction, metrics) {
        const prefix = direction;
        
        // Update total bytes
        const totalBytes = metrics.total_bytes || 0;
        const totalBytesElement = document.getElementById(`${prefix}TotalBytes`);
        if (totalBytesElement) {
            totalBytesElement.textContent = this.formatBytes(totalBytes);
        }
        
        // Update utilization percentages
        const util1s = metrics.average_utilisation_1sec || 0;
        const util30s = metrics.average_utilisation_30sec || 0;
        const util5m = metrics.average_utilisation_5mins || 0;
        
        this.updateUtilBadge(`${prefix}Util1s`, util1s);
        this.updateUtilBadge(`${prefix}Util30s`, util30s);
        this.updateUtilBadge(`${prefix}Util5m`, util5m);
        
        // Update progress bar
        const progressBar = document.getElementById(`${prefix}UtilBar`);
        if (progressBar) {
            const percentage = Math.min(100, util1s);
            progressBar.style.width = percentage + '%';
            progressBar.textContent = percentage.toFixed(1) + '%';
            
            // Update color based on utilization
            progressBar.classList.remove('bg-success', 'bg-warning', 'bg-danger');
            if (percentage < 50) {
                progressBar.classList.add('bg-success');
            } else if (percentage < 80) {
                progressBar.classList.add('bg-warning');
            } else {
                progressBar.classList.add('bg-danger');
            }
        }
    }

    updateUtilBadge(elementId, value) {
        const element = document.getElementById(elementId);
        if (!element) return;
        
        const percentage = parseFloat(value) || 0;
        element.textContent = percentage.toFixed(2) + '%';
        
        // Update badge color
        element.classList.remove('bg-success', 'bg-warning', 'bg-danger', 'bg-light');
        if (percentage < 50) {
            element.classList.add('bg-success', 'text-white');
        } else if (percentage < 80) {
            element.classList.add('bg-warning', 'text-dark');
        } else if (percentage >= 80) {
            element.classList.add('bg-danger', 'text-white');
        } else {
            element.classList.add('bg-light', 'text-dark');
        }
    }

    updateMessageStats(stats) {
        const tbody = document.querySelector('#messageStatsTable tbody');
        if (!tbody) return;
        
        // Clear loading message on first update
        if (tbody.children.length === 1 && tbody.children[0].children.length === 1) {
            tbody.innerHTML = '';
        }
        
        stats.forEach(stat => {
            const existingRow = document.getElementById(`msg-row-${stat.id}`);
            
            if (existingRow) {
                // Update existing row
                existingRow.cells[1].textContent = stat.count || 0;
                this.updateMessageFrequency(existingRow.cells[2], stat);
                existingRow.cells[3].textContent = new Date().toLocaleTimeString();
            } else {
                // Create new row
                const row = tbody.insertRow();
                row.id = `msg-row-${stat.id}`;
                
                const cellId = row.insertCell(0);
                cellId.innerHTML = `<code>${stat.id}</code>`;
                
                const cellCount = row.insertCell(1);
                cellCount.textContent = stat.count || 0;
                
                const cellFreq = row.insertCell(2);
                this.updateMessageFrequency(cellFreq, stat);
                
                const cellTime = row.insertCell(3);
                cellTime.textContent = new Date().toLocaleTimeString();
            }
            
            // Store stat data
            this.messageStats.set(stat.id, {
                count: stat.count,
                lastUpdate: new Date()
            });
        });
    }

    updateMessageFrequency(cell, stat) {
        const freq = stat.frequency || 0;
        if (freq > 0) {
            cell.innerHTML = `<span class="badge bg-primary">${freq.toFixed(2)} msg/s</span>`;
        } else {
            cell.innerHTML = `<span class="badge bg-secondary">-</span>`;
        }
    }

    // ==================== UI Helpers ====================
    
    updateConnectionStatus(connected) {
        const statusDot = document.getElementById('statsConnectionDot');
        const statusText = document.getElementById('statsConnectionStatus');
        
        if (statusDot) {
            statusDot.className = 'status-dot me-2';
            if (connected) {
                statusDot.classList.add('connected');
            } else {
                statusDot.classList.add('disconnected');
            }
        }
        
        if (statusText) {
            statusText.textContent = connected ? 'Connected' : 'Disconnected';
            statusText.className = connected ? 'text-success' : 'text-danger';
        }
    }

    updateLastUpdateTime() {
        const element = document.getElementById('statsLastUpdate');
        if (element) {
            element.textContent = new Date().toLocaleTimeString();
        }
    }

    formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    // ==================== Event Listeners ====================
    
    setupEventListeners() {
        const refreshBtn = document.getElementById('refreshStats');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => {
                this.loadDeviceInfo();
                this.showRefreshAnimation(refreshBtn);
            });
        }
    }

    showRefreshAnimation(button) {
        const icon = button.querySelector('i');
        if (icon) {
            icon.classList.add('fa-spin');
            setTimeout(() => {
                icon.classList.remove('fa-spin');
            }, 1000);
        }
    }
}

// Initialize when DOM is ready
let equipmentStats;
document.addEventListener('DOMContentLoaded', () => {
    equipmentStats = new EquipmentStats();
});
