# 40106 Lab — Development Handoff

**Status:** Concept / implementation planning  
**Target:** M-VAVE FM-1 synthesiser  
**Version:** v0.1 MVP  
**Date:** 29 October 2026

## 1. Executive summary

Build **40106 Lab**, a standalone, experimental firmware instrument for the M-VAVE FM-1. It models the behaviour of a **CD40106 hex Schmitt-trigger inverter** and lets users create unstable drones, hard-sync sounds, logic rhythms and glitches by connecting virtual oscillator circuits.

The defining idea is **a virtual CMOS noise box, not a conventional synthesiser with 40106-inspired presets**. Model capacitor state, RC charging/discharging and Schmitt thresholds so that interacting oscillators behave like a small circuit.

**Recommended first step:** implement a portable desktop C++ simulation of the user's existing physical CD40106 hard-sync breadboard circuit. Generate WAV files and plots; compare behaviour with recordings of the physical circuit before writing device firmware.

## 2. Product principles

1. **Circuit behaviour first:** interactions should arise from virtual electrical states wherever practical, rather than arbitrary synth modulation alone.
2. **Playful experimentation:** encourage strange and unstable sounds; preserve a reliable software/audio pipeline.
3. **Visible signal flow:** show the inverter relationships clearly on the FM-1 display.
4. **Hands-on control:** immediate editing with physical encoders and buttons, minimal menu depth.
5. **Portable engine:** DSP core must not depend on the FM-1 SDK, display, storage or input drivers.
6. **Progressive scope:** prove the core sound with three virtual inverters before expanding to all six.

## 3. MVP scope — v0.1

### In scope

- Three independently configurable virtual Schmitt-trigger RC oscillators.
- Per-oscillator capacitor voltage and output state; upper/lower thresholds, frequency/rate, output level.
- Two simultaneous patch connections initially; prioritise **hard sync** and **resistive coupling**.
- Mixed audio output, basic drive/tone treatment and output protection.
- FM-1 patch overview screen showing oscillators and their connections.
- Hardware control for selecting oscillators and editing primary parameters.
- A known working default patch and several demonstrative factory patches.
- Desktop reference engine with WAV rendering, waveform plotting and unit tests.
- FM-1 firmware integration after feasibility and performance checks.

### Deferred

- All six oscillators active simultaneously (design engine capacity for six).
- Arbitrary patch graph editor and many connection slots.
- AND/XOR/OR logic mixing, advanced feedback networks.
- Full sequencer, MIDI composition functionality or conventional polyphonic keyboard instrument.
- Sophisticated effects, full patch librarian, patch mutation and multiple performance modes.
- Browser-based patch editor or desktop plugin.

## 4. Example sounds / acceptance demos

1. **Hard Sync:** audio oscillator interrupted/reset by slower or faster oscillator; sweeping parameters produces recognisable sync textures.
2. **Unstable Drone:** three interacting audio-rate oscillators produce beating, pulsing and timbral changes.
3. **Logic Rhythm (post-MVP):** slow oscillator outputs combined via digital logic to produce irregular rhythms.

The first two should be demonstrable in v0.1; the third is a future direction, not a release blocker.

## 5. Interface and controls

### Visual design

A compact electronic test instrument aesthetic: dark background, crisp signal paths, restrained colour coding for inverter identities, animated state indications where performance permits. Avoid ornamental or deep menu-driven UI.

### Screens

- **Patch view (default):** three inverter nodes, labelled links (SYNC / COUPLE), mixer/output, selected inverter and primary parameters.
- **Oscillator editor:** rate/range, threshold low/high or asymmetry, output level, capacitor/state visualization.
- **Scope/monitor (later or lightweight MVP):** capacitor voltage and digital output; diagnostic rather than precision laboratory measurement.
- **Connection editor:** select source, target, connection type, amount; validate invalid links.

### Proposed control mapping — verify actual physical labels and input APIs

| Control role | Proposed action |
| --- | --- |
| Main selection encoder | Select inverter 1–3 |
| Patch/preset encoder | Select saved patch |
| Connection/algorithm encoder | Select link to edit |
| Parameter encoder 1 | Rate |
| Parameter encoder 2 | Range / scaling |
| Parameter encoder 3 | Threshold asymmetry |
| Parameter encoder 4 | Output level |
| Edit button | Oscillator details |
| FX button | Mixer/output view |
| SEQ button | Connection editor |
| Home button | Patch overview |
| Save button | Save patch |
| Play/Stop | Run/mute circuit |

**Important:** this is a design mapping, not a verified FM-1 control contract. Inspect current device drivers and hardware definitions before implementation. Avoid assuming a specific encoder count, screen resolution or button names until confirmed.

### UX rules

- Patch connections remain visible while adjusting core parameters.
- Common actions require at most one or two control interactions from the main view.
- Parameter changes have immediate audible and visible feedback.
- Provide safe reset/default-patch actions; avoid accidental destructive changes.

## 6. DSP model

### 6.1 Basic RC oscillator

A Schmitt-trigger inverter drives its own input through feedback resistance R. The input capacitance C charges/discharges, and the inverter changes output state at two thresholds.

