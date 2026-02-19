/**
 * Equipment Button — Toggle button component logic with command feedback
 *
 * Button status values from the API (magic_enum string representations):
 *   Auxillary: On, Off, Enabled, Pending, Unknown
 *   Pump:      Off, Running, NotInstalled, Unknown
 *   Heater:    Off, Heating, Enabled, NotInstalled, Unknown
 *   Chlor:     Off, Running, Unknown
 *
 * Icon resolution order:
 *   1. localStorage override (keyed by button id)
 *   2. device_type trait from backend (if present)
 *   3. Label keyword matching (fallback)
 */

// Device-type to icon mapping
const _deviceTypeIcons = {
    'light':        '\u{1F4A1}',
    'pump':         '\u2699\uFE0F',
    'filter_pump':  '\u{1F504}',
    'heater':       '\u{1F525}',
    'spa':          '\u{1F6C1}',
    'chlorinator':  '\u{1F9C2}',
    'aquapure':     '\u{1F9C2}',
    'cleaner':      '\u{1F9F9}',
    'jet':          '\u{1F4A8}',
    'spillway':     '\u{1F30A}',
    'valve':        '\u{1F527}'
};

// Load icon overrides once
const _iconOverrides = JSON.parse(localStorage.getItem('iconOverrides') || '{}');

function equipmentButton() {
    return {
        isActive(button) {
            const s = String(button.status || '').toLowerCase();
            return s === 'on' || s === 'enabled' || s === 'running' || s === 'heating';
        },

        statusLabel(button) {
            return String(button.status || 'Unknown');
        },

        cmdState(button) {
            return Alpine.store('pool').getCommandState(button.id);
        },

        icon(button) {
            // 1. localStorage override
            if (_iconOverrides[button.id]) return _iconOverrides[button.id];

            // 2. device_type trait from backend
            if (button.device_type) {
                const dtIcon = _deviceTypeIcons[button.device_type.toLowerCase()];
                if (dtIcon) return dtIcon;
            }

            // 3. Label keyword fallback
            const label = String(button.label || '').toLowerCase();
            if (label.includes('light')) return '\u{1F4A1}';
            if (label.includes('filter')) return '\u{1F504}';
            if (label.includes('pump')) return '\u2699\uFE0F';
            if (label.includes('heat')) return '\u{1F525}';
            if (label.includes('spa')) return '\u{1F6C1}';
            if (label.includes('chlor') || label.includes('aquapure')) return '\u{1F9C2}';
            if (label.includes('pool')) return '\u{1F3CA}';
            if (label.includes('jet')) return '\u{1F4A8}';
            if (label.includes('clean')) return '\u{1F9F9}';
            if (label.includes('valve')) return '\u{1F527}';
            if (label.includes('spill')) return '\u{1F30A}';
            return '\u26A1';
        },

        async toggle(button) {
            await Alpine.store('pool').toggleButton(button.id);
        }
    };
}
