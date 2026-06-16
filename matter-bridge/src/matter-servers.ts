/**
 * Custom matter.js cluster servers that implement the mandatory command handlers the
 * stock matter.js servers leave unimplemented.
 *
 * matter.js's `ValidatedElements` logs a warning for every mandatory command whose
 * handler is still the default `Behavior.unimplemented` stub (which throws if a
 * controller ever invokes it). Two such commands land on our bridged endpoints:
 *
 *   - Identify.triggerEffect       - on every endpoint (they all carry Identify)
 *   - Thermostat.setpointRaiseLower - on every heater (Thermostat) endpoint
 *
 * Each previously produced, once per endpoint at startup:
 *   WARN ValidatedElements  Error in IdentifyServer.triggerEffect: Throws unimplemented exception
 *   WARN ValidatedElements  Error in ThermostatServer.setpointRaiseLower: Throws unimplemented exception
 *
 * Supplying real handlers both silences those warnings AND makes the commands work
 * when a Matter controller (Apple Home / Google Home / Alexa / SmartThings) sends them.
 */

import { IdentifyServer } from "@matter/main/behaviors/identify";
import { ThermostatServer } from "@matter/main/behaviors/thermostat";
import { Identify, Thermostat } from "@matter/main/clusters";

/**
 * Identify server that accepts (and ignores) `triggerEffect`.
 *
 * `triggerEffect` asks a device to show a transient visual effect (blink / breathe /
 * okay). Our endpoints are *bridged* pool equipment with no physical indicator to
 * flash, so the spec-compliant behaviour is to accept the command as a no-op (§1.2.6.2
 * leaves the effect entirely up to the implementer). The base `IdentifyServer` already
 * implements the mandatory `identify` command; we only fill in the missing
 * `triggerEffect`.
 */
export class AqualinkIdentifyServer extends IdentifyServer {
  override triggerEffect(_request: Identify.TriggerEffectRequest): void {
    // No physical identifier on a bridged device: accept the command and do nothing.
  }
}

/**
 * Compute the new heating setpoint (in centi-°C) for a relative SetpointRaiseLower.
 *
 * Per the Matter spec (§4.3.10.1) the `Amount` field is a signed value in steps of
 * 0.1 °C that is *added* to the current setpoint, and the result is clamped to the
 * configured limits (clamping is explicitly not an error). Setpoints are stored in
 * centi-°C (0.01 °C), so one Amount step == 10 centi-°C.
 *
 * Pure (no matter.js state) so it can be unit-tested in isolation.
 */
export function raiseLowerHeatingSetpoint(opts: {
  /** Current occupiedHeatingSetpoint, centi-°C. */
  currentCenti: number;
  /** Signed Amount field, in 0.1 °C steps. */
  amountSteps: number;
  /** minHeatSetpointLimit, centi-°C. */
  minCenti: number;
  /** maxHeatSetpointLimit, centi-°C. */
  maxCenti: number;
}): number {
  const next = opts.currentCenti + opts.amountSteps * 10;
  return Math.max(opts.minCenti, Math.min(opts.maxCenti, next));
}

// Matter spec defaults for the heat-setpoint limits, used only as a fallback if the
// attributes are somehow unset (the bridge always configures explicit limits).
const SPEC_DEFAULT_MIN_HEAT_CENTI = 700; // 7.00 °C
const SPEC_DEFAULT_MAX_HEAT_CENTI = 3000; // 30.00 °C

/**
 * Heat-only Thermostat server implementing the mandatory `setpointRaiseLower` command.
 *
 * It moves `occupiedHeatingSetpoint` by the relative amount and clamps to the
 * configured heating limits. Mutating the attribute fires `occupiedHeatingSetpoint$Changed`,
 * which `bridge.ts` already forwards to the aqualink-automate setpoint API - so a
 * relative adjustment from a controller reaches the pool controller exactly like an
 * absolute one. The Heating feature is baked in here so `this.state` exposes the
 * heat-setpoint attributes.
 */
export class AqualinkThermostatServer extends ThermostatServer.with("Heating") {
  override setpointRaiseLower({ mode, amount }: Thermostat.SetpointRaiseLowerRequest): void {
    // Heating-only endpoint: a Cool-only adjustment has no setpoint to move.
    if (mode === Thermostat.SetpointRaiseLowerMode.Cool) {
      return;
    }

    this.state.occupiedHeatingSetpoint = raiseLowerHeatingSetpoint({
      currentCenti: this.state.occupiedHeatingSetpoint ?? 0,
      amountSteps: amount,
      minCenti: this.state.minHeatSetpointLimit ?? SPEC_DEFAULT_MIN_HEAT_CENTI,
      maxCenti: this.state.maxHeatSetpointLimit ?? SPEC_DEFAULT_MAX_HEAT_CENTI,
    });
  }
}
