# 40106 Lab

A virtual CMOS noise box: a simulation of interacting CD40106 Schmitt-trigger RC oscillators, aimed eventually at standalone firmware for the M-VAVE FM-1. See `docs/handoff.md` for the product brief and `GLOSSARY.md` for terms.

This is **M1**: a single inverter in a portable C++ engine, with a desktop WAV renderer and unit tests. There are no links between inverters, no FM-1 firmware and no UI yet.

## Build and run

You need CMake 3.20+ and a C++17 compiler (Apple clang or GCC). On macOS: `brew install cmake`.

One command builds the project and renders 10 seconds of audio:

```sh
./render.sh
```

This writes `out/inverter.wav` (mono, 16-bit, 48 kHz) and `out/inverter.csv`. The CSV has columns `time_s,capacitor_v,output_high` and covers the first 50 ms. Options are passed through:

```sh
./render.sh --r 47000 --c 22e-9 --vlow 0.3 --vhigh 0.7 --rate 44118 --out out/wide
./render.sh --help
```

To run the tests:

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

## Layout

- `engine/`: the portable model. It has no platform, file or UI dependencies, no allocation, and uses `float` throughout, because the FM-1 has a single-precision FPU. `-Wdouble-promotion` is enabled to catch accidental `double` maths.
- `desktop/`: the WAV/CSV renderer.
- `tests/`: unit tests using a small dependency-free harness (`tests/Check.h`).

## The model

Voltages are normalised to the supply: 0 is VSS and 1 is VDD. While the output is fixed, the capacitor follows the exact RC solution:

```text
Vc(t + dt) = Vout + (Vc(t) - Vout) * exp(-dt / (R * C))
```

The output is HIGH while the capacitor charges and LOW while it discharges. It switches LOW when `Vc` reaches the upper threshold and HIGH when `Vc` falls to the lower threshold. The time to reach the threshold is found analytically, so edges land at their exact time within a step. The capacitor is set to the threshold at each edge. Results therefore don't depend on the step size.

The ideal period is `RC * (ln((1 - Vl) / (1 - Vh)) + ln(Vh / Vl))`.

**Defaults.** R = 100 kΩ, C = 10 nF, Vl = 0.39 and Vh = 0.59. That gives about 1233 Hz, about 1.23/RC. The thresholds are the typical CD40106 values from the TI and onsemi datasheets (see the CD40106 research ticket in the wayfinder map). They are not the often-quoted 1/3 and 2/3.

**Power-on.** The capacitor starts discharged. Its input is below the lower threshold, so the output starts HIGH. The first half-cycle charges from 0 rather than from Vl, so it is longer than the rest, as on a real circuit at power-on.

**Validation.** `setParams` rejects invalid values and keeps the previous ones:
- non-finite values
- R ≤ 0 or C ≤ 0
- RC < 1 µs
- thresholds outside (0, 1)
- Vl ≥ Vh

The renderer reports the reason and exits.

**Changing thresholds while running.** If a new threshold has already been passed, the output switches on the next step.

## Limitations

- **Aliasing.** The renderer samples the output state directly, so its sharp edges alias badly: higher rates sound inharmonic and gritty. The plan is 2-point PolyBLEP corrections at each edge's exact time (see the anti-aliasing research ticket). The engine already knows those times, but it doesn't report them yet.
- **DC and level.** The output is a ±level square wave with a duty cycle near 48% at default thresholds. There is no DC blocker, drive, tone or limiter yet.
- **Ideal inverter.**
  - The output swings rail to rail, with no output resistance and no propagation delay.
  - There are no input protection diodes or supply interaction.
  - Thresholds are the same for every instance. Real parts vary widely (VT+ 0.44–0.72 × VDD at 5 V).
- **Extreme rates.** At most 64 edges are processed per step. Beyond that, the rest of the step runs without switching. This is only reached far above audio rates (RC near the 1 µs minimum).
- **Precision.** Single-precision floats. Over long runs at very slow rates, small rounding errors can shift edge timing slightly. Tests check a 60-second run stays finite and bounded.

## Assumptions and open questions

- **Engine parameters.** R, C and normalised thresholds are the parameters for now. Whether the engine should expose these physical values or performance controls (rate, range, asymmetry) is open in the wayfinder map ("What parameters does the engine expose per inverter?").
- **Sample rate.** 48 kHz is the desktop default. The FM-1 runs at 44,118 Hz, so the renderer takes `--rate` and the engine is rate-agnostic.
- **Output format.** 16-bit WAV is enough for inspection. The FM-1's audio path is 24-bit.
- **Not yet compared.** Nothing here has been compared with the breadboard reference circuit yet. That waits on the recordings ticket.
