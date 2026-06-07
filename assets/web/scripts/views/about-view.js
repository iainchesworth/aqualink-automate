/**
 * About View — Version info component
 */
function aboutView() {
    return {
        get shortCommit() {
            const sha = Alpine.store('pool').gitCommit;
            return sha ? sha.substring(0, 7) : '--';
        },

        get safeHomepage() {
            const url = Alpine.store('pool').projectHomepage;
            if (!url) return '';
            try {
                const parsed = new URL(url);
                if (parsed.protocol === 'http:' || parsed.protocol === 'https:') return url;
            } catch { /* invalid URL */ }
            return '';
        }
    };
}
