# FM-1 flashing and recovery route: research findings

Ticket: #5, "What is the verified flashing and recovery route for the FM-1?"
Date: 2026-10-29. Status: research only. This file describes how the published routes work and how well each one is supported by evidence. It is **not** a procedure, and nothing here should be taken as instructions to flash a device.

## Sources consulted

| Tag | Source | Owner / independence | Snapshot |
|---|---|---|---|
| MV-DL | M-VAVE download page, https://www.m-vave.com/download, and its file store (`FM-1.fwsc`, `FM-1.txt`) | First party (M-VAVE) | fetched 2026-10-29 |
| MV-OPEN | https://github.com/m-vave/fm1_open (README, `LICENSE.md`, `sdk/WebUpdate.js`, `sdk/firmware/FM-1_015.fwsc`) | First party (M-VAVE) | pushed 2026-10-21 |
| FEL | https://github.com/hugelton/Felucca (`README.md`, `BUILDING.md`, `tools/fm1_install.py`, `web/fm1pkg.js`, `web/fm1ota.js`, `firmware/loader/loader.c`) | Leo Kuroshita (kurogedelic) | `5d9694d`, 2026-10-28 |
| FEL-61 | Felucca issue #61, "FM-1 stuck in boot mode (WL80UBOOT)…", https://github.com/hugelton/Felucca/issues/61 | Independent user report (Acrawf1), with a reply from kurogedelic | closed 2026-10-07 |
| FT | https://github.com/kurogedelic/FM-1-transporter (`README.md`, `docs/ARCHITECTURE.md`, `docs/PROTOCOL.md`, `docs/DEVLOG.md`, `docs/logs/`) | **Same author as FEL**, so not independent of it | fetched 2026-10-29 |
| RE | https://github.com/AL-255/FM-1-RE (`docs/io/12-v15-reflash-proof.md`, `TODO_aug2.md`, `analysis/device/uboot/README.md`) | Independent reverse engineer (AL-255) | `ec832f2`, 2026-09-08 |
| JLU | https://github.com/kagaimiq/jl-uboot-tool (`README.md`, `docs/how-to-enter-uboot.md`) | Independent general JieLi tool, not FM-1 specific | fetched 2026-10-29 |
| GUIDE | https://fuleo.github.io/fm1-guide/ and `linux-update.html` | Community guide; cites FEL, RE, and M-VAVE | fetched 2026-10-29 |
| NEO | https://www.neoteo.com/en/m-vave-fm-1-firmware-why-stock-v16-and-open-v14-are-separate | Press article (quotes Ars Technica); secondary | fetched 2026-10-29 |

I found no first-party JieLi documentation of the AC79/WL82 USB download (UBOOT) mode. Everything below about that ROM mode comes from community sources (JLU, FT, RE, FEL-61).

## Hardware facts relevant to flashing

- **MCU**: JieLi AC791N (WL82), pi32v2 CPU, 1 MiB SPI NOR flash with ID `0x856014`. Sources: RE (the SPL is byte-identical to `cpu/wl82/tools/uboot.boot` at AC79 SDK tag `AC79NN_SDK_V1.1.9_2023-08-01`), FT, FEL (JieLi AC79 SDK toolchain), FEL-61 (chip key `0x980F`, flash `0x856014`), GUIDE. **Corroborated by several independent sources.**
- **Flash layout**: `0x0`–`0x4000` holds the header, SPL, and isd_config; `0x4000`–`0x93000` holds the application; `0x93000` onward holds device data. Sources: FT `PROTOCOL.md`, and FEL-61's independent dump (the stock V15 head sat at `0x0`–`0x3FFF` and the app at `0x4000`–`0x92FFF`). Felucca also keeps its own user data at `0x97000`–`0xE0000` and `0xFC000`–`0xFE000`. Its loader may write `0x4000`–`0x97000` and `0xE0000`–`0xFC000` (FEL `loader.c` `FL_RANGE_OK`). **Corroborated.**
- **Single-bank update**: there is one application image and no demonstrated A/B rollback. Sources: RE `TODO_aug2.md` and NEO (quoting Ars Technica). **Corroborated.**
- **No recovery button**: PB01 is a long-press (8 s) reset input. Nothing shows that it enters recovery (RE). GUIDE also says "There is no hardware recovery path… no debug header or recovery button."

## Route A: normal flashing over USB-MIDI SysEx (no hardware)

**How it works.** This is the update path built into stock firmware, and every current installer uses it: M-VAVE's own web workbench, Felucca's web installer and `tools/fm1_install.py`, and fuleo's Linux script. It runs in two steps. (1) The running firmware asks the host for byte ranges of the `.fwsc` package, checks them, and reboots into an update loader carried inside the package. The loader shows up as `ota-<model>_<ver>`. (2) The loader requests the whole image, writes flash, signals `0xE0000000`/`0xF0000000` completion, and restarts. The device drives the transfer by sending requests, and the host only answers them. The vendor protocol header is `00 59 …` with 7-bit packing.

