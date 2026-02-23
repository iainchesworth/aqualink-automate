/**
 * About View — Version info component
 */
function aboutView() {
    return {
        get shortCommit() {
            const sha = Alpine.store('pool').gitCommit;
            return sha ? sha.substring(0, 7) : '--';
        }
    };
}
