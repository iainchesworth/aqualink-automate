/**
 * Toast Store — Lightweight notification system
 *
 * Levels: 'info', 'warn', 'error'
 * Auto-dismisses after a configurable duration.
 * Deduplicates: won't show the same message if one is already visible.
 */
document.addEventListener('alpine:init', () => {
    Alpine.store('toast', {
        items: [],
        _nextId: 1,

        /**
         * Show a toast notification.
         * @param {string} message - Text to display
         * @param {'info'|'warn'|'error'} level - Severity level
         * @param {number} duration - Auto-dismiss in ms (default 5000)
         */
        show(message, level = 'info', duration = 5000) {
            // Deduplicate: skip if same message already visible
            if (this.items.some(t => t.message === message)) return;

            const id = this._nextId++;
            const toast = { id, message, level };
            this.items = [...this.items, toast];

            setTimeout(() => this.dismiss(id), duration);
        },

        dismiss(id) {
            this.items = this.items.filter(t => t.id !== id);
        }
    });
});
