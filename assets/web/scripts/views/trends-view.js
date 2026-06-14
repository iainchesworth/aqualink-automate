/**
 * Trends View — history charts (WS2).
 *
 * As in diagnostics-view, the Chart.js instance is kept OUTSIDE Alpine's
 * reactive proxy: Chart.js introspects its own (proxied) properties and a
 * proxied chart recurses infinitely. `_trends.chart` therefore lives module-side.
 */

const _trends = { chart: null };

const TREND_RANGES = [
    { label: '1h', seconds: 3600 },
    { label: '24h', seconds: 86400 },
    { label: '7d', seconds: 604800 },
    { label: '30d', seconds: 2592000 },
];

// Stable colours for the well-known series; anything else cycles a palette.
const TREND_COLORS = {
    'temp/pool': '#0ea5e9',
    'temp/spa': '#f97316',
    'temp/air': '#10b981',
    'chem/salt_ppm': '#eab308',
    'chem/ph': '#a855f7',
    'chem/orp': '#ec4899',
    'swg/percent': '#38bdf8',
};
const TREND_PALETTE = ['#0ea5e9', '#f97316', '#10b981', '#eab308', '#a855f7', '#ec4899', '#38bdf8', '#94a3b8'];

function _trendColor(key, index) {
    return TREND_COLORS[key] || TREND_PALETTE[index % TREND_PALETTE.length];
}

// Friendly display name for a series key (the wire keys like
// "device/air_blower/state" are structured but not pretty to read).
const TREND_LABELS = {
    'temp/pool': 'Pool Temp', 'temp/spa': 'Spa Temp', 'temp/air': 'Air Temp',
    'chem/salt_ppm': 'Salt', 'chem/ph': 'pH', 'chem/orp': 'ORP', 'swg/percent': 'SWG Output',
};
function _trendLabel(key) {
    if (TREND_LABELS[key]) return TREND_LABELS[key];
    // device/<name>/state -> "Name"; otherwise drop the category and any /state suffix.
    const m = key.match(/^device\/(.+)\/state$/);
    const base = m ? m[1] : key.replace(/^[^/]+\//, '').replace(/\/state$/, '');
    return base.split(/[_/]/)
        .map((w) => w.charAt(0).toUpperCase() + w.slice(1))
        .join(' ') || key;
}

function trendsView() {
    return {
        available: true,     // false when /api/history/series returns 503
        loading: false,
        error: '',
        series: [],          // [{ key, unit, first_ts, last_ts, count }]
        selected: {},        // key -> bool
        rangeSeconds: 86400,
        ranges: TREND_RANGES,
        _loaded: false,

        // Lazily load the first time the Trends route is shown, then re-render on
        // subsequent shows so the window stays fresh.
        onShown() {
            if (this._loaded) { this.render(); return; }
            this._loaded = true;
            this.loadSeries();
        },

        async loadSeries() {
            this.loading = true;
            this.error = '';
            try {
                const resp = await fetch('/api/history/series');
                if (resp.status === 503) { this.available = false; return; }
                this.available = true;
                if (!resp.ok) { this.error = 'Failed to load history series.'; return; }

                this.series = await resp.json();
                if (Object.keys(this.selected).length === 0) {
                    // Default-select temperatures + salt.
                    this.series.forEach((s) => {
                        this.selected[s.key] = s.key.startsWith('temp/') || s.key === 'chem/salt_ppm';
                    });
                }
                await this.render();
            } catch (e) {
                this.error = 'Failed to load history series.';
            } finally {
                this.loading = false;
            }
        },

        seriesLabel(key) {
            return _trendLabel(key);
        },

        setRange(seconds) {
            this.rangeSeconds = seconds;
            this.render();
        },

        toggleSeries(key) {
            this.selected[key] = !this.selected[key];
            this.render();
        },

        async render() {
            if (!this.available) return;

            const to = Math.floor(Date.now() / 1000);
            const from = to - this.rangeSeconds;
            const keys = this.series.filter((s) => this.selected[s.key]).map((s) => s.key);

            const datasets = [];
            for (let i = 0; i < keys.length; i++) {
                const key = keys[i];
                try {
                    const resp = await fetch(
                        `/api/history/series?key=${encodeURIComponent(key)}&from=${from}&to=${to}&max_points=500`,
                    );
                    if (!resp.ok) continue;
                    const body = await resp.json();
                    datasets.push({
                        label: _trendLabel(key),
                        data: (body.points || []).map((p) => ({ x: p.ts * 1000, y: p.value })),
                        borderColor: _trendColor(key, i),
                        backgroundColor: 'transparent',
                        borderWidth: 2,
                        pointRadius: 0,
                        tension: 0.25,
                    });
                } catch (_) { /* skip a failed series */ }
            }

            this._draw(datasets);
        },

        _draw(datasets) {
            const canvas = this.$refs.trendsCanvas;
            if (!canvas || typeof Chart === 'undefined') return;

            if (_trends.chart) {
                _trends.chart.data.datasets = datasets;
                _trends.chart.update('none');
                return;
            }

            _trends.chart = new Chart(canvas.getContext('2d'), {
                type: 'line',
                data: { datasets },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    parsing: false,
                    animation: false,
                    interaction: { mode: 'nearest', intersect: false },
                    scales: {
                        x: { type: 'time', ticks: { maxTicksLimit: 8 } },
                        y: { beginAtZero: false },
                    },
                    plugins: { legend: { display: true, position: 'bottom' } },
                },
            });
        },
    };
}
