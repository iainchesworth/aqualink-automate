/**
 * Pool Graphic — SVG pool visualization logic
 */
function poolGraphic() {
    // Active status values (case-insensitive): on, running, heating, enabled
    const isActiveStatus = (status) => {
        const s = String(status).toLowerCase();
        return s === 'on' || s === 'running' || s === 'heating' || s === 'enabled';
    };

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
            const btns = Alpine.store('pool').buttons;
            const spa = btns.find(b => b.label && b.label.toLowerCase().includes('spa'));
            return spa && isActiveStatus(spa.status);
        },
        get pumpActive() {
            const btns = Alpine.store('pool').buttons;
            const pump = btns.find(b => b.label && (b.label.toLowerCase().includes('pump') || b.label.toLowerCase().includes('filter')));
            return pump && isActiveStatus(pump.status);
        },
        get heaterActive() {
            const btns = Alpine.store('pool').buttons;
            const heater = btns.find(b => b.label && b.label.toLowerCase().includes('heat'));
            return heater && isActiveStatus(heater.status);
        },
        get chlorinatorActive() {
            const btns = Alpine.store('pool').buttons;
            const chlorinator = btns.find(b => b.label && (
                b.label.toLowerCase().includes('chlor') ||
                b.label.toLowerCase().includes('aquapure') ||
                b.label.toLowerCase().includes('salt')
            ));
            return chlorinator && isActiveStatus(chlorinator.status);
        },
        get hasChlorinator() {
            const btns = Alpine.store('pool').buttons;
            return btns.some(b => b.label && (
                b.label.toLowerCase().includes('chlor') ||
                b.label.toLowerCase().includes('aquapure') ||
                b.label.toLowerCase().includes('salt')
            ));
        }
    };
}
