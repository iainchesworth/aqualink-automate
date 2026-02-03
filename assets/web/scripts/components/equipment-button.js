/**
 * Equipment Button — Toggle button component logic
 *
 * Button status values from the API (magic_enum string representations):
 *   Auxillary: On, Off, Enabled, Pending, Unknown
 *   Pump:      Off, Running, NotInstalled, Unknown
 *   Heater:    Off, Heating, Enabled, NotInstalled, Unknown
 *   Chlor:     Off, Running, Unknown
 */
function equipmentButton() {
    return {
        pressing: false,

        isActive(button) {
            const s = String(button.status || '').toLowerCase();
            return s === 'on' || s === 'enabled' || s === 'running' || s === 'heating';
        },

        statusLabel(button) {
            return String(button.status || 'Unknown');
        },

        icon(button) {
            const label = String(button.label || '').toLowerCase();
            if (label.includes('light')) return '\u{1F4A1}';
            if (label.includes('pump') || label.includes('filter')) return '\u2699\uFE0F';
            if (label.includes('heat')) return '\u{1F525}';
            if (label.includes('spa')) return '\u{1F6C1}';
            if (label.includes('pool')) return '\u{1F3CA}';
            if (label.includes('jet')) return '\u{1F4A8}';
            if (label.includes('clean')) return '\u{1F9F9}';
            if (label.includes('valve')) return '\u{1F527}';
            return '\u26A1';
        },

        async toggle(button) {
            this.pressing = true;
            await Alpine.store('pool').toggleButton(button.id);
            this.pressing = false;
        }
    };
}
