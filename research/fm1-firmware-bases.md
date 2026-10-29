# FM-1 firmware bases and their licences

Research for [#4](https://github.com/johnharkinz/40106-lab/issues/4). Read on 2026-10-29 from the repositories' own
README, LICENSE, LICENSING, NOTICE and build files (commit hashes given where they matter). Every claim is marked
**Verified** (read in the primary source) or **Lead** (inferred, reported second-hand, or not checked).

## Short answer

- Yes: **`felucca-bsp` exists and is meant for standalone firmware**. It has no audio engine and no UI. It holds
  the chip and board layer only. It is **MPL-2.0**, and its README says outright "Your own code on top of it may use any licence."
- The practical base for a new firmware is **`fm1-base`** (charlesvestal), **MPL-2.0**. It sits on felucca-bsp as a
  submodule and adds the platform layer: screen and drawing, panel, LEDs, audio callback, MIDI, storage, a host
  simulator, a browser emulator, build and packaging tools, and a `template/` app. Two shipped firmwares use it (CTL-1, Radio).
- **Felucca itself is GPL-3.0-only** and its maintainer has stepped back. A fork of Felucca stays GPL even if it
  uses the BSP.
- There is now also an **official M-VAVE open firmware (`m-vave/fm1_open`)**. It is GPLv3 plus an SDK linking
  exception, links a proprietary binary engine, and builds on Windows only. It is not a good fit for 40106 Lab.
- **Recommendation:** start a new project on **fm1-base + felucca-bsp** and write our own app code. Do not copy any
  Felucca source. We can choose our own licence, and only modified fm1-base/BSP files must be published under MPL-2.0.

## Candidates

| Base | Licence | State | Fit for 40106 Lab |
| --- | --- | --- | --- |
| [hugelton/felucca-bsp](https://github.com/hugelton/felucca-bsp) | MPL-2.0 (two Apache-2.0 radio files) | Beta; HEAD `305c372`, 2026-10-26 | Hardware layer only: too low-level to use on its own, but it underlies fm1-base |
| [charlesvestal/fm1-base](https://github.com/charlesvestal/fm1-base) | MPL-2.0 | Beta; `plat.h` and `fm1.toml` "expect to change before 1.0" | **Best fit**: the app sees only `plat.h`; it has a simulator and template |
| [hugelton/Felucca](https://github.com/hugelton/Felucca) | GPL-3.0-only | v1.7.0; "Maintenance Notice": occasional maintenance only | Fallback (fork); the whole app becomes GPL-3.0-only |
| [hugelton/Felucca-Engines](https://github.com/hugelton/Felucca-Engines) | MIT | Maintained | DSP reference only; we have our own engine |
| [m-vave/fm1_open](https://github.com/m-vave/fm1_open) | GPLv3 + FM1 SDK linking exception; proprietary `libfm1_engine.a` | Official V14, 2026-10-21 | Poor: Windows-only toolchain, closed engine, GPL app |
| Felucca forks (fm1-x0x, Optimist, skiff, jangada, hortator, caravel, …) | Mostly GPL-3.0 (inherited) | Active | Not as bases: same GPL inheritance as Felucca |

## felucca-bsp

**Verified** (README, LICENSING.md, NOTICE at `305c372`):

- Purpose: "A base for custom firmware on M-VAVE synthesizers built around JieLi chips … It has no sound code and no
  user interface." The FM-1 is "Supported. Used by Felucca"; SMK-37 is planned.
- Layout: `chip/` (clock, IRQ, flash, USB device MIDI/serial/audio, UART, timers, OTA session, guards, perf counters),
  `boards/fm1/` (LCD pins, key scan, ADC, audio codec), `loader/` (update loader), `tools/` (`fm1pkg.py` package
  builder, `fm1_install.py` USB-MIDI installer, `fm1_rescue` (macOS) to restore stock firmware, `get_sdk.sh`),
  `examples/hwtest/` (a hardware test firmware that ran on an FM-1 on 2026-10-16).
- Audio API (`boards/fm1/fm1_audio.h`): ALNK0 I2S to an external codec. The DMA is double-buffered with int32 L/R
  samples, 24-bit left-justified, and the app fills the free half in the ALNK0 ISR.
- Beta parts: second core, expanded flash, USB MIDI host. A higher clock is still a concept (`docs/EXPERIMENTAL.md`).
  The safe `hwtest.bin` contains none of them.
- Licence: MPL-2.0 per file, with no "Incompatible With Secondary Licenses" notice, so it can be combined with
  GPL code. Leo Kuroshita relicensed his Felucca-origin files from GPL-3.0-only (felucca-bsp#4), and keremimo did the
  same for his part of `usb.c` (felucca-bsp#3). Copies already distributed in Felucca stay GPL-3.0-only.
- **Forks of Felucca do not become MPL by using the BSP**: "To publish under MPL-2.0 (or any other licence), start a
  new project, import this repository, and write the rest yourself. Do not copy Felucca's code into it."
- Exceptions: `chip/ble_rf_tables_v15.h` and `chip/fm1_rf_cal.h` are Apache-2.0 (JieLi / M-VAVE radio material).
  We need them only if we use Bluetooth.

## fm1-base

**Verified** (README, LICENSING.md, DESIGN.md, `include/plat.h`, `template/`):

- Layers: app → `plat.h` → fm1-base (device: `plat_fm1.c`, `lcd.c`, `gfx.c`, `panel.c`, `storage.c` …; host:
  `plat_host.c` simulator, `web/emu`) → felucca-bsp (pinned git submodule). "The app sees only `plat.h`… every app runs,
  unchanged, on the FM-1, in the host simulator (tests, screenshots) and in the browser."
- The app implements `app_init`, `app_frame` (UI at about 60 Hz), `app_render` (audio, called from the audio
  interrupt), an optional `app_tick` (1 kHz), `app_say` and `app_config`.
- Audio: the template has `void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)`, which is n stereo frames
  of 24-bit samples. `APP_BLOCK 256u` is commented "5.8 ms at 44.1 kHz". So the **device rate is 44.1 kHz, not the
  48 kHz assumed in the handoff**. CPU load and xruns are available as `plat_cpu_pct` and `plat_xruns`.
- Controls (`plat.h`): 14 buttons `B_FX, B_SEL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY,
  B_REC, B_OCTDN, B_OCTUP`, 7 encoders `EN_SELECT, EN_ALGO, EN_PRESET, EN_K1..EN_K4`, 27 keys (F3–G5), the MASTER
  pot (0..4096), and LEDs per button and key. Optional key velocity and pressure.
- Display: the template draws a 240×240 area (`lcd_fill(0, 0, 240, 240, …)`, `cv_begin(240, …)`).
- Storage: `plat_store_load/save` objects of up to `PLAT_STORE_MAX` 3840 bytes each, plus a flash data area for big data.
- Memory: a 581 KB app slot in flash, 96 KB RAM for `.data`/`.bss`, and a 336 KB pool. The template without a radio
  uses 77 KB flash, 27 KB RAM and 68 KB pool (measured October 2026).
- Build: one `tools/build.py` driven by `fm1.toml`. It enforces "no `double`, no hardware access outside the BSP and
  fm1-base's device files" and packs the `.fwsc`. `tools/run_tests.sh` builds the host simulator and runs scripted tests.
- Licence: every file is MPL-2.0. "Your own files, such as an app built on fm1-base, may use any licence, closed ones
  included." Its imports were checked file by file so that no GPL code and no code by other Felucca contributors is in it.
- Bluetooth/WiFi are optional. A build without a radio links nothing of JieLi's (its README says so).

## Toolchain, SDK and vendor terms

**Verified:**

- Compiler: the **JieLi Linux toolchain, clang 4.0.1 for pi32v2**. It is Linux x86-64 only. On macOS, Felucca's
  build runs it in a `linux/amd64` Docker container (Rosetta on Apple silicon) (Felucca `BUILDING.md`). Felucca's
  `tools/get_toolchain.sh` downloads it from `https://pkgman.jieliapp.com/s/linux-toolchain`.
- **JieLi AC79 AIoT SDK** is Apache-2.0. The Gitee API reports `license: Apache-2.0` and the LICENSE file is Apache 2.0
  (gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK). A package needs three SDK files (`uboot.boot`, `cfg_tool.bin`,
  `eq_cfg_hw.bin`), which are fetched at build time and never committed. **Every released package must ship
  `LICENSES/Apache-2.0.txt`** (felucca-bsp LICENSING.md, "Tools").
- felucca-bsp itself was "written against the public documentation and headers" of the SDK, and "no SDK source is
  intended to be included" (NOTICE).

**Leads / open:**

- **The toolchain's own licence terms are not stated anywhere we read.** It is a binary download from JieLi's package
  server. Its redistribution terms are unknown, so do not redistribute it; document how to fetch it instead.
- felucca-bsp's LICENSING.md: the SDK's `*.a` libraries "carry no separate licence statement… That has not been
  confirmed with JieLi." This matters only if we link JieLi's Bluetooth/WiFi libraries, which a no-radio build does not.
- `tools/get_sdk.sh` defaults to `https://github.com/Jieli-Tech/fw-AC79_AIoT_SDK`, which returned **HTTP 404** on
  2026-10-29 (the Jieli-Tech GitHub org exists, but that repo was not visible). The Gitee copy is live. Expect to set
  `SDK_URL` to Gitee. The pinned revision is `d179b44…` (V1.2.1, 2023-12-13). Not tried.
- The handoff's `fuleo.github.io/fm1-guide` was not checked.

## m-vave/fm1_open (official)

**Verified** (README, LICENSE.md, THIRD_PARTY.md, HEAD `4c7496d` 2026-10-21):

- App sources: GPLv3 with an "FM1 SDK linking exception" (GPLv3 §7) that allows linking with the proprietary
  `sdk/lib/libfm1_engine.a`. The proprietary engine may be used and redistributed only for M-VAVE FM1 devices, and no
  source for it is supplied.
- Build: Windows x64, PowerShell, JieLi Windows toolchain 2.5.3, with SHA-checked toolchain files.
- The boot/update resources and the stock recovery image `FM-1_015.fwsc` are **Apache-2.0**. This is a documented,
  redistributable route back to stock firmware.
- Fit: it doesn't match the handoff's macOS/Linux build or its "portable engine, minimal SDK coupling" principles, and it forces the GPL.

## What this means for publishing 40106 Lab

On fm1-base + felucca-bsp, writing our own app code without copying Felucca code:

1. **Our own files** (`engine/`, `firmware/` app code) can use any licence we choose.
2. **fm1-base and felucca-bsp files** stay MPL-2.0. If we modify any of them and distribute binaries, we publish the
   source of those modified files. Keeping them as unmodified submodules is the easy path.
3. **Binaries (`.fwsc`)** contain three Apache-2.0 JieLi SDK files. Ship `LICENSES/Apache-2.0.txt`, and also the
   MPL-2.0 text and the source pointer for the MPL parts.
4. **Fonts** in fm1-base are SIL OFL 1.1 (Barlow, Terminus). Ship their licence texts if we use them.
5. **Avoid** copying from Felucca, its forks or Melodee's GPL copies: any such code makes 40106 Lab GPL-3.0-only.
   Felucca-Engines code is MIT and safe to borrow with attribution, but we probably don't need it.
6. Don't enable Bluetooth/WiFi in v0.1. That keeps JieLi's binary libraries and the extra Apache/BSD notices out.

If we chose the fallback and forked Felucca instead, all of 40106 Lab would have to be GPL-3.0-only.

## Flashing and recovery (for #13 and later)

**Verified, from the docs only, not tried:** install is over USB-MIDI (`fm1_install.py`, or a web installer in
Chrome/Edge). hwtest/fm1-base firmware falls back to the chip's UBOOT mode after two failed boots. From UBOOT,
`tools/fm1_rescue.sh` (macOS) restores the stock firmware. Felucca also points to
[FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter) for recovery. M-VAVE's updater and workbench can
return a unit to official V15. **None of this has been tried on the user's FM-1.**

## Open questions this raises

- Does the pi32v2 core have hardware float? The template uses `float` in `app_render` and the build forbids `double`.
  The CPU budget for our exponential RC updates and oversampling needs measuring (M3).
- Is 44.1 kHz fixed, or can it be configured? The handoff's desktop renderer assumes 48 kHz.
- fm1-base is beta and `plat.h` may change before 1.0. Pin the submodule revision.
