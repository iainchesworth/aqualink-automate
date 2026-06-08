/**
 * UI Constants — single source of truth for cross-component UI configuration.
 *
 * Loaded as a classic (non-module) script so the values are exposed on the
 * global `window.AquaUI` namespace, matching the existing component/view/store
 * scripts (all plain global functions invoked by Alpine's `x-data`).
 *
 * Load order: this file MUST be referenced from index.html BEFORE the
 * component and view scripts (e.g. immediately after the store scripts and
 * before the `<!-- Component scripts -->` block). Consumers access the
 * namespace lazily (inside getters/methods that Alpine calls after all scripts
 * have loaded), and each consumer keeps an inline fallback so the UI still
 * renders correctly even if this script is absent.
 *
 * Consumed by:
 *   - chemistry-gauge.js / settings-view.js  (chemistry band defaults + key)
 *   - equipment-button.js / pool-graphic.js  (isActiveStatus + device keywords)
 */
(function (global) {
    'use strict';

    // ---- Chemistry bands -----------------------------------------------------
    // localStorage key holding per-gauge band overrides.
    const CHEMISTRY_BANDS_KEY = 'chemistryBands';

    // Three-tier band defaults (Good / Okay / Bad) for each chemistry gauge.
    // Shared between the gauge component (rendering) and the settings view
    // (editing + reset-to-defaults). Display-only fields (min/max/unit/etc.)
    // live in the gauge component; only the editable band fields are here.
    const CHEMISTRY_BAND_DEFAULTS = {
        ph: {
            goodMin: 7.4, goodMax: 7.6,
            okayMin: 7.2, okayMax: 7.8,
            badMin:  7.0, badMax:  8.0
        },
        orp: {
            goodMin: 700, goodMax: 750,
            okayMin: 650, okayMax: 700,
            badMin:  650, badMax:  800
        },
        salt: {
            goodMin: 3500, goodMax: 4000,
            okayMin: 2700, okayMax: 3500,
            badMin:  2700, badMax:  4500
        }
    };

    // ---- Active-status predicate --------------------------------------------
    // Status strings the backend reports (magic_enum names) that mean the
    // device is currently "on/active": On, Running, Heating, Enabled.
    const ACTIVE_STATUS_VALUES = ['on', 'running', 'heating', 'enabled'];

    function isActiveStatus(status) {
        const s = String(status == null ? '' : status).toLowerCase();
        return ACTIVE_STATUS_VALUES.includes(s);
    }

    // ---- Device-classification keyword lists --------------------------------
    // Label substrings (lower-case) used to classify a button/device when no
    // explicit device_type trait is available. Order within a list does not
    // matter; the first matching CLASSIFIER (in icon resolution) wins.
    const DEVICE_KEYWORDS = {
        chlorinator: ['chlor', 'aquapure', 'salt'],
        spa:         ['spa'],
        pump:        ['pump', 'filter'],
        heater:      ['heat']
    };

    function labelMatchesKeywords(label, keywords) {
        const l = String(label == null ? '' : label).toLowerCase();
        return keywords.some((kw) => l.includes(kw));
    }

    global.AquaUI = {
        CHEMISTRY_BANDS_KEY,
        CHEMISTRY_BAND_DEFAULTS,
        ACTIVE_STATUS_VALUES,
        isActiveStatus,
        DEVICE_KEYWORDS,
        labelMatchesKeywords
    };
})(typeof window !== 'undefined' ? window : globalThis);
