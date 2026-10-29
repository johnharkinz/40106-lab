# How do real CD40106 oscillators and hard-sync circuits behave?

Research for wayfinder ticket #6 (parent map #2, blocks #9). This is a throwaway
research branch and will not be merged.

Source trust levels: **[DS]** means a manufacturer datasheet (primary).
**[DIY]** means a well-known DIY synth reference (secondary; the circuits are
widely built, but the claims are not formally specified). **[Derived]** means
arithmetic on datasheet numbers done in this document.

## 1. Threshold voltages

### CD40106B / MC14106B (identical limits)

TI CD40106B (SCHS097F, rev. Mar 2017) and onsemi MC14106B publish the same
min/max limits. onsemi markets the MC14106B as a "pin-for-pin replacement for
CD40106B". **[DS]**

| VDD | VT+ min / typ / max | VT- min / typ / max | VH min / typ (TI) / typ (onsemi) / max |
|---|---|---|---|
| 5 V  | 2.2 / 2.9 / 3.6 V  | 0.9 / 1.9 / 2.8 V | 0.3 / 0.9 / 1.1 / 1.6 V |
| 10 V | 4.6 / 5.9 / 7.1 V  | 2.5 / 3.9 / 5.2 V | 1.2 / 2.3 / 1.7 / 3.4 V |
| 15 V | 6.8 / 8.8 / 10.8 V | 4.0 / 5.8 / 7.4 V | 1.6 / 3.5 / 2.1 / 5.0 V |

As ratios of VDD **[Derived]**:

- **Typical:** VT+ ≈ 0.58–0.59 × VDD and VT- ≈ 0.38–0.39 × VDD at all three
  supplies. The thresholds are close to ratiometric and sit slightly above
  mid-rail. They are not the "2/3 and 1/3" that MFOS uses as a teaching
  simplification.
- **Guaranteed window:** VT+ is 0.44–0.72 × VDD and VT- is 0.18–0.56 × VDD at
  5 V. At 15 V the window narrows a little: VT+ is 0.45–0.72 × VDD and VT- is
  0.27–0.49 × VDD. Hysteresis is guaranteed only down to 0.06 × VDD at 5 V and
  0.11 × VDD at 15 V.
- **The typical columns disagree with each other.** TI's typical VT+ minus VT-
  gives 1.0, 2.0 and 3.0 V, which does not match its own typical VH of 0.9, 2.3
  and 3.5 V. onsemi's typical VH is 1.1, 1.7 and 2.1 V. Treat "typical" as
  roughly 18–23 % of VDD, not as a precise figure. TI's Figure 11 plots percent
  hysteresis against supply, and the curve rises with VDD.

**Drift.** Neither datasheet gives a typical threshold-versus-temperature curve.
The min/max limits are the same from -55 °C to +125 °C, and onsemi notes that
"Typ" is not a design value. The large spread is part to part, not drift over
temperature. Two consequences:

- Two oscillators with the same R and C can differ a lot in pitch.
- The guaranteed range alone justifies a per-oscillator "threshold low/high"
  or "asymmetry" parameter.

### 74HC14 (TI SN74HC14), for comparison **[DS]**

| VCC | VT+ min / typ / max | VT- min / typ / max | ΔVT min / typ / max |
|---|---|---|---|
| 2 V   | 0.7 / 1.2 / 1.5 V  | 0.3 / 0.6 / 1.0 V  | 0.2 / 0.6 / 1.2 V |
| 4.5 V | 1.55 / 2.5 / 3.13 V | 0.9 / 1.6 / 2.45 V | 0.4 / 0.9 / 2.1 V |
| 6 V   | 2.1 / 3.3 / 4.2 V  | 1.2 / 2.0 / 3.2 V  | 0.5 / 1.3 / 2.5 V |

At 4.5 V the typical ratios are VT+ ≈ 0.56 × VCC and VT- ≈ 0.36 × VCC, close
to the CD40106. Hysteresis is about 20 % of supply.

## 2. Oscillator period

The TI astable figure (Fig. 20) and onsemi Fig. 5 both give: **[DS]**

