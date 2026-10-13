# CLAUDE.md - LibreELEC x86 Dolby Vision (Generic x86_64 / CB1 Engine)

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.
Without my explicit request, always use Context7 whenever I need library or API documentation, generate code, or setup or configuration steps.
Do not run build or compile commands unless I explicitly ask you to do so.
Always explain the conclusion and approach before triggering any build ("每次代码改动完毕严禁直接触发编译，必须先给出结论与方案解释").
All code comments must be written in rigorous, professional English (no Chinese in comments).

> **Workspace Isolation Notice**:
> This workspace is dedicated to **LibreELEC x86-DV (Generic x86_64, Linux kernel, Intel i915 / AMD DRM KMS, CB1 0.2 Engine, Kodi 22 Piers / master)**.
> For CoreELEC 21 (Amlogic-ng / arm), see `../CoreELEC/CLAUDE.md`.
> For CoreELEC 22 (Amlogic-no / aarch64), see `../CoreELEC-22/CLAUDE.md`.

---

## Architecture & Technology Stack

- **Target Architecture**: `PROJECT=Generic ARCH=x86_64` (x86_64 userland, DRM/KMS atomic commits with explicit out-fences).
- **Dolby Vision Engine (CB1 0.2)**:
  - Custom zero-copy GLES3 / compute shader pipeline (`pack_gles300.frag`) packing Dolby Vision metadata and 12-bit YUV/RGB into HDMI RGB tunneling format.
  - Deep `libplacebo` integration with custom chroma upsampling (`cb1_fast_chroma`), tone mapping, and MMR / polynomial curve reshaping.
- **Dual-Layer Blu-ray Demuxer (`CBlurayDovi`)**:
  - In-memory Annex-B ST-DL reassembly uniting UHD Blu-ray dual-track TS PES streams (Base Layer 0x1011 + Enhancement Layer 0x1015) into composite HEVC packets for hardware decoding and CB1 processing.
- **Display Pacing & Presentation**:
  - DRM KMS windowing in `xbmc/windowing/gbm/WinSystemGbmGLESContext.cpp` with dynamic scanout retirement polling (`ScanoutPeriodTimeoutMs`) matching physical display refresh intervals (24Hz / 60Hz).

---

## Directory Structure & Staging Sync

| Local Path | Purpose |
|------------|---------|
| `CB1/` | Standalone CB1 engine source (`src/`), shaders, and libplacebo patches |
| `packages/mediacenter/kodi/cb1/` | Staged CB1 engine copy consumed during Kodi build |
| `projects/Generic/patches/kodi/` | Kodi Dolby Vision, Blu-ray demuxing, and windowing patches |
| `projects/Generic/patches/linux/` | Intel i915 / AMD GPU DRM and HDMI video driver patches |
| `projects/Generic/patches/ffmpeg/` | FFmpeg Dolby Vision RPU demuxing and MVC patches |
| `packages/addons/addon-depends/multimedia-tools-depends/libplacebo/` | Custom libplacebo packaging and patches |
| `config/cb1-source-lock.json` | SHA-256 integrity lockfile tracking all staged sources and patch files |
| `scripts/cb1-stage.py` | Source staging and verification script (`--check`) |

> **CRITICAL - Lockfile & Staging Invariant**:
> 1. Any modification to `CB1/src/` must be mirrored to `packages/mediacenter/kodi/cb1/src/`.
> 2. Any change to tracked patch files or CB1 sources MUST update `config/cb1-source-lock.json`.
> 3. Verify integrity before building: `python3 scripts/cb1-stage.py --check` (must pass with 0 errors).

---

## Build Commands

All build commands must run from the repository root:
```bash
cd /home/mephis/Project/libreelec-x86-DV
```

### 1. Verification & Sanity Check (No Compile)
```bash
python3 scripts/cb1-stage.py --check
```

### 2. Single Package Rebuild (Kodi)
```bash
# Clean Kodi build stamp
PROJECT=Generic ARCH=x86_64 scripts/clean kodi

# Build kodi target binary
CB1_HDR10_AI=yes PROJECT=Generic ARCH=x86_64 scripts/build kodi:target
```

### 3. Full Firmware Image Build
```bash
CB1_HDR10_AI=yes PROJECT=Generic ARCH=x86_64 scripts/image
```

> **BUILD DIRECTORY PROTECTION**:
> **NEVER** delete `build.LibreELEC-Generic.x86_64-13.0-devel/` entirely. It contains the cross-compilation toolchain and hundreds of dependencies. Clean only individual packages via `scripts/clean <package>`.

---

## Key Source Files by Component

| Component | Paths |
|-----------|-------|
| CB1 Reshaping & Validation | `CB1/src/dvbridge_placebo.c`<br>`CB1/src/dvbridge_fel.c`<br>`packages/mediacenter/kodi/cb1/src/dvbridge_placebo.c` |
| CB1 Color Rendering & Fast Chroma | `CB1/src/dvbridge_render.c`<br>`packages/mediacenter/kodi/cb1/src/dvbridge_render.c` |
| Blu-ray Dual-Layer Demuxing | `projects/Generic/patches/kodi/kodi-9999-bluray-dv.patch`<br>`xbmc/cores/VideoPlayer/DVDDemuxers/BlurayDovi.h`<br>`xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxFFmpeg.cpp` |
| Kodi Output Settings & Defaults | `projects/Generic/patches/kodi/kodi-9999-zz-beta3.patch`<br>`system/settings/settings.xml`<br>`xbmc/cores/VideoPlayer/DVDCodecs/Video/DVOutputSettings.h` |
| DRM Atomic Windowing & VSync | `xbmc/windowing/gbm/WinSystemGbmGLESContext.cpp` |
| Intel i915 Native DV Tunneling | `projects/Generic/patches/linux/linux-9900-*.patch` |

---

## Test Device & Hot-Swap Deployment

- **Test Device**: Intel N100 / x86-64 Box (`192.168.50.9`, SSH credentials: `root` / `libreelec`)
- **Binary Location**:
  `build.LibreELEC-Generic.x86_64-13.0-devel/build/kodi-*/.x86_64-libreelec-linux-gnu/kodi.bin`

### Hot-Swap kodi.bin Workflow
```bash
# 1. Upload updated binary to writable storage on target
sshpass -p libreelec scp build.LibreELEC-Generic.x86_64-13.0-devel/build/kodi-*/.x86_64-libreelec-linux-gnu/kodi.bin root@192.168.50.9:/storage/kodi.bin

# 2. Stop kodi, bind-mount over read-only rootfs, and restart
sshpass -p libreelec ssh root@192.168.50.9 "systemctl stop kodi && (mountpoint -q /usr/lib/kodi/kodi.bin && umount /usr/lib/kodi/kodi.bin || true) && chmod 755 /storage/kodi.bin && mount --bind /storage/kodi.bin /usr/lib/kodi/kodi.bin && systemctl start kodi"

# 3. Restore stock binary:
# sshpass -p libreelec ssh root@192.168.50.9 "systemctl stop kodi && umount /usr/lib/kodi/kodi.bin && systemctl start kodi"
```

### Live Diagnostics on Target
```bash
# Check Kodi logs in real time
sshpass -p libreelec ssh root@192.168.50.9 "tail -f /storage/.kodi/temp/kodi.log"

# Run DV diagnostic tool
sshpass -p libreelec ssh root@192.168.50.9 "dv-diagnostics"
```
