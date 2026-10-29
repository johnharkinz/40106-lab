# Antialiasing hard-edged, event-driven oscillators

Research for [#7](https://github.com/johnharkinz/40106-lab/issues/7): how to render square/pulse outputs without aliasing when an event-based state simulation already gives each transition's time to within a fraction of a sample. Compared: oversampling plus decimation, PolyBLEP, and minBLEP/BLEP tables. Looked at: cost per voice and per edge, behaviour under hard sync (**Sync links**), and fit for a small embedded CPU.

Status: throwaway research branch, not to be merged. Claims carry a source tag ([S&S], [Brandt], [VPN]; see the end). Lines marked **(inference)** are my own reasoning or estimates, not something a source states.

## TL;DR

- **Recommendation: a 2-point PolyBLEP residual accumulator, run at the output sample rate.** Each **Inverter** output edge adds a 2-sample polynomial correction at its exact sub-sample time. The cost is about 4 multiplies and 4 adds per edge, with no tables and a 1-sample delay [VPN §III, Table I, Table IX].
- **Oversampling the naive edges is the weakest option per CPU cycle.** A naive step's spectrum falls only 6 dB/octave [S&S §2], so each doubling of the rate buys only about 6 dB less aliasing **(inference)**. [VPN] notes that "a very high oversampling ratio is required."
- **Hard sync is not a special case for BLEP-family methods.** A sync-forced flip is just another step at a known time and height. Brandt's minBLEP paper was written for exactly this problem [Brandt]. Corrections are linear, so edges that land close together just add.
- **minBLEP** gives better stopband control and zero lookahead [Brandt §6.2–6.3]. It costs about 16–64 multiply-adds per edge plus a table of a few KB. Keep it as an upgrade path only.
- Upgrade path if measurements demand it: the 4-point B-spline PolyBLEP. It costs 13 multiplies and 15 adds per edge with 2 samples of latency, and is perceptually alias-free for sawtooth up to about 7.8 kHz at 44.1 kHz [VPN Table VIII, IX].

## Why naive edges alias

Rounding each transition to the nearest sample is the same as sampling a signal that is not bandlimited. Discontinuous waveforms have spectra that fall no faster than 6 dB/octave, so infinitely many harmonics fold back. The result is heard as pitch-period jitter, roughness and inharmonic tones [S&S §2; Brandt §2]. Aliasing is least objectionable at low fundamentals and gets worse as pitch rises [S&S §2].

A 40106 noise box spends a lot of time at high, inharmonic, sync-modulated pitches, which is the worst case.

## Option 1: oversampling (2x/4x) plus decimation

**How it works.** Render the naive square at N·fs, low-pass it, and decimate to fs.

**Aliasing.** The naive edge still aliases at N·fs. Oversampling only pushes the folding point up. Because harmonic amplitude falls about 6 dB/octave [S&S §2], the components that fold back into the audio band are only about 6·log2(N) dB weaker than at 1x **(inference)**. That is roughly 6 dB at 2x and 12 dB at 4x. [VPN §I] states it directly: "since spectra of periodic waveforms decay gently ... a very high oversampling ratio is required for proper aliasing suppression." For comparison, [VPN] measured 2-point PolyBLEP at about 70 dB better than trivial sampling, by noise-to-mask ratio, for a sawtooth at 2 kHz (Fig. 11).

**Cost (inference).** The cost is per output sample, not per edge.
- Any per-sample work in the engine runs N times as often, including capacitor-voltage integration if that is done per sample rather than analytically.
- Decimation needs a half-band FIR per 2x stage. As a rough figure, a 31-tap half-band polyphase stage is about 8–16 MACs per output sample. A 4x cascade is about 25–40 MACs per output sample per mixed stream.
- Decimating the final mix (once) instead of each Inverter keeps this bounded.

**Hard sync.** Neutral. Sync edges get rounded to the N·fs grid and alias exactly like other edges.

**Verdict.** Poor value for the edges themselves. It may still be worth it later for nonlinear post-processing (drive, soft clipper; handoff §6.5), which creates aliasing that BLEP cannot fix.

## Option 2: PolyBLEP (polynomial BLEP residual)

**How it works.** The ideal bandlimited step is the sine integral, ½ + Si(π·fs·t)/π [VPN Eq. 10]. The "BLEP residual" is that ideal step minus the naive step. Add samples of the residual around every discontinuity of the naive waveform [VPN §II]. PolyBLEP replaces the residual with a low-order polynomial obtained by integrating a Lagrange or B-spline interpolation kernel. No table and no oversampling are needed [VPN abstract, §III].

**2-point form** (integrated linear interpolation; NI = 2) [VPN Table I]. A step of height `A` lies between two samples. `d ∈ [0,1)` is its distance in samples to the *following* sample [VPN Eq. 12–13, Fig. 3]:

```
sample before edge  += A * ( d*d/2 )
sample after edge   += A * ( -d*d/2 + d - 1/2 )
```

Because the event engine already knows `t_edge`, `d` comes straight from it. No phase-counter division is needed, unlike the oscillator case in [VPN Eq. 13] **(inference)**. The method corrects the sample *before* the edge, so output must be delayed 1 sample (the z⁻¹ in [VPN Fig. 6]).

**Quality** [VPN Table VIII, §IV]. These are perceptually alias-free limits for a sawtooth at 44.1 kHz:

| Method | Samples touched | Highest alias-free f0 |
|---|---|---|
| LUT-BLEP, K = 4 | 4 | 358 Hz |
| LUT-BLEP, K = 32 | 32 | 2036 Hz |
| PolyBLEP NI = 2 (linear) | 2 | 2135 Hz |
| B-spline PolyBLEP NI = 3 | 3 | 4591 Hz |
| LUT-BLEP, K = 64 | 64 | 4595 Hz |
| Lagrange PolyBLEP NI = 4 | 4 | 5134 Hz |
| B-spline PolyBLEP NI = 4 | 4 | 7845 Hz |

(LUT-BLEP is a windowed, 64x-oversampled table with a Blackman window [VPN Table VIII caption].)

[VPN] concludes that a table needs "over ten times more samples around each discontinuity" to match the polynomial method. A square or pulse wave has two edges per period, each the same kind of step, so the per-edge behaviour carries over **(inference)**. At 48 kHz the thresholds should scale roughly with fs **(inference)**.

**Cost** [VPN §IV.C, Table IX], per discontinuity, on top of the naive waveform:
- NI = 2: 4 multiplies, 4 adds.
- Lagrange NI = 4: 14 multiplies, 15 adds.
- B-spline NI = 4: 13 multiplies, 15 adds.
- No memory beyond NI delay cells.
- Use even orders only: odd orders need branching and cost at least as much as the next even order [VPN §III.A].

**Side effect.** PolyBLEP slightly attenuates the top harmonics. [VPN Table IV] gives a fixed 3-tap linear-phase EQ, `b0 + b1 z⁻¹ + b0 z⁻²`, that brings harmonics below 15 kHz to within 1 dB. For example, NI = 2: b0 ≈ −0.1469, b1 ≈ 1.2674. This is optional; for a noise box the slight dulling is probably fine **(inference)**.

**Hard sync (inference, from the linearity in [VPN §II] and [Brandt §6.3]).** A sync-forced flip is a step at a known time with a known height. It gets the same correction as any other edge.

The one trap is copied "polyBLEP oscillator" snippets. These detect the discontinuity from a phase counter wrapping and assume one wrap per period. That breaks under sync. Instead, emit corrections from the event list into a small additive residual buffer, so that any number of edges per sample, from any cause, superpose correctly.

## Option 3: minBLEP / table BLEP

**How it works** [Brandt §4.2, §6]:
1. Take a windowed sinc (Brandt uses Blackman, Ω = 64x oversampled, Nz = 16 zero crossings).
2. Integrate it to a BLEP, which removes the leaky integrator and the DC problems of BLIT.
3. Make it minimum-phase (cepstral method, MATLAB `rceps`), which removes the lookahead.
4. At each edge, add the residual read from the table by sub-sample offset, using linear interpolation between table points.

"Don't just jump to the new level, mix in a MinBLEP instead" [Brandt §6.3].

**Hard sync.** This is the case the paper was written for, titled "Hard Sync Without Aliasing" [Brandt].
- Square, sawtooth and hard-synced square/sawtooth are C1-continuous apart from the jumps, so replacing each step with a MinBLEP suffices [Brandt §6.3].
- Hard-synced triangle or sine needs ramp corrections too (see the BLAMP section below).
- Cost per sample is "roughly proportional to f1" (the edge rate). Very high sync rates are "problematic for real-time", and a sub-linear method was an open problem [Brandt §7]. The same scaling applies to PolyBLEP, but its per-edge constant is about 10x smaller **(inference, from [VPN Table IX] vs the LUT cost below)**.

**Cost.**
- Per correction sample with an interpolated table: 2 reads, 2 multiplies, 2 adds, plus one multiply for addressing. For K samples that is 2K reads, 2K+1 multiplies, 3K adds [VPN §IV.C].
- [VPN] says the K = 32 table needs "almost 450 operations" to match the 8-operation PolyBLEP.
- Table memory is about Ω × K floats, e.g. 64 × 32 × 4 B ≈ 8 KB, or double that with a delta table for interpolation **(inference)**.
- Latency is zero: minimum phase, no lookahead [Brandt §6.2].

**Verdict.** Best quality per tap and zero latency, but expensive per edge and uses memory. Worth it only if a 1–2-sample delay matters (it doesn't here: 1 sample at 48 kHz ≈ 21 µs) or if measured aliasing at high sync rates is still audible with 4-point PolyBLEP.

## Related: BLIT (Stilson and Smith 1996)

BLIT bandlimits the *derivative* (an impulse train of windowed sincs), then integrates [S&S §3.8, §4]. Integration needs leaky integrators and DC-offset fix-ups. These are awkward when frequency, duty cycle or phase change abruptly, as they do under sync [S&S §4.3; Brandt §5]. Pre-integrating to a BLEP removes all of this [Brandt §6.1]. Rule BLIT out in favour of the BLEP family.

## Related: corners as well as steps (BLAMP)

If the engine ever outputs a **capacitor voltage** (an exponential ramp with slope breaks at each threshold crossing, and wherever a **Coupling link** source flips), those are first-derivative discontinuities, not steps. BLEP does not fix them. The matching tool is the bandlimited ramp (BLAMP) and its polynomial form, polyBLAMP [Esqueda]. Brandt also notes that hard-synced triangles need "bandlimited ramps" [Brandt §6.3]. The polyBLAMP correction is scaled by the change in slope at the corner, which the event engine also knows **(inference)**.

## Embedded cost sketch (inference; FM-1 CPU not yet verified)

The FM-1's processor, clock and sample rate are still open questions (handoff "Verify before depending on them"). As a rough upper bound:
- 6 Inverters × 2 edges per cycle × 4 kHz ≈ 48k edges/s.
- At an assumed ~20–40 cycles per 2-point PolyBLEP edge, including buffer bookkeeping, that is about 1–2 M cycles/s.
- That is a few percent of a ~100 MHz MCU. The Felucca emulator defaults to 100 MHz per the community guide; that is not a hardware spec.
- minBLEP at K = 32 would be about 10x that. 4x oversampling would multiply all per-sample engine work by 4, plus the decimator.

## Recommendation for #15 / the engine

1. Keep the split from handoff §6.5. The physical-state simulation emits `(t_edge, ΔV)` events. A separate reconstruction stage turns events into samples.
2. In reconstruction, start from the naive held level. For each event, add `ΔV × residual(d)` into a 2–4-sample additive ring buffer. Delay output by the kernel's lookahead.
3. Start with 2-point PolyBLEP. Measure aliasing at high pitch and under fast Sync-link modulation. If needed, swap in the 4-point B-spline kernel [VPN Table VII] behind the same interface.
4. Do not oversample for the edges. Reconsider 2x oversampling only around nonlinear output stages.
5. If capacitor voltages become audible taps, add polyBLAMP for their corners.

## Sources

- **[S&S]** T. Stilson, J. O. Smith, "Alias-Free Digital Synthesis of Classic Analog Waveforms," Proc. ICMC 1996. https://ccrma.stanford.edu/~stilti/papers/blit.pdf
- **[Brandt]** E. Brandt, "Hard Sync Without Aliasing," Proc. ICMC 2001, pp. 365–368. https://www.cs.cmu.edu/~eli/papers/icmc01-hardsync.pdf
- **[VPN]** V. Välimäki, J. Pekonen, J. Nam, "Perceptually informed synthesis of bandlimited classical waveforms using integrated polynomial interpolation," J. Acoust. Soc. Am. 131(1), 974–986, 2012. DOI 10.1121/1.3651227. https://mac.kaist.ac.kr/pubs/ValimakiPeknenNam-jasa2012.pdf
- **[VH07]** V. Välimäki, A. Huovilainen, "Antialiasing Oscillators in Subtractive Synthesis," IEEE Signal Processing Magazine 24(2), 116–125, 2007. DOI 10.1109/MSP.2007.323276. This is where PolyBLEP was first proposed. Not read in full here; its content is cited via [VPN §III].
- **[Esqueda]** F. Esqueda, V. Välimäki, S. Bilbao, "Rounding Corners with BLAMP," Proc. DAFx-16, Brno, 2016. https://dafx.de/paper-archive/details/xRM0GiuF-JmeN0_aaP9VNg. Only the abstract and listing were consulted.
- FM-1 community guide: https://fuleo.github.io/fm1-guide/. Checked for CPU specs; none are stated.
