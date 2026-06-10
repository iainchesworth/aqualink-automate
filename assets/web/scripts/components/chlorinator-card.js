/**
 * Chlorinator / Chemistry Card
 *
 * Surfaces the salt-water generator (SWG) output % + salt PPM now, and is
 * structured for future ORP / pH sensors: ORP and pH always render as labelled
 * placeholders ("--") until a value arrives, so a newly-decoded sensor shows up
 * automatically with no template change.
 *
 * Data is sourced from the shared pool store (populated by the ~2s equipment
 * poller from /api/equipment chemistry.{salt_ppm,orp_mv,ph,chlorinator{...}}).
 */
function chlorinatorCard() {
    return {
        get present() {
            return Alpine.store('pool').chlorinatorPresent === true;
        },

        get generatingPercent() {
            const v = Alpine.store('pool').swgGeneratingPercent;
            return (v === '--' || v == null) ? '--' : (v + '%');
        },

        /** Numeric 0–100 for the output bar (0 when no reading). */
        get percent() {
            const v = Alpine.store('pool').swgGeneratingPercent;
            const n = (v === '--' || v == null) ? NaN : Number(v);
            return isNaN(n) ? 0 : Math.max(0, Math.min(100, n));
        },

        get saltPpm() {
            const v = Alpine.store('pool').saltPpm;
            return (v === '--' || v == null) ? '--' : (v + ' ppm');
        },

        get orp() {
            const v = Alpine.store('pool').orp;
            return (v === '--' || v == null) ? '--' : (v + ' mV');
        },

        get ph() {
            const v = Alpine.store('pool').ph;
            return (v === '--' || v == null) ? '--' : String(v);
        },

        get statusLabel() {
            const s = Alpine.store('pool').chlorinatorStatus;
            return (s && s !== '--') ? s : '--';
        },

        get healthLabel() {
            const h = Alpine.store('pool').chlorinatorHealth;
            if (!h || h === '--') return '--';
            // Convert the magic_enum names (e.g. 'Warning_LowSalt') to a friendly label.
            return String(h).replace(/_/g, ' ');
        },

        /** Colour the SWG % by the chlorinator health: ok=good, warning=warn, error=bad. */
        get healthColor() {
            const h = String(Alpine.store('pool').chlorinatorHealth || '');
            if (h === 'Ok' || h === 'TurningOff') return 'var(--gauge-good)';
            if (h.startsWith('Warning')) return 'var(--gauge-warn)';
            if (h.startsWith('Error') || h === 'GeneralFault') return 'var(--gauge-bad)';
            return 'var(--text-muted)';
        },

        get healthBand() {
            const h = String(Alpine.store('pool').chlorinatorHealth || '');
            if (h === 'Ok' || h === 'TurningOff') return 'good';
            if (h.startsWith('Warning')) return 'okay';
            if (h.startsWith('Error') || h === 'GeneralFault') return 'bad';
            return null;
        }
    };
}