For an isolated RC node during a fixed-output interval:

```text
Vc(t + dt) = Vout + (Vc(t) - Vout) * exp(-dt / (R * C))
```

- Output HIGH while the capacitor charges, until `Vc >= Vhigh`, then switch LOW.
- Output LOW while the capacitor discharges, until `Vc <= Vlow`, then switch HIGH.
- Require `Vlow < Vhigh`, with sensible bounds and defaults.
- Use normalized voltages (e.g., 0 to 1) initially rather than pretending to calibrate one particular CD40106 manufacturer or supply voltage.

### 6.2 Resistive coupling

For multiple drivers connected through virtual resistances:

```text
C * dVc/dt = Σ((Vsource_j - Vc) / Rj)
```

When source outputs are fixed over a time interval, calculate the equivalent conductance and target voltage and integrate exponentially. Resistive coupling is preferable to purely arbitrary frequency modulation because it reflects the circuit model and naturally creates interactions.

### 6.3 Hard sync

Provide an explicit sync connection for v0.1: an edge from source can force a target capacitor to a defined reset level or trigger a target state transition. Document that this is **a useful modelling abstraction**, not a universal wiring-equivalent representation of a specific real-world hard-sync circuit. Later, model diode/capacitive injection or specific circuit topologies if recordings justify the extra complexity.

### 6.4 Scheduling and coupled updates

- Capture inverter output states at the beginning of each integration substep.
- Evaluate all coupling and trigger effects from a consistent snapshot.
- Advance all inverter states without source-order dependence.
- Detect threshold crossings and handle substep/event timing accurately enough for audio-rate behaviour.
- Avoid unbounded event cascades from feedback loops.

### 6.5 Audio output quality

Schmitt outputs have sharp edges and can alias badly when sampled directly.

- Benchmark at target sample rate; investigate 2x/4x oversampling and anti-alias filtering.
- Consider transition-aware rendering / band-limited steps if necessary.
- Separate **physical-state simulation** from **audio reconstruction/mixing**.
- Apply DC blocking, bounded gain, optional drive and a conservative output limiter/soft clipper.
- Parameter-smooth changes that otherwise create unwanted digital clicks while preserving intentional glitch behaviours.

### 6.6 Early prototype interfaces (illustrative)

```cpp
struct Inverter {
    float capacitorVoltage;
    float resistance;
    float capacitance;
    float thresholdLow;
    float thresholdHigh;
    bool outputHigh;
};

enum class ConnectionType {
    Resistive,
    HardSync,
    ThresholdMod // later
};

struct Connection {
    uint8_t source;
    uint8_t destination;
    ConnectionType type;
    float amount;
};

class Circuit {
public:
    void setPatch(/* validated patch data */);
    void process(float* output, size_t frameCount);
private:
    Inverter inverters_[6];
    Connection connections_[12];
};
```

These interfaces are **sketches**, not drop-in implementation. Use suitable precision/structure, no allocations in audio processing, and fixed-capacity collections.

## 7. Software architecture

```text
PRESENTATION / INTERACTION
  Patch view | Oscillator editor | Scope/monitor
                  ↓
APPLICATION / CONTROL
  Input mapping | Parameter handling | Patch manager
                  ↓
AUDIO / CIRCUIT ENGINE (portable C++)
  RC/Schmitt model | Connection solver | Mixer/output DSP
                  ↓
FM-1 HARDWARE ABSTRACTION
  Audio callback | Display | Controls | Storage | MIDI if needed
                  ↓
PHYSICAL HARDWARE
  Processor | Screen | Encoders/buttons | Audio I/O
```

Maintain a strict dependency boundary: `engine/` knows nothing about firmware UI or hardware specifics. Keep the display/control loop separate from real-time audio processing.

### Proposed repository

```text
40106-lab/
├── engine/
│   ├── SchmittInverter.h/.cpp
│   ├── Circuit.h/.cpp
│   ├── Connection.h
│   ├── Mixer.h/.cpp
│   └── OutputStage.h/.cpp
├── desktop/
│   └── render_wav.cpp
├── firmware/
│   ├── main.cpp
│   ├── audio/
│   ├── controls/
│   ├── display/
│   ├── patches/
│   └── platform/
├── tests/
│   ├── oscillator_tests.cpp
│   ├── connection_tests.cpp
│   ├── sync_tests.cpp
│   └── performance_tests.cpp
├── docs/
│   ├── product-spec.md
│   ├── dsp-model.md
│   ├── ui-design.md
│   └── architecture.md
└── CMakeLists.txt
```

Adapt actual firmware structure to the SDK/BSP build conventions.

### Real-time constraints

- Never allocate, log, render UI, read files or block on locks inside the audio callback.
- Use fixed-size structures and bounded processing time.
- Pass UI changes through atomic parameters, bounded lock-free queues or safely swapped prevalidated configurations.
- Apply patch changes at a defined buffer boundary.
- Profile CPU load, timing and memory use on target hardware; detect underruns.

## 8. Platform integration research

Current community references identified in discussion:

