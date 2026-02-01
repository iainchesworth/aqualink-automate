/**
 * Aqualink Automate - Pool Monitor
 * Main JavaScript for dashboard functionality
 */

class PoolMonitor {
    constructor() {
        this.apiBaseUrl = window.location.origin;
        this.wsSocket = null;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 10;
        this.reconnectDelay = 3000;
        this.buttons = new Map();
        this.lastUpdate = null;
        this.buttonLoadRetries = 0;
        this.maxButtonLoadRetries = 12; // 12 retries × 5 seconds = 1 minute
        
        this.init();
    }

    init() {
        console.log('Initializing Pool Monitor...');
        
        // Show initialization banner
        this.showSystemBanner('System Starting', 'Connecting to pool controller...', 'info');
        
        // Load initial data
        this.loadVersionInfo();
        this.loadEquipmentData();
        this.loadButtons();
        
        // Setup WebSocket connection
        this.connectWebSocket();
        
        // Setup event listeners
        this.setupEventListeners();
        
        // Start periodic updates
        this.startPeriodicUpdates();
    }

    // ==================== API Methods ====================
    
    async fetchAPI(endpoint) {
        try {
            const response = await fetch(`${this.apiBaseUrl}${endpoint}`);
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            return await response.json();
        } catch (error) {
            console.error(`API Error (${endpoint}):`, error);
            this.showNotification('API Error', `Failed to fetch ${endpoint}`, 'danger');
            throw error;
        }
    }

