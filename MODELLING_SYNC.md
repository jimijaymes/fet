# Shared modelling update policy

Saturator is the reference implementation for the reusable transformer, magnetic-memory and circuit saturation algorithms.

When modifying a shared algorithm in Saturator, review and intentionally port the applicable changes to:
- FET (1176-inspired Rev A/F/G input/output stages)
- OptoFET (FET and optical saturation stages)
- OptoValve (optical/tube and transformer stages)
- Guitar Polisher (transformer and frequency-dependent saturation stages)
- Ultimate (all matching preamp, compressor, and output stages)

Do not blindly replace revision-specific calibration. Preserve hardware topology and signal ordering, including FET's post-output-stage makeup and final output gain.

For each port, compare parameters and state variables, build the affected repositories, and perform level-matched listening plus distortion/frequency-response checks. Track which plugins have actually been updated; separate GitHub repositories do not synchronize automatically.

## FET integration status (2026-11-22)
- Ported Saturator's transformer leaky flux memory, frequency-dependent low/high response, nonlinear magnetic branch, and amplifier blending to FET's input/output transformer processing.
- Rev A: coloured early input and Class A-style output, Saturator Class A output transformer voicing.
- Rev F: transformer-coupled input, later Class AB amplifier, Saturator Rev F output transformer voicing.
- Rev G: transformerless differential input, later Class AB amplifier and Rev F-style output transformer.
- FET-specific revision voicings remain estimates, not measured circuit-identical emulations.
- No automatic synchronization or hardware calibration has been implemented.
