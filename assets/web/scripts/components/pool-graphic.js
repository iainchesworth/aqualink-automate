/**
 * Pool Graphic — SVG pool visualization logic
 */
function poolGraphic() {
    // Shared UI helpers (single source of truth in scripts/config/ui-constants.js).
    // Fall back to local definitions so the graphic still renders if that
    // script is not loaded.
    const ui = (typeof window !== 'undefined' && window.AquaUI) || {};
    const isActiveStatus = ui.isActiveStatus || ((status) => {
        const s = String(status == null ? '' : status).toLowerCase();
        return s === 'on' || s === 'running' || s === 'heating' || s === 'enabled';
    });
    const keywords = ui.DEVICE_KEYWORDS || {
        chlorinator: ['chlor', 'aquapure', 'salt'],
        spa:         ['spa'],
        pump:        ['pump', 'filter'],
        heater:      ['heat']
    };
    const matches = ui.labelMatchesKeywords || ((label, kws) => {
        const l = String(label == null ? '' : label).toLowerCase();
        return kws.some((kw) => l.includes(kw));
    });

    const findButtonByKeywords = (kws) =>
        Alpine.store('pool').buttons.find((b) => b.label && matches(b.label, kws));

    return {
        get poolTempDisplay() {
            return Alpine.store('pool').poolTemp;
        },
        get spaTempDisplay() {
            return Alpine.store('pool').spaTemp;
        },
        get airTempDisplay() {
            return Alpine.store('pool').airTemp;
        },
        get spaActive() {
            const spa = findButtonByKeywords(keywords.spa);
            return spa && isActiveStatus(spa.status);
        },
        get pumpActive() {
            const pump = findButtonByKeywords(keywords.pump);
            return pump && isActiveStatus(pump.status);
        },
        get heaterActive() {
            const heater = findButtonByKeywords(keywords.heater);
            return heater && isActiveStatus(heater.status);
        },
        get chlorinatorActive() {
            const chlorinator = findButtonByKeywords(keywords.chlorinator);
            return chlorinator && isActiveStatus(chlorinator.status);
        },
        get hasChlorinator() {
            return Alpine.store('pool').buttons.some((b) => b.label && matches(b.label, keywords.chlorinator));
        }
    };
}
