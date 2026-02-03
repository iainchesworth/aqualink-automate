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
    windowSeconds: 60
};

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

        initChart() {
            Alpine.store('ws').connectStats();

            if (!_diag.chart) {
                this.$nextTick(() => this._createChart());
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

            if (_diag.statsListener) {
                window.removeEventListener('stats-updated', _diag.statsListener);
                _diag.statsListener = null;
            }

            if (_diag.chart) {
                _diag.chart.destroy();
                _diag.chart = null;
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