- Felucca firmware: https://github.com/hugelton/Felucca
- FM-1 community guide: https://fuleo.github.io/fm1-guide/

**Verify before depending on them:** exact availability/version/API of `felucca-bsp`, FM-1 processor and RAM constraints, audio sample rate/buffer API, display resolution, inputs, persistence mechanism, simulator support, flashing and recovery workflow, and compatible licences. Prior discussion suggested the BSP may support standalone firmware; treat that as a lead to validate against current repository source and documentation, not a locked architecture assumption.

Two viable routes:

- **Preferred if supported:** standalone firmware using `felucca-bsp` and selectively reused, licence-compatible support components.
- **Fallback:** small fork of existing working FM-1 firmware with our engine and custom UI, keeping upstream changes isolated.

Do not start by rewriting drivers if they are available and usable.

## 9. Test strategy

### Desktop tests

- Isolated oscillator starts reliably and switches at specified thresholds.
- Period/frequency follows R and C (within calculated tolerance).
- Threshold ordering and parameter boundaries are validated.
- Coupled oscillators are independent of update iteration order.
- Hard-sync edge and reset semantics are deterministic.
- Zero/invalid resistance, extreme rates and invalid patch graphs fail safely.
- Rendered WAV has no unexpected DC, NaNs, runaway gain or buffer overruns.
- Audio rendering performance measured for 1, 3 and 6 inverter configurations.

### Physical comparison

The user has an actual breadboard CD40106 circuit, including a working audio oscillator and separate hard-sync oscillator. Use phone/audio-interface recordings and circuit values where available to compare:

- Rate range and sweep shape.
- Sync-induced pitch/timbre changes.
- Asymmetry, silence zones and behaviours near thresholds.
- Interactions between oscillator rates.

Aim for a characterful behavioural match, not component-level fidelity.

### On-device tests

- Stable audio during UI redraws and encoder sweeps.
- No underruns or unacceptable UI latency.
- Patch load/save and reboot persistence (if included).
- Output remains within safe limits on headphones/line output.
- Flashing and recovery steps proven on the actual device.

## 10. Milestones and deliverables

### M1 — Desktop model

**Deliver:** single-inverter C++ model, WAV renderer, waveform/capacitor voltage traces, unit tests.  
**Exit:** oscillator starts, meets threshold/frequency tests and produces usable audio.

### M2 — Interactions

**Deliver:** three oscillators, hard sync, resistive coupling, two links, mixer, safe output.  
**Exit:** two demonstrable sounds (hard sync and unstable drone), deterministic tests, no significant DSP artefacts without known mitigation.

### M3 — Hardware proof

**Deliver:** minimal bootable FM-1 firmware running the portable engine.  
**Exit:** stable audio, measured CPU load and verified flashing/recovery.

### M4 — UI and physical controls

**Deliver:** patch overview and editor, parameter changes, connection editing.  
**Exit:** main sound parameters controllable directly; no audio glitches when navigating.

### M5 — First release

**Deliver:** validated presets, patch save/load if practical, release binary, source, setup/flash/recovery documentation.  
**Exit:** someone unfamiliar with the code can install, hear, edit and recover safely.

## 11. Decisions / risks to resolve

1. **FM-1 firmware base:** standalone BSP vs Felucca fork, after inspecting real source and licence requirements.
2. **Exact control inventory:** confirm encoders, keys, labels and display constraints before finalising navigation.
3. **Audio performance:** establish CPU and memory budgets before committing to oversampling or rich scope rendering.
4. **Electrical fidelity:** choose a representative reference circuit and record its component values/sounds; distinguish physical approximations from creative behaviours.
5. **Connection semantics:** specify sync edge polarity, reset target and resistive-coupling strength units.
6. **Patch persistence:** determine storage options and versioned serialization.
7. **Safety/recovery:** avoid risky firmware flashing instructions until a verified recovery route is documented.
8. **Open-source licence:** review licences of all incorporated upstream code before publication.

## 12. Suggested first development task for coding agent

**Task:** Create M1: portable CD40106 behavioural oscillator prototype.

1. Set up minimal CMake C++ project for `engine/`, `desktop/`, and `tests/`.
2. Implement normalised 0–1 capacitor voltage, Schmitt output state and exact exponential RC updates.
3. Define correct initialisation and reset behaviour; make threshold transitions deterministic.
4. Render 10 seconds of mono 48-kHz WAV audio and a CSV of time/capacitor/output values for inspection.
5. Include CLI arguments or a small fixed preset for R, C and threshold levels.
6. Write tests for threshold crossings, frequency, stability and parameter validation.
7. Explain model limitations and identify anticipated aliasing issues.
8. **Do not** integrate FM-1 firmware, add a complex UI, or implement all six oscillators in this task.

**Definition of done:** build instructions work on macOS/Linux; a user can run one command to generate and inspect a valid WAV file; automated tests pass. Record assumptions and outstanding questions in the README.

---

**Long-term direction:** expand from three to six virtual inverters, then add logic combiners, graph-based patching, mutation, additional CMOS components and possibly a web-based educational circuit playground. These are opportunities, not MVP requirements.
