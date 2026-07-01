/**
 * Accent Store — accent-colour selection (teal / azure / aqua / violet).
 *
 * Mirrors theme-store: a client preference persisted to localStorage and
 * surfaced on the root element as `data-accent`, which drives the
 * `:root[data-accent="..."]` token overrides in app.css. The default
 * (teal) needs no attribute rule — it is the base `--accent`.
 */
document.addEventListener('alpine:init', () => {
    Alpine.store('accent', {
        name: 'teal',
        options: ['teal', 'azure', 'aqua', 'violet'],

        init() {
            const saved = localStorage.getItem('accent');
            if (saved && this.options.includes(saved)) {
                this.name = saved;
            }
        },

        set(name) {
            if (!this.options.includes(name)) return;
            this.name = name;
            localStorage.setItem('accent', name);
        }
    });
});
