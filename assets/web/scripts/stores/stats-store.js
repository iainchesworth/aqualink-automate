/**
 * Stats Store — Diagnostics data from /ws/equipment/stats
 *
 * StatisticsUpdate payload:
 *   message_counts: [{ id, name, count }]
 *   bandwidth_read: { total_bytes, average_utilisation_1sec, average_utilisation_30sec, average_utilisation_5mins }
 *   bandwidth_write: { total_bytes, average_utilisation_1sec, average_utilisation_30sec, average_utilisation_5mins }
 *   latency: { serial_read, serial_write, message_processing } — each has p1/p50/p95/p99/min/max/mean_us, sample_count
 *   serial: { message_error_rate, overflow_count, underflow_count, transmission_failures, write_queue_depth }
 *   message_errors: { checksum_failures, deserialization_failures, invalid_packet_format, generator_failures, overlapping_packets, buffer_overflows }
 */

// Chart history kept outside Alpine's reactive proxy to avoid Chart.js conflicts.
// Throttled to ~2 samples/sec so the buffer covers long time windows.
const _chartHistory = {
    times: [],
    reads: [],
    writes: [],
    maxDataPoints: 1800,   // 2/sec × 900s (15 min) — covers "All" generously
    minIntervalMs: 500,    // at most 2 chart points per second
    lastPushTime: 0
};

// Frequency tracking kept outside Alpine for the same reason.
// Uses exponential moving average: freq = alpha * instantaneous + (1-alpha) * prev
// alpha = 1 - exp(-dt / tau), tau = smoothing time constant in seconds.
const _freq = {
    prevCounts: {},
    prevFreqs: {},
    lastSeen: {},
    prevTime: null,
    tau: 3 // seconds — controls how quickly the EMA adapts
};

// Expose for diagnostics-view.js to read directly.
window.__statsChartHistory = _chartHistory;

document.addEventListener('alpine:init', () => {
    Alpine.store('stats', {
        messageCounts: [],
        bandwidthRead: null,
        bandwidthWrite: null,
        latency: null,
        serial: null,
        messageErrors: null,
        lastUpdate: null,

        handleEvent(msg) {
            if (!msg || msg.type !== 'StatisticsUpdate' || !msg.payload) return;
            this.lastUpdate = new Date();

            const p = msg.payload;

            if (p.bandwidth_read) {
                this.bandwidthRead = p.bandwidth_read;
            }
            if (p.bandwidth_write) {
                this.bandwidthWrite = p.bandwidth_write;
            }
            if (p.latency) {
                this.latency = p.latency;
            }
            if (p.serial) {
                this.serial = p.serial;
            }
            if (p.message_errors) {
                this.messageErrors = p.message_errors;
            }

            // Append to plain (non-reactive) chart history arrays (timestamps as ms).
            // Throttled so high-frequency updates don't exhaust the buffer too quickly.
            const h = _chartHistory;
            const nowChart = Date.now();
            if (nowChart - h.lastPushTime >= h.minIntervalMs) {
                h.lastPushTime = nowChart;
                h.times.push(nowChart);
                h.reads.push(p.bandwidth_read?.average_utilisation_1sec || 0);
                h.writes.push(p.bandwidth_write?.average_utilisation_1sec || 0);

                if (h.times.length > h.maxDataPoints) {
                    h.times.shift();
                    h.reads.shift();
                    h.writes.shift();
                }
            }

            // Update message counts with computed frequency
            if (p.message_counts) {
                const nowMs = Date.now();
                const dtSec = _freq.prevTime ? (nowMs - _freq.prevTime) / 1000 : 0;

                const alpha = dtSec > 0 ? 1 - Math.exp(-dtSec / _freq.tau) : 0;

                const enriched = p.message_counts.map(stat => {
                    const key = stat.name || stat.id;
                    const prev = _freq.prevCounts[key];
                    let frequency = _freq.prevFreqs[key] || 0;
                    if (prev != null && dtSec > 0) {
                        const instantaneous = (stat.count - prev) / dtSec;
                        frequency = alpha * instantaneous + (1 - alpha) * frequency;
                    }
                    // Only update lastSeen when the count has actually increased
                    if (stat.count > 0 && (prev == null || stat.count > prev)) {
                        _freq.lastSeen[key] = nowMs;
                    }
                    _freq.prevCounts[key] = stat.count;
                    _freq.prevFreqs[key] = frequency;
                    return { ...stat, frequency, lastSeen: _freq.lastSeen[key] || null };
                });

                _freq.prevTime = nowMs;
                enriched.sort((a, b) => (a.name || '').localeCompare(b.name || ''));
                this.messageCounts.splice(0, this.messageCounts.length, ...enriched);
            }

            // Dispatch custom event for chart updates
            window.dispatchEvent(new CustomEvent('stats-updated'));
        }
    });
});
