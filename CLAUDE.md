# CLAUDE.md - LibreELEC x86 Dolby Vision (Generic x86_64 / CB1 Engine)

Guidance for Claude Code when working in this repository.
- Always use Context7 for library/API documentation, code generation, or setup/configuration steps without explicit request.
- Whenever I mention "检查log", "查log", "查下log", "看下日志", "看log", "还是卡", "依旧卡顿", "执行播放测试", or ask to diagnose playback issues, ALWAYS invoke the `kodi-diagnose` skill first.
- Do not run build or compile commands unless explicitly requested (except within an explicitly authorized "播放测试循环").
- Explain the conclusion and proposed approach before triggering any build.
- All code comments must be written in rigorous, professional English (no Chinese in comments).

> **Workspace Isolation**:
> Dedicated to **LibreELEC x86-DV (Generic x86_64, Linux kernel, Intel i915 / AMD DRM KMS, CB1 0.2 Engine, Kodi 22 Piers / master)**.
> For CoreELEC 21 (Amlogic-ng / arm), see `../CoreELEC/CLAUDE.md`.
> For CoreELEC 22 (Amlogic-no / aarch64), see `../CoreELEC-22/CLAUDE.md`.

## Repositories & Remotes

- `origin`: `https://github.com/flhs0606/libreelec-x86-DV.git` (user fork)
- `upstream`: `https://github.com/CroqueMr/libreelec-x86-DV.git` (upstream repository)

## Architecture Overview

- **Target**: `PROJECT=Generic ARCH=x86_64` (DRM/KMS atomic commits with out-fences).
- **Dolby Vision Engine (CB1 0.2)**: GLES3/compute pipeline (`CB1/src/pack_gles300.frag`) packing DV metadata + 12-bit YUV/RGB into HDMI RGB tunneling format. Uses `libplacebo` with custom chroma upsampling (`cb1_fast_chroma`) and polynomial curve reshaping.
- **Dual-Layer Blu-ray Demuxer (`CBlurayDovi`)**: Reassembles UHD Blu-ray dual-track TS PES (BL 0x1011 + EL 0x1015) into composite HEVC packets in `projects/Generic/patches/kodi/kodi-9999-bluray-dv.patch`.
- **Cloud Blu-ray & Display Pacing**: HTTP ISO direct play and dynamic scanout polling (`ScanoutPeriodTimeoutMs`) matching physical refresh rates in `projects/Generic/patches/kodi/kodi-9999-zzz-cloud-bluray-direct-play.patch`.

## Key Paths & Staging Invariant

| Path | Description |
|------|-------------|
| `CB1/` | CB1 engine source (`src/`), shaders, tests, and configuration |
| `packages/mediacenter/kodi/cb1/` | Gitignored staging directory consumed during Kodi build |
| `projects/Generic/patches/kodi/` | Kodi patches (Blu-ray DV, cloud direct play, L5 overlay, Beta3) |
| `projects/Generic/patches/linux/` | Intel i915 / AMD DRM KMS and HDMI video driver patches |
| `config/cb1-source-lock.json` | SHA-256 integrity lockfile tracking all staged sources and patch files |
| `scripts/cb1-stage.py` | Staging script and integrity validator (`--check`) |

> **Lockfile & Staging Rules**:
> 1. Kodi source is not in git root; changes are maintained as patches under `projects/Generic/patches/kodi/`.
> 2. `CB1/` is authoritative. Changes to `CB1/src/` must mirror to `packages/mediacenter/kodi/cb1/src/`.
> 3. Any change to tracked patches or CB1 sources MUST update `config/cb1-source-lock.json`.
> 4. Always verify integrity before builds: `python3 scripts/cb1-stage.py --check` (must pass with 0 errors).

## Build Commands

Run from repository root (`/home/mephis/Project/libreelec-x86-DV`):

```bash
# 1. Integrity check (fast, no compilation)
python3 scripts/cb1-stage.py --check
python3 -m unittest discover -s tests/cb1 -p 'test_*.py'

# 2. Rebuild Kodi single package
PROJECT=Generic ARCH=x86_64 scripts/clean kodi
CB1_HDR10_AI=yes PROJECT=Generic ARCH=x86_64 scripts/build kodi:target

# 3. Full firmware release build
CB1_HDR10_AI=yes PROJECT=Generic ARCH=x86_64 scripts/image
```

> **Build Directory Protection**: Never delete `build.LibreELEC-Generic.x86_64-13.0-devel/` entirely. Clean specific packages via `scripts/clean <package>`.

## Test Device & Hot-Swap Deployment

- **Device**: Intel N100 Box (`192.168.50.9`, SSH: `root` / `libreelec`)
- **Binary Path**: `build.LibreELEC-Generic.x86_64-13.0-devel/build/kodi-*/.x86_64-libreelec-linux-gnu/kodi.bin`

### Hot-Swap kodi.bin
```bash
# Deploy and bind-mount over read-only rootfs
sshpass -p libreelec scp build.LibreELEC-Generic.x86_64-13.0-devel/build/kodi-*/.x86_64-libreelec-linux-gnu/kodi.bin root@192.168.50.9:/storage/kodi.bin
sshpass -p libreelec ssh root@192.168.50.9 "systemctl stop kodi && (mountpoint -q /usr/lib/kodi/kodi.bin && umount /usr/lib/kodi/kodi.bin || true) && chmod 755 /storage/kodi.bin && mount --bind /storage/kodi.bin /usr/lib/kodi/kodi.bin && systemctl start kodi"

# Restore stock binary
sshpass -p libreelec ssh root@192.168.50.9 "systemctl stop kodi && umount /usr/lib/kodi/kodi.bin && systemctl start kodi"
```

### Remote Diagnostics
```bash
# Live Kodi log stream
sshpass -p libreelec ssh root@192.168.50.9 "tail -f /storage/.kodi/temp/kodi.log"

# Dolby Vision runtime diagnostics
sshpass -p libreelec ssh root@192.168.50.9 "dv-diagnostics"
```