- Evidence: FEL `fm1_install.py` docstring and code; MV-OPEN `sdk/WebUpdate.js` ("Fixed device protocol: 00 59, 7-bit SysEx packing, USB MIDI only"); RE `12-v15-reflash-proof.md`, which records a captured request chain of 48 staging requests, a switch to the loader identity, 1167 flash requests, and a reboot; GUIDE `linux-update.html`.
- **Corroborated by first party (MV-OPEN) plus two independent implementations (FEL, RE).**

**Return to stock (Route A in reverse).** Felucca's installer, M-VAVE's workbench, and RE's client can all write the official V15 package back over the same protocol.

- The V15 package is pinned in three independent places by the same SHA-256, `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a`, size 699956 bytes: FEL `web/fm1pkg.js` and `tools/fm1_install.py` (`STOCK_V15_SHA256`), MV-OPEN `WebUpdate.js` line 140, and RE `12-v15-reflash-proof.md`. I downloaded `sdk/firmware/FM-1_015.fwsc` from MV-OPEN and it hashes to this value. **Strongly corroborated.**
- RE recorded a real rollback from a modified build that reported itself as "16" to stock V15 on hardware, and concluded the version restriction is "at least partly host-side policy rather than a one-way device fuse". That is one device and one run.
- MV-OPEN README: "The workbench supports returning to original V15 and migrating from compatible third-party firmware." NEO (quoting Ars Technica) reports a failed third-party install, stuck at about 80%, that the workbench's force-flash restored to V15.
- GUIDE says that if a write is cut off, the unit stays in its update loader and running the flash again finishes it. FEL `fm1_install.py` says the same thing: "If the FM-1 is still in update mode… the install finishes the write." This rests on the authors' own testing and has not been checked independently for every interruption point.

**Limits of Route A.** It works only while the device still runs firmware, either an app or the OTA loader, that answers on USB-MIDI. RE: "A normal-mode updater inside a non-booting application is not a recovery strategy." MV-OPEN `WebUpdate.js`: "A successful transfer does not guarantee data preservation or recovery from interrupted power."

## Route B: JieLi ROM USB download mode (UBOOT) for hard recovery

**How it works.** The AC791N boot ROM has a USB download mode. It appears as USB mass storage `4C4A:8057` "WL80UBOOT1.00", and a host tool sends vendor SCSI CDBs through it. The host uploads a RAM loader (`wl82loader.bin` from JLU) and then reads, erases, and writes flash directly, without needing any working firmware on the FM-1. There are three known ways in:

1. **Automatic on a failed boot.** JLU: "The chip can automatically enter the USB download mode if it couldn't boot off the main flash." FEL-61 is an independent case of this: after a Felucca 1.0.1 install the FM-1 sat in `WL80UBOOT1.00` on a normal USB cable. **Observed on real FM-1 hardware.**
2. **USB_KEY signal at power-on.** The key `0x16EF` is clocked over D+ and D-. Normally a vendor "USB Updater" dongle sends it (JLU). For the FM-1, FM-1-transporter has an RP2040 (Seeed XIAO) send it, wired to D+, D-, and GND with VBUS left unconnected (FT). FT's DEVLOG covers one board on 2026-10-01: UBOOT entry, a 1 MiB dump identical to V15, one sector rewrite with V15 still booting, out-of-range writes refused, and key ACK after a watchdog reset.
3. **Soft key from stock V15.** The USB-MIDI packet `04 F0 22 24 07 35 7D F7` moves a running stock V15 into UBOOT (FT `PROTOCOL.md`). Only FT documents this.

**Documented recoveries using Route B.**
- FEL-61 (independent user, Windows): the unit was already in UBOOT. The user ran `jluboottool.py` against the raw disk, extracted the app area from the stock V15 `.fwsc`, wrote only `0x4000`–`0x92FFF` with the head left alone, read it back, matched the SHA-256, and the unit booted V15. This was repeated several times in the same thread. **This is the only independent, first-hand, real-device restore of a non-booting FM-1 that I found.**
- FT's README says that with no working firmware you run `fm1t.py rekey` and then power the FM-1 on. Its DEVLOG does not log restoring a deliberately non-booting unit. Felucca's `BUILDING.md` names FT as the recovery route, but FEL and FT come from the same author.