```
t_high (cap charging, output high) = R·C·ln((VDD − VT-) / (VDD − VT+))
t_low  (cap discharging, output low) = R·C·ln(VT+ / VT-)
f = 1 / (t_high + t_low)
```

TI states this is valid for 50 kΩ ≤ R ≤ 1 MΩ and 100 pF ≤ C ≤ 1 µF. onsemi
requires t1 + t2 to be much larger than tPHL + tPLH.

Worked values **[Derived]**:

| Case | ln sum | f | Duty (high) |
|---|---|---|---|
| Typical, 5 / 10 / 15 V | 0.81 | ≈ 1.23 / RC | 48–49 % |
| Typical 74HC14 at 4.5 V | 0.82 | ≈ 1.22 / RC | 45 % |
| Wide-hysteresis corner (5 V, VT+ 3.6, VT- 0.9) | 2.46 | ≈ 0.41 / RC | 44 % |
| Narrow-hysteresis corner (5 V, VT+ 2.5, VT- 2.2) | 0.24 | ≈ 4.1 / RC | 47 % |

At typical thresholds the frequency barely depends on supply, because the
thresholds scale with it. Pitch changes with supply only through the
non-ratiometric parts: hysteresis percentage, output resistance and diode drops.
The duty cycle is close to 50 %, with a small bias toward a longer low phase,
because the thresholds sit above mid-rail.

The capacitor waveform is a "triangle made of exponential curves" between VT-
and VT+. The output is a rail-to-rail square wave. (Hackaday Logic Noise.) **[DIY]**

## 3. Hard-sync wiring and what each method does to the synced capacitor

### a) Diode injection into the slave's input / timing-cap node (the common one)

The master output connects to the slave's input pin (which is also the top of
its timing cap) through a diode, usually with a series resistor.

- **Diode pointing into the slave (anode at master), as in Hackaday Logic
  Noise.** **[DIY]** While the master is high, the diode conducts and holds the
  slave input high, which "effectively turn[s] it off". While the master is low,
  the diode blocks and the slave free-runs. Logic Noise uses about 1 kΩ in
  series. Look Mum No Computer forum users recommend 10–100 kΩ to limit
  current into the 40106 input.
- **Diode pointing out of the slave (cathode at master), as in the MFOS Weird
  Sound Generator "Wackiness" switch (D1).** **[DIY]** "the input of U1-A is
  brought low during the low part of U1-B's oscillations (via D1)", and D1 is
  reverse-biased (so ignored) during the master's high phase. With the master
  (Wacky) set below the slave (Weird) frequency, MFOS describes a "hollow
  sounding sync effect".

What happens to the slave capacitor **[Derived from the circuits above]**:

1. **Clamp (hold).** For the whole active half-cycle of the master, the cap is
   forced to about VDD − Vf (pull-high case) or about Vf (pull-low case).
   - Vf is about 0.6 V for a 1N4148 and about 0.3 V for a 1N5817 Schottky.
   - The clamp is not a reset to the threshold. It overshoots past the
     threshold toward the rail.
   - The slave's own feedback resistor is normally 10 kΩ or more, much larger
     than the master's output resistance plus the series R, so the clamp wins.
2. **Clamp speed is finite.** The cap is pulled through (master output
   resistance + series R + diode), giving τ ≈ (R_out + R_s)·C_slave.
   - Example: about 250 Ω + 1 kΩ with 0.1 µF gives τ ≈ 125 µs. That is a
     sizeable part of a kHz-range cycle, so the reset sounds "soft".
   - With 10 nF it drops to about 12 µs. With 100 kΩ in series it becomes a
     slow pull, closer to soft sync or frequency modulation than to hard sync.
3. **First cycle after release is longer.** On release the cap starts from near
   the rail, not from the threshold.
   - Example (pull-high, 9 V, typical thresholds 5.3 / 3.5 V, 1N4148): the
     first discharge takes RC·ln(8.4 / 3.5) ≈ 0.87·RC, against 0.41·RC for a
     normal low phase. The first half-cycle is about twice as long.
   - This is part of the timbre and should be modelled. It is not a perfect
     phase reset.
