# M-VAVE FM-1: hardware and SDK constraints

Research for issue #3 (parent map #2). Gathered 29 October 2026.

There is no official M-VAVE hardware datasheet. Nearly every hardware fact below comes from **source code that
runs on real FM-1s**: register-level drivers, linker scripts and SDK headers. Treat these as primary sources for
how the device behaves, and as secondary sources for the silicon itself.

**Confidence levels used here**

- **Verified**: stated in code or headers that drive the hardware, and ideally confirmed by a second independent
  codebase.
- **Lead**: stated once, measured by someone else, derived by me, or marked "beta" or "concept" upstream. Check it
  on hardware before relying on it.

## Sources (pinned commits)

| Key | Source | Licence | Commit |
| --- | --- | --- | --- |
| BSP | [hugelton/felucca-bsp](https://github.com/hugelton/felucca-bsp): bare-metal chip and board layer | MPL-2.0 | `305c372c3ec1` |
| FEL | [hugelton/Felucca](https://github.com/hugelton/Felucca): a full synth firmware built on BSP | GPL-3.0-only | `5d9694d0475a` |
| BASE | [charlesvestal/fm1-base](https://github.com/charlesvestal/fm1-base): platform layer plus host simulator and browser emulator | MPL-2.0 | `d8ff8f41aa51` |
| MV | [m-vave/fm1_open](https://github.com/m-vave/fm1_open): **M-VAVE's official** open firmware (V14) with a binary SDK | GPLv3 + FM1 SDK linking exception; proprietary `libfm1_engine.a` | `4c7496daac04` |
| ENG | [hugelton/Felucca-Engines](https://github.com/hugelton/Felucca-Engines): fixed-point DSP engines | MIT | HEAD, 29 Oct 2026 |
| GUIDE | [fuleo.github.io/fm1-guide](https://fuleo.github.io/fm1-guide/): community user guide | n/a | 29 Oct 2026 |
| EMU | [joakimeriksson/fm1-emu](https://github.com/joakimeriksson/fm1-emu): whole-device emulator | n/a | README, 29 Oct 2026 |
| JL | [Jieli-Tech/fw-AC79_AIoT_SDK](https://github.com/Jieli-Tech/fw-AC79_AIoT_SDK): the chip vendor's SDK, rev `d179b448` (V1.2.1), as pinned by BSP `tools/get_sdk.sh` | Apache-2.0 | not read directly |

GUIDE is a user guide. It gives no processor, RAM, flash, audio or display specifications; it only confirms the
control count and points to MV and EMU.

## Processor, clock and memory

| Fact | Value | Status | Source |
| --- | --- | --- | --- |
| SoC | JieLi **AC79** family, `pi32v2` core (compiler target `-mcpu=r3`) | Verified | BSP `docs/CPU.md`, `NOTICE`; BASE `tools/build.py`; MV README ("FM1 continues to use the `r3` CPU target") |
| Cores | 2. CPU1 is usable but **beta**: on some units CPU1 sometimes misreads shared RAM, so code needs a single-core fallback | Verified (the misread is a field report) | BSP `chip/fm1_cpu1.h`, `docs/EXPERIMENTAL.md`; FEL `core.h` `FELUCCA_CPU2` |
| System clock | **360 MHz** under M-VAVE's bootloader (PLL1 540 MHz × 2/3). Peripheral buses: HSB 180 MHz, LSB 60 MHz. The third-party m-bootloader 1.2 runs it at 320 MHz | Lead (measured once, 23 Oct 2026, with fm1-x0x's debug build) | BSP `chip/fm1_clock.h`, `docs/CPU.md` |
| Clock control | Raising the clock is a **concept only**. Nothing has run | Verified (as status) | BSP `docs/EXPERIMENTAL.md` |
| FPU | **Single-precision hardware FPU** (`-mfprev1`). `double` falls back to software routines, which BASE's build rejects | Verified | BASE `tools/build.py` lines 125–128 and 768–775 |
| Fixed-point bug | 32×32→64-bit multiplies inlined next to 16-bit loads occasionally corrupt a register. Workaround: put those multiplies in `noinline` helpers. Float code has not shown the bug | Verified (repeated bench tests) | BSP `docs/CPU.md` |
| RAM | **512 KiB internal SRAM** (0x01C00000–0x01C80000). **No external RAM** | Verified | FEL `firmware/app.ld`; BASE README ("the chip has no external RAM") |
| RAM budget for an app | FEL: 72 KiB `.data`/`.bss` + 360 KiB pool, plus stacks, RAM code and noinit. BASE: 96 KiB + 336 KiB pool (128 + 304 in a WiFi build) | Verified | FEL `app.ld`; BASE README "What each costs" |
| Flash | Stock **1 MiB SPI NOR**, SOIC-8 (JEDEC `85 60 14`). Code runs in place (XIP) from 0x02000120 | Verified | BSP `chip/fm1_flash_ext.h`, `chip/fm1_flash.h`; FEL `app.ld` |
| App code slot | **~581 KB** (XIP length 0x8DFBC) | Verified | FEL `app.ld`; BASE README |
| Larger flash | Code exists for a swapped, bigger chip but has **never run on a device** | Lead | BSP `docs/EXPERIMENTAL.md` |

**Derived budget (lead, my arithmetic):** 360 MHz ÷ 44,118 Hz ≈ **8,160 CPU cycles per stereo sample** on one
core, before UI, key scan and interrupt overhead. Felucca sheds voices above 85 % load. BASE calls `app_overload`
above 90 %.

## Audio path

| Fact | Value | Status | Source |
| --- | --- | --- | --- |
| Interface | ALNK0 I2S to an **external codec**, using double-buffered DMA. The codec part number is **not identified** in any source | Verified (codec unknown) | BSP `boards/fm1/fm1_audio.h` |
| Sample format | Stereo interleaved `int32` slots holding **24-bit left-justified** samples. MV's SDK describes USB PCM as signed Q23 | Verified | BSP `fm1_audio.h`; MV `sdk/include/fm1_audio_io.h` |
| Sample rate | Nominal **44.1 kHz**. Actual **44,117.6 Hz** (24 MHz ÷ 544); MV hard-codes `FM1_AUDIO_HARDWARE_RATE 44118` | Verified (two codebases agree) | FEL `src/audio.c` `DAC_TICKS`; MV `sdk/include/fm1_block.h`, `app/board/fm1_audio.h` |
| Other rates | ENG builds its tables for 32–96 kHz. Nobody shows the I2S clock running at any rate other than ~44.1 kHz on the device | Lead | ENG README "Sample rate" |
| Block size | MV SDK: fixed `FM1_BLOCK_FRAMES 128` (2.9 ms). FEL: `HALF_FRAMES 128`. BASE: `APP_BLOCK` default 256 (5.8 ms), set per app | Verified | MV `fm1_block.h`; FEL `src/core.h`; BASE `include/fm1b_config.h` |
| Callback model | No OS callback. The DMA half-buffer **interrupt** renders the next half. BASE: `app_render(int32_t *lr, uint32_t n, uint32_t master_q12)` runs in the audio interrupt. MV: audio code must carry `FM1_AUDIO_CODE` (runs from RAM, so it stays safe while flash is being written) | Verified | BSP `fm1_audio.h`; BASE `template/app.h`; MV `sdk/include/fm1.h` |
| Headroom | FEL renders in Q15 and shifts up to 24-bit with a −6 dBFS ceiling | Verified (one implementation's choice) | FEL `src/audio.c` `OUT_SHIFT` |
| Outputs | Speaker and headphones from the DAC. USB Audio Class in and out over the cable. MASTER is an analog pot read by the ADC and applied as digital gain | Verified | FEL `src/audio.c` comments; BSP `fm1_adc.h`; BASE README |
| Flash writes vs audio | Flash writes and erases briefly silence audio. Audio code cannot run from flash while it is busy | Verified | MV `fm1.h` flash section; BSP `fm1_flash.h` |

## Display

| Fact | Value | Status | Source |
| --- | --- | --- | --- |
| Panel | **240×240** IPS, ST7789V-class controller | Verified | FEL `src/lcd.c`; MV `FM1_SCREEN_WIDTH/HEIGHT 240` |
| Colour | **RGB565**, 16-bit; the panel expects big-endian byte order | Verified | FEL `lcd.c` (COLMOD 0x55), `gfx.c`; MV `fm1_screen_blit` |
| Bus | SPI1, write-only, DMA from RAM; 12 MHz at the default divider (LSB 60 MHz ÷ 5). Source buffers must be in RAM | Verified | BSP `fm1_lcd_hw.h`; FEL `lcd.c` |
| Refresh | No tearing/TE line is used. **Full-frame maximum ≈ 13 fps** at 12 MHz (115,200 B per frame). In practice, redraw partial regions in strips (MV DMA band 240×16 px; BASE canvas 240×124 or ×48) | Lead (my arithmetic; the divider is adjustable but untested) | derived from BSP/FEL |
| Backlight | On/off only (PA2, active low) | Verified | BSP `fm1_lcd_hw.h` |

## Physical controls

All verified from BSP `boards/fm1/fm1_input.h`, BASE `include/plat.h` (by printed label) and MV `fm1_controls`.

- **7 rotary encoders**, detented, 1 click = 1 step: `SELECT`, `ALGO`, `PRESET`, `K1`–`K4`.
  GUIDE: "four knobs on the left, KNOB 1–4 on the right".
- **14 buttons**: `FX`, `SEL`, `ENV`, `LFO`, `EDIT`, `GLO`, `HOME`, `SAVE`, `ARP`, `SEQ`, `PLAY`, `REC`, `OCT-`, `OCT+`.
  GUIDE counts 12 buttons, without the octave pair. Each button has an LED; `PLAY` also has a second, green LED.
- **27 keys**, F3–G5, with LEDs. They are carbon-pill keys read through a diode matrix (no ghosting).
  Velocity and pressure can be sensed through the ADC, off by default (`APP_TOUCH`).
- **1 analog pot**: `MASTER`, 0–1023.
- **Battery** level is readable through the ADC.
- Scanning: one 11×6 matrix behind two 74HC595 chips, polled by a ~10 kHz timer interrupt. A full frame takes about 1.1 ms.
  LEDs support off, dim glow, breathing, mid and full brightness.
- Other I/O: USB (MIDI, serial, audio; USB host is experimental). MIDI in over TRS
  (`fm1_io_trs_midi_enabled`; MV also lists DIN and BLE input). Bluetooth LE and WiFi exist but each uses most of the RAM;
  you cannot have both.

The handoff's proposed control mapping (Edit, FX, SEQ, Home, Save, Play) matches real printed labels. The
"main selection", "patch/preset" and "connection/algorithm" encoders map naturally onto `SELECT`, `PRESET` and
`ALGO`, and the four parameter encoders map onto `K1`–`K4`.

## Storage

| Fact | Value | Status | Source |
| --- | --- | --- | --- |
| Medium | The internal 1 MiB flash only. **No SD card** | Verified (no SD driver in any codebase) | BSP, BASE, MV |
| App data area | 0x97000–0xDFFFF (~292 KiB), shared with the app's own data | Verified | BSP `fm1_flash.h` `FL_DATA_LO/HI`; BASE `plat.h` |
| BASE store | Key/value objects of up to **3,840 B** each, written A/B across two 4 KiB sectors | Verified | BASE `plat.h` `PLAT_STORE_MAX` |
| MV raw API | `fm1_flash_read/write/erase`, 4 KiB sectors. Not transactional. Control task only | Verified | MV `sdk/include/fm1.h` |

A few patches of a dozen or so parameters each fit easily.

## Simulators and emulators

| Tool | What it does | Status | Source |
| --- | --- | --- | --- |
| BASE host simulator (`host/plat_host.c`, `host/rfm1.c`) | Builds the **same app source** for the Mac, driven by a script, with a framebuffer | Verified (exists) | BASE README, `include/plat.h` |
| BASE browser emulator (`web/emu`) | The same app compiled to WASM. Also a `.rfm1` build for RemoteFM1 (iOS/AUv3) | Verified (exists) | BASE README |
| fm1-emu | Emulates the **CPU, screen, keys and audio**, and runs unmodified `.fwsc` firmware (stock firmware with both cores). Runs in a browser, natively, or as AU/VST3 | Verified (exists); accuracy not assessed | EMU README |
| FEL web emu, MV web simulator | Firmware-specific browser builds | Verified (exist) | FEL `web/emu`; MV `sdk/Simulator.js`, `web/` |

## SDK and toolchain options

1. **M-VAVE official (MV)**: binary `libfm1_engine.a` (FreeRTOS-based; control-task, routing-task and audio ISR
   contexts) with a GPLv3 + linking-exception app. The binary SDK may be used **only on FM-1 devices**. The build is
   Windows-only and needs JieLi toolchain 2.5.3, which the build checks by hash.
2. **felucca-bsp (BSP)**: bare-metal, MPL-2.0, no RTOS. Each file stays MPL-2.0; your own code can use any licence.
3. **fm1-base (BASE)**: built on BSP, MPL-2.0. Adds a platform API (`plat.h`), the host simulator, the browser
   emulator, packaging and installer. Apps built on it can use any licence. Beta: `plat.h` may change before 1.0.
4. **Felucca (FEL)** itself is GPL-3.0-only. Copying code from it makes 40106 Lab GPL. Use it as a reference only,
   unless the project accepts GPL.

Toolchain: JieLi `pi32v2` clang 4.0.1. It is **Linux x86-64 only**; on macOS, FEL and BASE run it in a
`linux/amd64` Docker container (Rosetta on Apple silicon). Installing needs no extra hardware: the web installer
works over USB, and M-VAVE's updater or "Return to official V15" restores stock firmware (FEL README).

## Implications for 40106 Lab (leads for #13 and #15)

- Plan the circuit engine for **44,118 Hz** (not exactly 44,100), **128- or 256-frame blocks**, and **24-bit stereo**
  output in `int32` slots.
- **Single-precision float** is available in hardware. Keep everything `float`, never `double`. If you choose fixed
  point, keep 64-bit multiplies in `noinline` helpers.
- Oversampling: 2× means ~4,080 cycles per oversampled step and 4× means ~2,040, shared across all oscillators,
  mixing and filtering. This budget is the main input to #13.
- Plan for **one core**. Treat CPU1 as optional, because some units cannot run it.
- The UI is 240×240 RGB565 with strip-wise partial redraw. Patch view, editor and scope must fit that.
- The most likely platform choice is fm1-base: MPL-2.0, its own host simulator, and the 40106 engine stays
  portable. To be decided under #15.

## Open questions

- What is the codec chip, and what is its analog output level and noise floor? No source names it.
- Can the ALNK/I2S clock run at other rates (48 or 88.2 kHz)? Would a different `DAC_TICKS` work?
- What is the real cycle cost of `expf`/`exp`-style RC updates on pi32v2? Benchmark it on hardware or under fm1-emu.
  Note that fm1-emu's timing accuracy is not established.
- Does the 360 MHz figure hold across units and bootloaders? It is a single measurement.
- What are the actual SPI LCD frame rates at a lower divider (`LCD_BAUD` < 4)?