    async postAPI(endpoint, data = {}) {
        try {
            const response = await fetch(`${this.apiBaseUrl}${endpoint}`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(data)
            });
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            return await response.json();
        } catch (error) {
            console.error(`API Error (${endpoint}):`, error);
            this.showNotification('Action Failed', error.message, 'danger');
            throw error;
        }
    }

    // ==================== Data Loading ====================
    
    async loadVersionInfo() {
        try {
            const data = await this.fetchAPI('/api/version');
            
            if (data.software_version) {
                document.getElementById('appVersion').textContent = data.software_version.version || '-';
            }
            
            if (data.git_info) {
                const gitShort = data.git_info.commit_sha1 
                    ? data.git_info.commit_sha1.substring(0, 7) 
                    : '';
                document.getElementById('gitInfo').textContent = gitShort ? `(${gitShort})` : '';
            }
        } catch (error) {
            console.error('Failed to load version info:', error);
        }
    }

    async loadEquipmentData() {
        try {
            const data = await this.fetchAPI('/api/equipment');
            
            // Check if system is initializing (data may be incomplete)
            if (!data || Object.keys(data).length === 0) {
                console.log('Equipment data is empty, system may be initializing');
                this.showSystemBanner('System Initializing', 'Waiting for equipment data...', 'info');
                return;
            }
            
            this.updateEquipmentDisplay(data);
            this.hideSystemBanner();
        } catch (error) {
            console.error('Failed to load equipment data:', error);
            this.showSystemBanner('Error Loading Equipment', 'Unable to retrieve equipment status. Retrying...', 'danger');
        }
    }

    async loadButtons() {
        try {
            const data = await this.fetchAPI('/api/equipment/buttons');
            console.log('Buttons API response:', data);
            
            // Handle null buttons (system not initialized yet)
            if (data && data.buttons === null) {
                console.log('System not initialized yet, buttons are null');
                
                if (this.buttonLoadRetries < this.maxButtonLoadRetries) {
                    this.showButtonsInitializing();
                    this.buttonLoadRetries++;
                    
                    // Retry after 5 seconds
                    setTimeout(() => {
                        console.log(`Retrying button load (attempt ${this.buttonLoadRetries}/${this.maxButtonLoadRetries})...`);
                        this.loadButtons();
                    }, 5000);
                } else {
                    console.warn('Max button load retries reached');
                    this.showButtonsError();
                }
                return;
            }
            
            // Reset retry counter on successful load
            this.buttonLoadRetries = 0;
            
            // Handle invalid data structure
            if (!data || !Array.isArray(data.buttons)) {
                console.error('Invalid buttons data structure:', data);
                this.showButtonsError();
                return;
            }
            
            this.renderButtons(data.buttons);
        } catch (error) {
            console.error('Failed to load buttons:', error);
            this.showButtonsError();
        }
    }

    async loadEquipmentVersion() {
        try {
            const data = await this.fetchAPI('/api/equipment/version');
            if (data) {
                document.getElementById('controllerType').textContent = data.type || 'Unknown';
                document.getElementById('controllerFirmware').textContent = data.firmware || '-';
            }
        } catch (error) {
            console.error('Failed to load equipment version:', error);
        }
    }

    // ==================== UI Updates ====================
    
    updateEquipmentDisplay(data) {
        // Update temperatures (with null checks)
        if (data.temperatures) {
            this.updateTemperature('poolTemp', data.temperatures.pool);
            this.updateTemperature('spaTemp', data.temperatures.spa);
            this.updateTemperature('airTemp', data.temperatures.air);
        }

        // Update heating status
        this.updateStatusBadge('poolHeatStatus', data.poolHeatStatus);
        this.updateStatusBadge('spaHeatStatus', data.spaHeatStatus);

        // Update chemistry (with null checks)
        if (data.chemistry) {
            this.updateChemistry('phLevel', data.chemistry.ph, 7.2, 7.8, 'phProgress');
            this.updateChemistry('orpLevel', data.chemistry.orp, 650, 750, 'orpProgress');
            this.updateChemistry('saltLevel', data.chemistry.salt_in_ppm, 2700, 3400, 'saltProgress');
        }

        // Update main action buttons
        this.updateMainButton('spaHeatBtn', 'spaHeatBtnStatus', data.spaHeatStatus);
        this.updateMainButton('poolHeatBtn', 'poolHeatBtnStatus', data.poolHeatStatus);
        this.updateMainButton('userBtn', 'userBtnStatus', data.userStatus);
        this.updateMainButton('cleanBtn', 'cleanBtnStatus', data.cleanStatus);

        // Update system status
        this.updateSystemStatus('operational');
    }

    updateTemperature(elementId, value) {
        const element = document.getElementById(elementId);
        if (element) {
            element.textContent = value || '--';
            
            // Add color based on temperature
            if (value && value !== '-' && value !== '--') {
                const temp = parseFloat(value);
                element.classList.remove('temp-cold', 'temp-warm', 'temp-hot');
                
                if (temp < 70) {
                    element.classList.add('temp-cold');
                } else if (temp > 95) {
                    element.classList.add('temp-hot');
                } else if (temp > 85) {
                    element.classList.add('temp-warm');
                }
            }
        }
    }

    updateChemistry(elementId, value, minTarget, maxTarget, progressId) {
        const element = document.getElementById(elementId);
        const progressBar = document.getElementById(progressId);
        
        if (element) {
            element.textContent = value || '-';
        }
        
        if (progressBar && value && value !== '-') {
            const numValue = parseFloat(value);
            const range = maxTarget - minTarget;
            const percentage = Math.min(100, Math.max(0, ((numValue - minTarget) / range) * 100));
            progressBar.style.width = percentage + '%';
            
            // Change color based on range
            progressBar.classList.remove('bg-success', 'bg-warning', 'bg-danger');
            if (numValue >= minTarget && numValue <= maxTarget) {
                progressBar.classList.add('bg-success');
            } else if (numValue < minTarget * 0.9 || numValue > maxTarget * 1.1) {
                progressBar.classList.add('bg-danger');
            } else {
                progressBar.classList.add('bg-warning');
            }
        }
    }

    updateStatusBadge(elementId, status) {
        const element = document.getElementById(elementId);
        if (!element) return;
        
        const statusText = status != null ? String(status) : 'Unknown';
        element.textContent = statusText;
        element.className = 'badge';
        
        const statusLower = statusText.toLowerCase();
        if (statusLower === 'on' || statusLower === 'enabled') {
            element.classList.add('bg-success');
        } else if (statusLower === 'off') {
            element.classList.add('bg-secondary');
        } else {
            element.classList.add('bg-light', 'text-dark');
        }
    }

    updateMainButton(btnId, statusId, status) {
        const button = document.getElementById(btnId);
        const statusElement = document.getElementById(statusId);
        
        if (!button || !statusElement) return;
        
        const statusText = status != null ? String(status) : 'Unknown';
        statusElement.textContent = statusText;
        
        button.classList.remove('active');
        const statusLower = statusText.toLowerCase();
        if (statusLower === 'on' || statusLower === 'enabled') {
            button.classList.add('active');
        }
    }

    updateSystemStatus(status) {
        const statusElement = document.getElementById('systemStatus');
        const iconElement = document.getElementById('systemStatusIcon');
        
        if (statusElement) {
            statusElement.textContent = status === 'operational' ? 'Operational' : 'Offline';
        }
        
        if (iconElement) {
            iconElement.className = 'bi';
            if (status === 'operational') {
                iconElement.classList.add('bi-power', 'text-success');
            } else {
                iconElement.classList.add('bi-exclamation-triangle', 'text-warning');
            }
        }
    }

    // ==================== Button Rendering ====================
    
    renderButtons(buttons) {
        const container = document.getElementById('equipmentButtons');
        if (!container) {
            console.error('Equipment buttons container not found');
            return;
        }
        
        container.innerHTML = '';
        
        if (!buttons || buttons.length === 0) {
            console.log('No buttons to render');
            container.innerHTML = `
                <div class="col-12 text-center py-4">
                    <i class="bi bi-inbox text-muted" style="font-size: 3rem;"></i>
                    <p class="text-muted mt-3">No equipment controls available</p>
                </div>
            `;
            return;
        }
        
        console.log(`Rendering ${buttons.length} buttons`);
        
        buttons.forEach((button, index) => {
            try {
                if (!button || !button.id) {
                    console.warn(`Button at index ${index} is invalid:`, button);
                    return;
                }
                
                this.buttons.set(button.id, button);
                const buttonHtml = this.createButtonElement(button);
                container.insertAdjacentHTML('beforeend', buttonHtml);
            } catch (error) {
                console.error(`Error rendering button at index ${index}:`, button, error);
            }
        });
        
        // Add event listeners to all buttons
        buttons.forEach(button => {
            if (!button || !button.id) return;
            
            const btnElement = document.getElementById(`btn-${button.id}`);
            if (btnElement) {
                btnElement.addEventListener('click', () => this.handleButtonClick(button.id));
            } else {
                console.warn(`Button element not found for id: ${button.id}`);
            }
        });
    }

    createButtonElement(button) {
        const statusClass = this.getButtonStatusClass(button.status);
        const iconClass = this.getButtonIcon(button.label);
        
        // Safely extract label and status, handling various data types
        const labelText = button.label != null ? String(button.label) : 'Unknown';
        const statusText = button.status != null ? String(button.status) : 'Unknown';
        
        return `
            <div class="col-lg-3 col-md-4 col-sm-6">
                <div class="equipment-button ${statusClass}" id="btn-${button.id}">
                    <i class="bi ${iconClass}"></i>
                    <div class="button-label">${labelText}</div>
                    <span class="button-status" id="status-${button.id}">${statusText}</span>
                </div>
            </div>
        `;
    }

    getButtonStatusClass(status) {
        // Safely convert status to string and lowercase
        const statusStr = status != null ? String(status) : '';
        const statusLower = statusStr.toLowerCase();
        
        if (statusLower === 'on' || statusLower === 'enabled') {
            return 'active';
        }
        return '';
    }

    getButtonIcon(label) {
        // Safely convert label to string and lowercase
        const labelStr = label != null ? String(label) : '';
        const labelLower = labelStr.toLowerCase();
        
        if (labelLower.includes('light')) return 'bi-lightbulb';
        if (labelLower.includes('pump')) return 'bi-gear';
        if (labelLower.includes('heater') || labelLower.includes('heat')) return 'bi-fire';
        if (labelLower.includes('spa')) return 'bi-water';
        if (labelLower.includes('pool')) return 'bi-water';
        if (labelLower.includes('jet')) return 'bi-wind';
        if (labelLower.includes('clean')) return 'bi-brush';
        if (labelLower.includes('valve')) return 'bi-diagram-3';
        if (labelLower.includes('filter')) return 'bi-funnel';
        
        return 'bi-toggle-on';
    }

    showButtonsError() {
        const container = document.getElementById('equipmentButtons');
        if (container) {
            container.innerHTML = `
                <div class="col-12 text-center py-4">
                    <i class="bi bi-exclamation-circle text-danger" style="font-size: 3rem;"></i>
                    <p class="text-muted mt-3">Failed to load equipment controls</p>
                    <button class="btn btn-primary btn-sm" onclick="poolMonitor.loadButtons()">
                        <i class="bi bi-arrow-clockwise me-1"></i>Retry
                    </button>
                </div>
            `;
        }
    }

    showButtonsInitializing() {
        const container = document.getElementById('equipmentButtons');
        if (container) {
            const retryInfo = this.buttonLoadRetries > 0 
                ? `<p class="small text-muted mb-0">Retry ${this.buttonLoadRetries}/${this.maxButtonLoadRetries}</p>` 
                : '';
            
            container.innerHTML = `
                <div class="col-12 text-center py-4">
                    <div class="spinner-border text-primary mb-3" role="status">
                        <span class="visually-hidden">Loading...</span>
                    </div>
                    <p class="text-muted mt-3">System initializing...</p>
                    <p class="small text-muted">Equipment controls will appear once the system is ready</p>
                    ${retryInfo}
                </div>
            `;
        }
    }

    // ==================== Button Actions ====================
    
    async handleButtonClick(buttonId) {
        const btnElement = document.getElementById(`btn-${buttonId}`);
        const statusElement = document.getElementById(`status-${buttonId}`);
        
        if (!btnElement) return;
        
        // Disable button during action
        btnElement.style.opacity = '0.6';
        btnElement.style.pointerEvents = 'none';
        
        try {
            const response = await this.postAPI(`/api/equipment/buttons/${buttonId}`);
            
            // Update button state
            if (response) {
                const button = this.buttons.get(buttonId);
                if (button) {
                    button.status = response.status;
                }
                
                if (statusElement) {
                    statusElement.textContent = response.status;
                }
                
                // Update visual state
                btnElement.classList.toggle('active', this.getButtonStatusClass(response.status) === 'active');
                
                this.showNotification('Action Complete', `${response.label || 'Device'} is now ${response.status}`, 'success');
            }
        } catch (error) {
            console.error('Button action failed:', error);
        } finally {
            // Re-enable button
            btnElement.style.opacity = '1';
            btnElement.style.pointerEvents = 'auto';
        }
    }

    // ==================== WebSocket ====================
    
    connectWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws/equipment`;
        
        console.log('Connecting to WebSocket:', wsUrl);
        
        try {
            this.wsSocket = new WebSocket(wsUrl);
            
            this.wsSocket.onopen = () => {
                console.log('WebSocket connected');
                this.reconnectAttempts = 0;
                this.updateConnectionStatus(true);
                this.hideSystemBanner();
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
                console.log('WebSocket disconnected');
                this.updateConnectionStatus(false);
                this.attemptReconnect();
            };
        } catch (error) {
            console.error('WebSocket connection error:', error);
            this.updateConnectionStatus(false);
            this.attemptReconnect();
        }
    }

    handleWebSocketMessage(data) {
        if (!data || !data.type) return;
        
        console.log('WebSocket message:', data.type, data.payload);
        
        switch (data.type) {
            case 'TemperatureUpdate':
                this.handleTemperatureUpdate(data.payload);
                break;
            case 'ChemistryUpdate':
                this.handleChemistryUpdate(data.payload);
                break;
            case 'SystemStatusChange':
                this.handleSystemStatusChange(data.payload);
                break;
            case 'ButtonStateChange':
                this.handleButtonStateChange(data.payload);
                break;
            default:
                console.log('Unknown WebSocket message type:', data.type);
        }
        
        this.updateLastUpdateTime();
    }

    handleTemperatureUpdate(payload) {
        if (payload.pool_temp) {
            this.updateTemperature('poolTemp', payload.pool_temp);
        }
        if (payload.spa_temp) {
            this.updateTemperature('spaTemp', payload.spa_temp);
        }
        if (payload.air_temp) {
            this.updateTemperature('airTemp', payload.air_temp);
        }
    }

    handleChemistryUpdate(payload) {
        if (payload.ph) {
            this.updateChemistry('phLevel', payload.ph, 7.2, 7.8, 'phProgress');
        }
        if (payload.orp) {
            this.updateChemistry('orpLevel', payload.orp, 650, 750, 'orpProgress');
        }
        if (payload.salt_ppm) {
            this.updateChemistry('saltLevel', payload.salt_ppm, 2700, 3400, 'saltProgress');
        }
    }

    handleSystemStatusChange(payload) {
        console.log('System status change:', payload);
        
        const newStatus = payload.status || 'unknown';
        this.updateSystemStatus(newStatus);
        
        // If system just became operational and we don't have buttons yet, try loading them
        if (newStatus === 'operational' && this.buttons.size === 0) {
            console.log('System became operational, loading buttons...');
            this.buttonLoadRetries = 0; // Reset retry counter
            this.loadButtons();
        }
    }

    handleButtonStateChange(payload) {
        if (payload.button_id && payload.status) {
            const button = this.buttons.get(payload.button_id);
            if (button) {
                button.status = payload.status;
                const statusElement = document.getElementById(`status-${payload.button_id}`);
                if (statusElement) {
                    statusElement.textContent = payload.status;
                }
                const btnElement = document.getElementById(`btn-${payload.button_id}`);
                if (btnElement) {
                    btnElement.classList.toggle('active', this.getButtonStatusClass(payload.status) === 'active');
                }
            }
        }
    }

    attemptReconnect() {
        if (this.reconnectAttempts >= this.maxReconnectAttempts) {
            console.error('Max reconnection attempts reached');
            this.showSystemBanner('Connection Lost', 'Unable to connect to server. Please refresh the page.', 'danger');
            return;
        }
        
        this.reconnectAttempts++;
        const delay = this.reconnectDelay * this.reconnectAttempts;
        
        console.log(`Attempting to reconnect in ${delay}ms (attempt ${this.reconnectAttempts}/${this.maxReconnectAttempts})`);
        this.showSystemBanner('Connection Lost', `Reconnecting in ${delay / 1000} seconds...`, 'warning');
        
        setTimeout(() => {
            this.connectWebSocket();
        }, delay);
    }

    // ==================== UI Helpers ====================
    
    updateConnectionStatus(connected) {
        const statusDot = document.getElementById('connectionStatus');
        const statusText = document.getElementById('connectionText');
        
        if (statusDot) {
            statusDot.className = 'status-dot';
            if (connected) {
                statusDot.classList.add('connected');
            } else {
                statusDot.classList.add('disconnected');
            }
        }
        
        if (statusText) {
            statusText.textContent = connected ? 'Connected' : 'Disconnected';
        }
    }

    updateLastUpdateTime() {
        this.lastUpdate = new Date();
        const timeElement = document.getElementById('lastUpdateTime');
        if (timeElement) {
            const timeStr = this.lastUpdate.toLocaleTimeString();
            timeElement.innerHTML = `<i class="bi bi-clock me-1"></i>Updated: ${timeStr}`;
        }
    }

    showSystemBanner(title, message, type = 'info') {
        const banner = document.getElementById('systemStatusBanner');
        const titleElement = document.getElementById('bannerTitle');
        const messageElement = document.getElementById('bannerMessage');
        
        if (banner && titleElement && messageElement) {
            titleElement.textContent = title;
            messageElement.textContent = message;
            
            banner.className = 'alert alert-dismissible fade show';
            banner.classList.add(`alert-${type}`);
            banner.style.display = 'block';
        }
    }

    hideSystemBanner() {
        const banner = document.getElementById('systemStatusBanner');
        if (banner) {
            banner.style.display = 'none';
        }
    }

    showNotification(title, message, type = 'info') {
        const toast = document.getElementById('notificationToast');
        const toastTitle = document.getElementById('toastTitle');
        const toastMessage = document.getElementById('toastMessage');
        const toastTime = document.getElementById('toastTime');
        
        if (toast && toastTitle && toastMessage) {
            toastTitle.textContent = title;
            toastMessage.textContent = message;
            toastTime.textContent = new Date().toLocaleTimeString();
            
            const toastBootstrap = new bootstrap.Toast(toast);
            toastBootstrap.show();
        }
    }

    // ==================== Event Listeners ====================
    
    setupEventListeners() {
        // Refresh buttons
        const refreshBtn = document.getElementById('refreshButtons');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => {
                this.loadButtons();
            });
        }
        
        // Handle page visibility change
        document.addEventListener('visibilitychange', () => {
            if (document.hidden) {
                console.log('Page hidden');
            } else {
                console.log('Page visible - refreshing data');
                this.loadEquipmentData();
            }
        });
    }

    // ==================== Periodic Updates ====================
    
    startPeriodicUpdates() {
        // Refresh equipment data every 30 seconds as fallback
        setInterval(() => {
            if (!this.wsSocket || this.wsSocket.readyState !== WebSocket.OPEN) {
                console.log('WebSocket not connected, fetching data via API');
                this.loadEquipmentData();
            }
        }, 30000);
        
        // Update time display every second
        setInterval(() => {
            const now = new Date();
            const timeElement = document.getElementById('systemDateTime');
            if (timeElement) {
                timeElement.textContent = now.toLocaleTimeString();
            }
        }, 1000);
    }
}

// Initialize when DOM is ready
let poolMonitor;
document.addEventListener('DOMContentLoaded', () => {
    poolMonitor = new PoolMonitor();
});
