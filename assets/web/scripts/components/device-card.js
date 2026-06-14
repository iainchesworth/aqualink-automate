/**
 * Device Card — shared presenter for the Diagnostics "Emulated Devices" and
 * "Actual Devices" sections.
 *
 * ONE component renders every device type and BOTH sections (see the
 * deviceGroups() loop in index.html); the per-type tailoring — icon, role,
 * "what it's doing" summary, the detail rows and the live-activity indicator —
 * lives in the REGISTRY below, keyed by device_type. This is the single source
 * of truth, so the two sections can never drift: they differ only by which
 * device list is bound.
 *
 * The card is a pure presenter over the `dev` object delivered by the
 * /api/diagnostics/{emulated,actual}-devices pollers. It owns no polling and
 * issues no commands. All values are rendered with x-text (auto-escaped).
 */
function deviceCard() {
    const yn = (v) => (v ? 'Yes' : 'No');
    const orDash = (v) => (v === undefined || v === null || v === '' ? '--' : String(v));
    // "Busy" if the state exists and isn't one of the listed idle/terminal states.
    const isBusy = (state, idleStates) => !!state && !idleStates.includes(String(state));

    function fmtAge(secs) {
        if (secs === undefined || secs === null) { return ''; }
        const s = Number(secs);
        if (!Number.isFinite(s)) { return ''; }
        if (s < 60) { return `${s}s ago`; }
        if (s < 3600) { return `${Math.floor(s / 60)}m ago`; }
        return `${Math.floor(s / 3600)}h ago`;
    }

    // Per-device-type presentation:
    //   summary(d)  -> one-line "what it's doing"
    //   sections(d) -> [{ label, rows: [[key, value], ...] }]
    //   active(d)   -> whether the live-activity dot should pulse
    const REGISTRY = {
        OneTouch: {
            icon: 'onetouch',
            role: 'OneTouch menu controller',
            summary(d) {
                if (d.spider_engine && isBusy(d.spider_engine.state, ['Idle', 'Complete', 'Done'])) {
                    return `Surveying menus (${orDash(d.spider_engine.visited_count)} pages visited)`;
                }
                if (d.navigator && isBusy(d.navigator.state, ['Idle', 'Synced'])) {
                    return `Navigating to ${orDash(d.navigator.target_page)}`;
                }
                if (d.navigator) { return `Idle on ${orDash(d.navigator.current_page)}`; }
                return 'Monitoring the controller';
            },
            sections(d) {
                const out = [];
                if (d.navigator) {
                    out.push({ label: 'Navigator', rows: [
                        ['State', orDash(d.navigator.state)],
                        ['Current', orDash(d.navigator.current_page)],
                        ['Target', orDash(d.navigator.target_page)],
                        ['Cursor', orDash(d.navigator.cursor_line)],
                        ['Synced', yn(d.navigator.synced)],
                    ] });
                }
                if (d.spider_engine) {
                    out.push({ label: 'Spider Engine', rows: [
                        ['State', orDash(d.spider_engine.state)],
                        ['Visited', orDash(d.spider_engine.visited_count)],
                        ['Target', orDash(d.spider_engine.current_target)],
                    ] });
                }
                out.push({ label: 'Scraping', rows: [
                    ['Stall Counter', orDash(d.scraping_stall_counter)],
                    ['Highlighted', orDash(d.highlighted_line)],
                    ['Key Cmd', orDash(d.pending_key_command)],
                    ['ACK Type', orDash(d.ack_type)],
                ] });
                return out;
            },
            active(d) {
                return (d.spider_engine && isBusy(d.spider_engine.state, ['Idle', 'Complete', 'Done']))
                    || (d.navigator && isBusy(d.navigator.state, ['Idle', 'Synced']))
                    || d.operating_state === 'Scraping';
            },
        },

        IAQ: {
            icon: 'iaq',
            role: 'AqualinkTouch panel',
            summary(d) {
                if (d.awaiting_control_ready) { return 'Waiting for control-ready'; }
                if ((d.command_queue_depth || 0) > 0) { return `Sending command (queue ${d.command_queue_depth})`; }
                return 'Monitoring the panel';
            },
            sections(d) {
                return [{ label: 'Command', rows: [
                    ['Pending Cmd', orDash(d.pending_command)],
                    ['Queue Depth', orDash(d.command_queue_depth)],
                    ['Awaiting Ctrl-Ready', yn(d.awaiting_control_ready)],
                    ['Control Data', orDash(d.control_data_value)],
                ] }];
            },
            active(d) { return d.awaiting_control_ready === true || (d.command_queue_depth || 0) > 0; },
        },

        SerialAdapter: {
            icon: 'serial',
            role: 'RS Serial Adapter',
            summary(d) {
                if (d.has_pending_command) { return 'Dispatching a command'; }
                return `Decoding status (${orDash(d.status_collection_count)} types)`;
            },
            sections(d) {
                return [{ label: 'Decoder', rows: [
                    ['Status Types', orDash(d.status_collection_count)],
                    ['Status Received', yn(d.status_message_received)],
                    ['Pending Cmd', yn(d.has_pending_command)],
                    ['Pending Count', orDash(d.pending_command_count)],
                ] }];
            },
            active(d) { return d.has_pending_command === true; },
        },

        PDA: {
            icon: 'pda',
            role: 'PDA menu controller',
            summary(d) { return `Scraping: ${orDash(d.scrape_state)}`; },
            sections(d) {
                return [{ label: 'Scraping', rows: [['Scrape State', orDash(d.scrape_state)]] }];
            },
            active(d) { return isBusy(d.scrape_state, ['Idle', 'Done', 'Complete']); },
        },

        Keypad: {
            icon: 'keypad',
            role: 'RS keypad',
            summary(d) {
                return (d.screen && d.screen.page_type) ? `Showing ${d.screen.page_type}` : 'Mirroring the keypad';
            },
            sections() { return []; },
            active() { return false; },
        },

        SpasideRemote: {
            icon: 'remote',
            role: 'Spa-side remote',
            summary(d) {
                if (d.last_button > 0) { return `Last button ${d.last_button}`; }
                return 'Watching for button presses';
            },
            sections(d) {
                return [{ label: 'Activity', rows: [
                    ['Poll Count', orDash(d.poll_count)],
                    ['Last Button', orDash(d.last_button)],
                    ['Last Seen', d.last_button_age_seconds == null ? '--' : fmtAge(d.last_button_age_seconds)],
                    ['LEDs Seen', yn(d.led_image_seen)],
                ] }];
            },
            active(d) { return d.last_button_age_seconds != null && d.last_button_age_seconds < 5; },
        },
    };

    const FALLBACK = {
        icon: 'power',
        role: 'Device',
        summary() { return ''; },
        sections() { return []; },
        active() { return false; },
    };

    return {
        cfg(d) { return (d && REGISTRY[d.device_type]) || FALLBACK; },
        iconKey(d) { return this.cfg(d).icon; },
        role(d) { return this.cfg(d).role; },
        summary(d) { return this.cfg(d).summary(d); },
        sections(d) { return this.cfg(d).sections(d); },
        isActive(d) { return !!this.cfg(d).active(d); },

        // Emulation posture badge: active emulator vs passive decoder vs real device.
        emuLabel(d) {
            if (!d.is_emulated) { return 'Real device'; }
            return d.emulation_suppressed ? 'Passive decoder' : 'Active emulator';
        },
        emuClass(d) {
            if (!d.is_emulated) { return 'real'; }
            return d.emulation_suppressed ? 'passive' : 'active';
        },

        // Mirrors diagnosticsView.operatingStateClass so the card is self-contained.
        stateClass(state) {
            switch (state) {
                case 'NormalOperation':
                case 'Scraping':
                    return 'badge-status-normal';
                case 'StartUp':
                case 'ColdStart':
                    return 'badge-status-warn';
                case 'FaultHasOccurred':
                case 'ScrapingFaulted':
                    return 'badge-status-danger';
                default:
                    return '';
            }
        },

        // Recent commands, newest first, capped to keep the card compact.
        recentCommands(d) {
            const list = Array.isArray(d.recent_commands) ? d.recent_commands : [];
            return list.slice().reverse().slice(0, 8);
        },
        fmtAge(secs) { return fmtAge(secs); },
        cmdOutcomeClass(outcome) { return outcome === 'Success' ? 'ok' : 'err'; },

        rawJson(d) { return JSON.stringify(d, null, 2); },
    };
}