**Disagreements and caveats.**
- **USB_KEY polarity**: JLU's general docs say D- is clock and D+ is data. FT says that on the FM-1 (WL82) "D+ = clock and D- = data. Only this polarity works." Treat this as unresolved: either WL82 differs or one of the docs is wrong.
- **Tool support**: JLU's own table lists WL82/AC791N as "unknown". FEL-61 nonetheless used it successfully on an FM-1.
- RE's 2026-08-02 verdict ("NO-GO for non-stock flashing… does not prove a ROM-level recovery path") predates FT and FEL-61. Its P0 items are partly covered now (UBOOT entry and dump-and-restore on hardware), but nobody has published a systematic power-loss or interruption test matrix.
- Route B writes raw flash. Writing `0x0`–`0x4000` (head and SPL) or the device-data region could destroy the SPL/boot configuration or per-device data. FT hard-refuses anything outside `0x4000`–`0x93000`. JLU does not, and it offers `erasechip`.

## Important findings for this project

1. **Stock is now V16, not V15.** M-VAVE's download page lists "Version V16 · 2026_10_21" (MV-DL). The current `FM-1.fwsc` is 718196 bytes with SHA-256 `d017fb65e70aca97ee29d944470c3515a74581df5c0576bf70de917c7b2b61d5`. Its release note `FM-1.txt` (GB18030 encoded) describes multitimbral and arrangement changes and contains no update or recovery instructions. Every community installer, M-VAVE's own workbench, and every recovery write-up is pinned to **V15**. The V15 file is still published first-party in `m-vave/fm1_open/sdk/firmware/FM-1_015.fwsc`. **Unverified:** whether V16 keeps the same flash layout, head, and update protocol; whether Felucca, or our own package, can install over V16; and whether V16 can be rolled back to V15. Felucca's installer checks only the model string (`FM-1`), not the version (FEL `fm1ota.js` line 179).
2. **Returning to stock does not wipe custom-firmware data.** In FEL-61, Felucca data records at `0x97000`+ and `0xFC000`+ survived a restore to V15. A later Felucca install then failed to boot because of that leftover data, and erasing those areas in UBOOT fixed it. Firmware we write must not trust leftover flash contents from other firmware.
3. **Package format**: `.fwsc` is a JieLi container. The installable package combines app and loader, and Felucca builds its package by wrapping its app in the official V15 package's head and loader (FEL `BUILDING.md`, `fm1pkg.js` `buildFeluccaPackage`). The build needs three files from the JieLi AC79 SDK (Apache-2.0). MV-OPEN ships its own `sdk/tools/pack/` with `uboot.boot`, `ota.bin`, and `cfg_tool.bin`.
4. **Licensing note** (relevant to #2 and the firmware base): Felucca is GPL-3.0-only and its BSP is MPL-2.0. M-VAVE's FM1 Open app source is GPL-3.0 with a linking exception for a proprietary binary SDK (`libfm1_engine.a`).

## Verification status summary

| Step / claim | Status |
|---|---|
| MCU is JieLi AC791N/WL82, 1 MiB flash, app at `0x4000`–`0x93000` | **Corroborated** (RE, FT, FEL-61, FEL) |
| USB-MIDI SysEx two-stage OTA is the normal install path | **Corroborated** (MV-OPEN, FEL, RE, GUIDE) |
| Official V15 image identity (SHA-256 `db1642b2…edb8a`) | **Corroborated** (MV-OPEN file + code, FEL, RE; hash re-checked) |
| Return to V15 over USB-MIDI while firmware or loader still answers | **Corroborated** (MV-OPEN workbench, FEL installer, RE hardware record, NEO report) |
| Re-running an interrupted OTA finishes it (unit stays in loader) | Plausible, author-tested (FEL, GUIDE); **not systematically tested** |
| ROM auto-enters UBOOT when the app does not boot | **Observed** on an FM-1 (FEL-61), consistent with JLU |
| Raw app-area restore via jl-uboot-tool from UBOOT | **One independent first-hand report** (FEL-61, repeated by the same user) |
| Forced UBOOT entry via RP2040 USB_KEY (FM-1-transporter) | Author-tested on one board (FT DEVLOG); **not independently reproduced**; same author as Felucca |
| Soft-key UBOOT entry from stock V15 | **Single source** (FT) |
| USB_KEY polarity on WL82 | **Conflicting** (JLU vs FT) |
| Power-loss or disconnect behaviour at each write stage | **Unverified** (RE P0 list still open) |
| Anything involving stock V16 (install over it, roll back from it, layout) | **Unverified** |
| Holding a button enters recovery | **No evidence**; PB01 is reset only (RE) |
| First-party JieLi documentation of AC79 UBOOT entry | **Not found** |

## Risky or open items to resolve before M3 hardware work

- Find out which stock version the target unit runs (V15 or V16), and whether V16 changes the head, layout, or protocol.
- Get the recovery path (Route B entry plus a full 1 MiB dump of the unit's own flash) working on the target unit **before** writing any non-stock image, as both RE and FT recommend.
- Our firmware should avoid the head (`0x0`–`0x4000`) and the device-data region, validate its own storage records with versioning and a CRC, and ignore foreign data instead of crashing on it (see FEL-61).
