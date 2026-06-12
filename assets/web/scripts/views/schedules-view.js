/**
 * Schedules View — time-based automation (WS4).
 *
 * Lists schedules with enable toggles, an add/edit form (day-of-week chips,
 * time, action selector), and delete-with-confirm. Mutating controls are
 * disabled in monitor-only mode, consistent with the dashboard.
 */

const SCHED_DAYS = [
    { bit: 0, label: 'Mon' },
    { bit: 1, label: 'Tue' },
    { bit: 2, label: 'Wed' },
    { bit: 3, label: 'Thu' },
    { bit: 4, label: 'Fri' },
    { bit: 5, label: 'Sat' },
    { bit: 6, label: 'Sun' },
];

const SCHED_ACTION_TYPES = [
    { value: 'button_on', label: 'Turn device ON' },
    { value: 'button_off', label: 'Turn device OFF' },
    { value: 'button_toggle', label: 'Toggle device' },
    { value: 'pool_setpoint', label: 'Pool setpoint' },
    { value: 'spa_setpoint', label: 'Spa setpoint' },
    { value: 'chlorinator_percent', label: 'Chlorinator %' },
    { value: 'circulation_mode', label: 'Circulation mode' },
];

function _schedDefaultForm() {
    return { name: '', enabled: true, days: 0b0011111, time: '08:00', actionType: 'button_toggle', target: '', value: 0 };
}

function _schedIsButton(type) {
    return type === 'button_on' || type === 'button_off' || type === 'button_toggle';
}
function _schedIsValue(type) {
    return type === 'pool_setpoint' || type === 'spa_setpoint' || type === 'chlorinator_percent';
}

function _schedToForm(s) {
    const f = _schedDefaultForm();
    f.name = s.name || '';
    f.enabled = s.enabled !== false;
    f.days = s.days_of_week || 0;
    f.time = s.time_local || '08:00';
    f.actionType = (s.action && s.action.type) || 'button_toggle';
    f.target = (s.action && s.action.target) || '';
    f.value = (s.action && typeof s.action.value === 'number') ? s.action.value : 0;
    return f;
}

function _schedFormToPayload(f) {
    const action = { type: f.actionType };
    if (_schedIsButton(f.actionType) || f.actionType === 'circulation_mode') {
        action.target = f.target;
    } else {
        action.value = Number(f.value);
    }
    return { name: f.name, enabled: f.enabled, days_of_week: f.days, time_local: f.time, action };
}

function schedulesView() {
    return {
        available: true,
        loading: false,
        error: '',
        schedules: [],
        buttons: [],     // device labels for the target dropdown
        editing: null,   // null | 'new' | <uuid>
        form: _schedDefaultForm(),
        days: SCHED_DAYS,
        actionTypes: SCHED_ACTION_TYPES,
        _loaded: false,

        onShown() {
            if (this._loaded) { this.load(); return; }
            this._loaded = true;
            this.load();
        },

        get monitorOnly() {
            try { return Alpine.store('system').monitorOnly; } catch (_) { return false; }
        },

        async load() {
            this.loading = true;
            this.error = '';
            try {
                const resp = await fetch('/api/schedules');
                if (resp.status === 503) { this.available = false; return; }
                this.available = true;
                if (resp.ok) { this.schedules = await resp.json(); }

                try {
                    const b = await fetch('/api/equipment/buttons');
                    if (b.ok) {
                        const bj = await b.json();
                        const list = Array.isArray(bj) ? bj : (bj.buttons || []);
                        this.buttons = list.map((x) => x.label).filter(Boolean);
                    }
                } catch (_) { /* dropdown is best-effort */ }
            } catch (e) {
                this.error = 'Failed to load schedules.';
            } finally {
                this.loading = false;
            }
        },

        isButtonAction(t) { return _schedIsButton(t); },
        isValueAction(t) { return _schedIsValue(t); },

        actionSummary(s) {
            const a = s.action || {};
            if (_schedIsButton(a.type)) { return `${a.type.replace('button_', '')} ${a.target}`; }
            if (a.type === 'circulation_mode') { return `circulation ${a.target}`; }
            return `${a.type} ${a.value}`;
        },

        daySummary(mask) {
            return SCHED_DAYS.filter((d) => (mask & (1 << d.bit)) !== 0).map((d) => d.label).join(' ') || '—';
        },

        newSchedule() { this.form = _schedDefaultForm(); this.editing = 'new'; this.error = ''; },
        editSchedule(s) { this.form = _schedToForm(s); this.editing = s.uuid; this.error = ''; },
        cancel() { this.editing = null; },

        toggleDay(bit) { this.form.days = this.form.days ^ (1 << bit); },
        hasDay(bit) { return (this.form.days & (1 << bit)) !== 0; },

        async save() {
            const payload = _schedFormToPayload(this.form);
            const isNew = this.editing === 'new';
            const url = isNew ? '/api/schedules' : `/api/schedules/${this.editing}`;
            const resp = await fetch(url, {
                method: isNew ? 'POST' : 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload),
            });
            if (!resp.ok) {
                let detail = '';
                try { detail = await resp.text(); } catch (_) { /* ignore */ }
                this.error = `Save failed (${resp.status})${detail ? ': ' + detail : ''}`;
                return;
            }
            this.editing = null;
            await this.load();
        },

        async toggleEnabled(s) {
            const payload = _schedFormToPayload(_schedToForm(s));
            payload.enabled = !s.enabled;
            await fetch(`/api/schedules/${s.uuid}`, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload),
            });
            await this.load();
        },

        async remove(s) {
            if (!window.confirm(`Delete schedule "${s.name || s.uuid}"?`)) { return; }
            await fetch(`/api/schedules/${s.uuid}`, { method: 'DELETE' });
            await this.load();
        },
    };
}
