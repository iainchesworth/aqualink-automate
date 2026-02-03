/**
 * Pool Graphic — SVG pool visualization logic
 */
function poolGraphic() {
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
            return spa && String(spa.status).toLowerCase() === 'on';
        },
        get pumpActive() {
            const btns = Alpine.store('pool').buttons;
            const pump = btns.find(b => b.label && (b.label.toLowerCase().includes('pump') || b.label.toLowerCase().includes('filter')));
            return pump && String(pump.status).toLowerCase() === 'on';
        },
        get heaterActive() {
            const btns = Alpine.store('pool').buttons;
            const heater = btns.find(b => b.label && b.label.toLowerCase().includes('heat'));
            return heater && String(heater.status).toLowerCase() === 'on';
        }
    };
}
