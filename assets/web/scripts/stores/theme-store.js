/**
 * Theme Store — Dark/light mode management
 */
document.addEventListener('alpine:init', () => {
    Alpine.store('theme', {
        isDark: false,

        init() {
            const saved = localStorage.getItem('theme');
            if (saved) {
                this.isDark = saved === 'dark';
            } else {
                this.isDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
            }

            window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', (e) => {
                if (!localStorage.getItem('theme')) {
                    this.isDark = e.matches;
                }
            });
        },

        toggle() {
            this.isDark = !this.isDark;
            localStorage.setItem('theme', this.isDark ? 'dark' : 'light');
        }
    });
});