4. **Gated silence.** The slave is frozen for the master's whole active half
   period. If the slave cannot finish a cycle within the master's free half
   period, you hear nothing. Logic Noise: when the frequencies get close, "you
   don't get any sound anymore". The Look Mum No Computer thread reports the
   slave only "turned on" when its rate exceeded the master's. The master sets
   the perceived pitch and the slave rate sets the timbre ("sometimes nasal,
   sometimes biting").
5. **Diode orientation selects the frozen state.** A simulation reported in the
   Look Mum No Computer thread: with no diode, the slave just copies the
   master, inverted. With a diode one way, the slave "never turns on". The
   other way, it "never turns off". Either orientation syncs when the master
   is faster.

### b) Capacitor coupling (edge injection)

A buffered master output is coupled to the slave input through a small
capacitor. This is the Bleep Labs "Glitch Delay" style: buffer, then cap, then
the second oscillator's input. **[DIY]** Bleep Labs describes the result as
"the first oscillator turn[ing] the second on and off". An optional cap makes
the first oscillator lower the second's pitch while high. It does not fully
explain the mechanism.

Physically **[Derived]**, each master edge adds a charge step of about
ΔV ≈ VDD · Cc / (Cc + C_t) to the slave cap. A step toward the threshold that
is about to be crossed triggers an early transition, which acts like
hard/soft sync depending on ΔV. A step the other way delays the transition.

Steps that push the node beyond VDD + 0.5 V or below VSS − 0.5 V are clamped
by the input protection diodes. The TI absolute maximum is ±10 mA DC input
current, so the excess charge is dumped and the cap state is changed
non-linearly. No primary source gives a quantitative 40106-specific treatment.

### c) Pulling / discharging the timing cap with a switch

A transistor or MOSFET dumps the timing cap on each master edge (wiresounds
hard-sync page). **[DIY]** That design discharges through 330 Ω to "4 V"
(its lower threshold) and uses a limiter because a fixed-width pulse "could
easily discharge more than is required", causing timing inconsistency.

This is the closest to an ideal "reset to threshold" and is edge-triggered,
not level-held. It is not typical of minimalist 40106 noise boxes. An All
About Circuits thread reports that a bare diode sync that worked in simulation
failed on real hardware (and one direct-coupling attempt made the 40106
oscillate at about 2 MHz), while an NPN buffer in front of the input worked.
The thread was not directly retrievable (HTTP 403); this is from search
snippets only.

## 4. Non-ideal behaviours and whether they are audible

| Behaviour | Numbers | Audible? |
|---|---|---|
| **Threshold spread between parts** | VT+ 0.44–0.72 × VDD, VT- 0.18–0.56 × VDD; f from ≈ 0.4/RC to ≈ 4/RC at the guaranteed corners **[DS/Derived]** | **Yes, large.** Sets pitch, duty and sync-timbre differences between "identical" oscillators. Model per-oscillator thresholds. |
| **Output resistance** (CD40106) | 5 V: (5 − 4.6 V)/1 mA typ ≈ 400 Ω (≈ 780 Ω at the 0.51 mA min). 10 V: 0.5 V/2.6 mA ≈ 190 Ω. 15 V: 1.5 V/6.8 mA ≈ 220 Ω. 74HC14 at 4.5 V: ≈ 50 Ω **[DS/Derived]** | Adds to the timing R. Negligible inside TI's 50 kΩ+ range, noticeable at low R and high pitch. **Matters most in the sync path,** where it sets the clamp time constant. |
| **Output swing** | VOH ≥ VDD − 0.05 V and VOL ≤ 0.05 V at no load **[DS]**. Under mA loads it sags (see output resistance) | Mostly no. Unloaded outputs are effectively rail-to-rail. Divide down for line level (about ÷9 from 9 V, Logic Noise). |
| **Input protection diodes** | Inputs limited to VSS − 0.5 V … VDD + 0.5 V, ±10 mA abs max; "not overvoltage tolerant… input clamp diodes to VCC" **[DS]** | Yes, for capacitor-coupled or external-signal sync: they clip injected steps and dump charge. Look Mum No Computer users add series 10 kΩ or more and suggest an external Zener or Schottky clamp. |
| **Shared supply / ground on one chip ("soft sync")** | Not specified. The electro-music forum attributes pitch pulling between oscillators to the inverters sharing internal supply rails; decoupling (0.1–1 µF at pin 14) "reduce[s] it and [doesn't] eliminate it" **[DIY]** | **Yes.** It is part of the character of real 40106 boxes: oscillators on one chip pull and beat. Optional weak cross-coupling term. |
| **Supply sag / battery** | Typical thresholds are ratiometric, so f is roughly constant with VDD, but VH % and R_out are not **[DS/Derived]** | Small pitch drift and mild cross-modulation, mainly through shared-supply sag when several outputs switch. |
| **Propagation delay / transition time** | tPLH/tPHL 140 ns typ at 5 V, 70 ns at 10 V, 60 ns at 15 V. Transition times 100 / 50 / 40 ns. CIN 5 pF typ **[DS]** | No, below about 100 kHz. Irrelevant for audio-rate C and R. |
| **Input leakage** | 10 pA typ, ±0.1 µA max at 25 °C (±1 µA at 85 °C) **[DS]** | No, except with MΩ-plus resistors or LFO-scale timing. |
| **Diode Vf in sync** | 0.6 V (silicon) vs 0.3 V (Schottky) | Yes. Sets the clamp level and therefore the first-cycle stretch after release (section 3a). |

## 5. Implications for the simulator (for #9)

- Parameterise each oscillator by VT+/VDD and VT-/VDD. Default to about 0.59
  and 0.39. Allow ranges of about 0.44–0.72 and 0.18–0.56 for "chip
  character".
- Model diode sync as a level-sensitive clamp: a conductance of 1/(R_out + R_s)
  toward (V_master − Vf), active only while the diode is forward biased. Do not
  model it as an instantaneous phase reset. Clamp level, clamp speed, the
  stretched first cycle and gated silence all come out of this naturally.
- Diode orientation should be a link parameter: pull-high (Logic Noise) or
  pull-low (MFOS WSG).
- Clamp the cap node to [VSS − 0.5, VDD + 0.5] to mimic the protection diodes.
  This matters for any capacitive coupling.
- Optional: add a small shared-supply coupling term between oscillators to get
  real-chip "soft sync".

## Gaps

- No primary source gives typical threshold drift against temperature or a
  per-manufacturer curve beyond the 25 °C typical values. Nexperia (HEF40106B,
  74HC14) datasheets returned 403 and were not checked.
- The user's actual breadboard sync wiring (orientation, series R, diode type)
  is unknown. Record it, or measure Vcap with a scope, to pick the defaults.
- Capacitor-coupled sync has no quantitative primary source. Section 3b is
  derived from first principles.

## Sources

- TI, *CD40106B CMOS Hex Schmitt-Trigger Inverters*, SCHS097F, rev. March
  2017: https://www.ti.com/lit/ds/symlink/cd40106b.pdf
- onsemi, *MC14106B Hex Schmitt Trigger* datasheet:
  https://www.onsemi.com/download/data-sheet/pdf/mc14106b-d.pdf
- TI, *SN74HC14 Hex Schmitt-Trigger Inverters*:
  https://www.ti.com/lit/ds/symlink/sn74hc14.pdf
- Elliot Williams, *Logic Noise: Sweet, Sweet Oscillator Sounds*, Hackaday,
  2015: https://hackaday.com/2015/02/04/logic-noise-sweet-sweet-oscillator-sounds
- Ray Wilson, *Weird Sound Generator* schematics and theory, Music From Outer
  Space: https://musicfromouterspace.com/analogsynth_new/WSG2010/wsg_page2.html
- Look Mum No Computer forum, *Sync-ing multiple 40106 LFOs*:
  https://lookmumnocomputer.discourse.group/t/sync-ing-multiple-40106-lfos/7063
- electro-music.com forum thread on 40106 cross-oscillator "soft synch":
  https://electro-music.org/forum/post-295700.html
- Bleep Labs, light-controlled oscillator coupling: https://bleeplabs.com/?p=6048
- wiresounds, hard sync (MOSFET discharge with limiter):
  https://wiresounds.leprd.space/hardsync.html
- All About Circuits, *Sync/reset pulse on a CD40106 astable* (search snippets
  only):
  https://forum.allaboutcircuits.com/threads/sync-reset-pulse-on-a-cd40106-astable.159522/
