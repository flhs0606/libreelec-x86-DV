# LibreELEC x86-DV 蓝光双轨杜比视界 (DT-DL FEL) 排查与交接手册

本文档为 **LibreELEC x86-DV (Generic x86_64, Intel N100)** 针对双轨杜比视界（Profile 7 DT-DL FEL）原盘播放故障的完整技术交接文档，涵盖系统架构、联调环境、排查过程、日志查询方法与后续攻坚方向，供接手此任务的工程师或 Agent 查阅。

---

## 1. 核心环境与连接基础

### 1.1 开发与构建环境 (WSL2)
- **操作系统**：WSL2 (Ubuntu 24.04 LTS, x86_64)
- **工作区路径**：`/home/mephis/Project/libreelec-x86-DV`
- **Git 仓库信息**：
  - `origin`: `https://github.com/flhs0606/libreelec-x86-DV.git`
  - `upstream`: `https://github.com/CroqueMr/libreelec-x86-DV.git`
  - `branch`: `main`
- **源码锁机制**：
  - 本项目通过 `CB1/` 维护杜比视界桥接层原生 C 代码，并通过 `scripts/cb1-stage.py` 同步到 `packages/mediacenter/kodi/cb1/`。
  - 每次修改 `CB1/` 或 patch 文件后，必须执行校验：
    ```bash
    python3 scripts/cb1-stage.py --check
    ```
    校验和记录在 `config/cb1-source-lock.json` 中。
- **本地 SMB 挂载**：
  - 服务器：`192.168.50.254` (fnOS)
  - 本地挂载路径：`/mnt/smb/clouddrive`
  - 测试片源路径：`/mnt/smb/clouddrive/media/临时/BUG/海洋奇缘2.Moana 2 (2024) 4k.HDR10.Atmos 7.1-sGnb@CHDBits.iso`

### 1.2 实机测试环境 (Intel N100)
- **硬件**：Intel N100 (Alder Lake-N, Gen12 Intel UHD Graphics, 24 EU)
- **操作系统**：LibreELEC 13.0-devel (Linux Kernel 6.6.x)
- **网络 IP**：`192.168.50.9`
- **SSH 连接信息**：
  - 用户名：`root`
  - 密码：`libreelec`
  - 免密命令行（依赖 `sshpass`）：
    ```bash
    sshpass -p libreelec ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@192.168.50.9 "<COMMAND>"
    ```
- **Kodi 运行架构与热替换挂载点**：
  - 系统内置路径：`/usr/lib/kodi/kodi.bin` (由于底层 squashfs 为只读系统，实机通过 bind mount 机制覆盖：`/dev/sda2 on /usr/lib/kodi/kodi.bin type ext4 (rw,noatime)`)
  - 实际读写持久化路径：`/storage/kodi.bin`
  - Kodi 运行日志：`/storage/.kodi/temp/kodi.log`
- **Kodi JSON-RPC 播控端口**：
  - HTTP 端点：`http://192.168.50.9:8080/jsonrpc` (无需认证)
  - 连通性测试命令：
    ```bash
    curl -s -X POST -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","method":"JSONRPC.Ping","id":1}' http://192.168.50.9:8080/jsonrpc
    # 期望返回: {"id":1,"jsonrpc":"2.0","result":"pong"}
    ```

---

## 2. 编译、打包与热替换工作流

在开发过程中严禁执行全量打包（耗时极长），仅需增量编译 `kodi.bin` 并进行秒级网络热替换：

### 2.1 增量编译命令
```bash
# 必须进入目标平台构建目录
cd /home/mephis/Project/libreelec-x86-DV/build.LibreELEC-Generic.x86_64-13.0-devel/build/kodi-28ea2eac1eb7af8fdcbd2672d933ce87f594be79/.x86_64-libreelec-linux-gnu

# 触发多核增量编译 (约 15~30 秒)
ninja -j$(nproc) kodi.bin

# 检查生成的二进制
ls -lh kodi.bin && md5sum kodi.bin
```

### 2.2 远程实机热替换一键脚本
```bash
# 在 WSL2 构建目录下执行：
sshpass -p libreelec ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@192.168.50.9 "systemctl stop kodi" && \
sshpass -p libreelec scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null kodi.bin root@192.168.50.9:/storage/kodi.bin && \
sshpass -p libreelec ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@192.168.50.9 "chmod +x /storage/kodi.bin; umount /usr/lib/kodi/kodi.bin 2>/dev/null; mount -o bind /storage/kodi.bin /usr/lib/kodi/kodi.bin; md5sum /usr/lib/kodi/kodi.bin; systemctl start kodi"
```

---

## 3. 实机日志查询与实时监控方法

在测试机播放故障片源时，应通过远程 SSH 捕获各管道层级的时序行为：

### 3.1 实时滚动日志监控 (Live Tail)
```bash
sshpass -p libreelec ssh -o StrictHostKeyChecking=no root@192.168.50.9 "tail -F /storage/.kodi/temp/kodi.log"
```

### 3.2 关键时序与指标过滤查询
```bash
# 1. 检查杜比视界信号协商与渲染器统计 (direct vs composed, fel 状态)
sshpass -p libreelec ssh -o StrictHostKeyChecking=no root@192.168.50.9 \
  "grep -E 'DVBridge|CB1 FEL|signal committed|renderer reset' /storage/.kodi/temp/kodi.log | tail -n 40"

# 2. 检查播放健康度与队列水位 (stalled, video_queue_pct, buffering)
sshpass -p libreelec ssh -o StrictHostKeyChecking=no root@192.168.50.9 \
  "grep -E 'playback health|SetCaching|stream stalled|timeout waiting for buffer' /storage/.kodi/temp/kodi.log | tail -n 40"

# 3. 检查 HEVC 解码器切片与参考帧状态 (POC, NALU, RPS)
sshpass -p libreelec ssh -o StrictHostKeyChecking=no root@192.168.50.9 \
  "grep -E 'Could not find ref|First slice|Multiple Dolby Vision|Skipping invalid' /storage/.kodi/temp/kodi.log | tail -n 40"

# 4. 检查解复用器双轨吞吐与配对日志
sshpass -p libreelec ssh -o StrictHostKeyChecking=no root@192.168.50.9 \
  "grep -E 'joined Profile 7|DV Blu-ray' /storage/.kodi/temp/kodi.log | tail -n 40"
```

---

## 4. 关键架构与核心源码布局

LibreELEC x86-DV 的杜比视界双轨处理链路跨越了 4 个层次：

```
[MPEG-TS demuxer] (CDVDDemuxFFmpeg)
         │  (BL Packet 0x1011 + EL Packet 0x1015)
         ▼
[双轨解复用合并] (CBlurayDovi, BlurayDovi.h)
         │  (组合包：BL 4K + RPU NAL 62 + 00 00 00 01 7E 01 分界符 + 原生 EL 1080p 切片)
         ▼
[主解码器与截断] (CDVDVideoCodecFFmpeg)
   ┌─────┴─────────────────────────────────┐
   │ 截断分界符之前 (纯 BL 4K + RPU)        │ 提取完整包送入次级解码器
   ▼                                       ▼
[VAAPI 主解码器] (ff-hevc-vaapi)    [次级桥接层] (CB1/src/dvbridge_fel.c)
   │ (解出 4K 主层表面)                     │ (跳过 dovi_split, VAAPI 解出 1080p 增强层表面)
   │                                       ▼
   │                                [dvbridge_fel_take()] (按 PTS 严格对齐配对)
   └───────────────────┬───────────────────┘
                       ▼
          [VideoPicture::doviEnhancement] (持有次级 VAAPI 表面)
                       ▼
           [CDVBridgeGLES / dvbridge_fel_vpp] (DVBridgeGLES.cpp)
                       │ (VAAPI VPP 硬件双表面 1080p -> 4K 零拷贝缩放)
                       ▼
            [Dolby Vision Packed RGB Direct Scanout] (全屏零拷贝直接扫出至 DRM KMS)
```

### 核心源文件位置速查：
1. **解复用双轨合并**：
   - 本地构建源码：`build.LibreELEC-Generic.x86_64-13.0-devel/build/kodi-28ea2eac1eb7af8fdcbd2672d933ce87f594be79/xbmc/cores/VideoPlayer/DVDDemuxers/BlurayDovi.h`
   - Git 补丁追踪：`projects/Generic/patches/kodi/kodi-9999-bluray-dv.patch`
2. **解复用调用点**：
   - 本地构建源码：`.../xbmc/cores/VideoPlayer/DVDDemuxers/DVDDemuxFFmpeg.cpp`
   - Git 补丁追踪：`projects/Generic/patches/kodi/kodi-9999-bluray-dv.patch`
3. **主解码器截断与配对调用**：
   - 本地构建源码：`.../xbmc/cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecFFmpeg.cpp`
   - Git 补丁追踪：`projects/Generic/patches/kodi/kodi-9993-fel-presentation-pairing.patch`
4. **次级 VAAPI 解码桥接层**：
   - 核心源文件：`CB1/src/dvbridge_fel.c`
   - 暂存发布目录：`packages/mediacenter/kodi/cb1/src/dvbridge_fel.c`
5. **渲染与 VPP 硬件缩放层**：
   - 本地构建源码：`.../xbmc/cores/VideoPlayer/VideoRenderers/DVBridgeGLES.cpp`
   - 缩放源码：`CB1/src/dvbridge_fel_vpp.c`

---

## 5. 故障排查全过程与已攻克里程碑

### 5.1 历史缺陷与已验证解决项（切勿回滚！）

| 故障现象 | 原始机理 | 解决方案与当前状态 |
|---|---|---|
| `Multiple Dolby Vision RPUs found in one AU` | EL 帧由多个 TS 切片包构成，AU 内未做 RPU 幂等检查，导致后序分片把重复的 NAL 62 追加到尾部。 | 在 `BlurayDovi.h::Push()` 遍历 NAL 时加 `if (unit.rpu.empty())` 门限；组装前释放 side data。**（已彻底根治，0 次）** |
| `First slice in a frame missing` | `Take()` 中的 1 帧前瞻门限被误改为了 `if (m_base.empty())`，4K 主层后序切片未读完就被早产截断发射。 | 恢复前瞻等待门禁；并在 `Push` 中严禁将 B 帧粘包。**（已彻底根治，0 次）** |
| `Could not find ref with POC 15` | 原盘起播或切段交接处，解复用器吐出的首个包是前导无参考帧的残留 B 帧（POC 15）。 | 在 `BlurayDovi.h` 增加 `HasHevcKeyframeNal` 门禁，未同步（`!m_seenBaseKeyframe`）前丢弃非关键帧，强制从合法 IDR (POC 0) 开解。**（已彻底根治，0 次）** |
| 次级 EL 解码器参考帧破坏 | `AssembleComposite` 对每个 EL NAL 套用 `{0x7e, 1}` 隧道，经过 FFmpeg `dovi_split` 滤镜时误剥 `0x03` 防竞争字节，毁坏 EL 切片语法。 | `BlurayDovi.h` 保持原生 Annex-B EL 字节流，仅加一次 6 字节分界符；`dvbridge_fel.c` 识别后直接绕过 `dovi_split` 直送解码。**（已彻底根治，EL 硬件解码成功）** |
| 次级 FEL 画面无法缩放合成 | 1080p EL 表面直接送显会导致与 4K BL 尺寸不匹配。 | 实机日志已证实 `CB1 FEL upscaling: VAAPI 1920x1080 to 3840x2160, two leased surfaces` 成功介入。 |

---

## 6. 当前残留病灶深入分析（新 Agent 攻坚重点）

### 6.1 当前最新日志切片 (11:35 实测)
```log
2026-10-17 11:35:07.080 T:860106 debug <general>: CVideoPlayer::SetCaching - caching state 0
2026-10-17 11:35:07.327 T:858988  info <general>: CB1 FEL upscaling: VAAPI 1920x1080 to 3840x2160, two leased surfaces, zero CPU pixel copies
2026-10-17 11:35:07.330 T:858988  info <general>: DVBridge first frame: source=3840x2160 output=3840x2160 offset=0,0 enhancement=true fel=true cm4=false direct=true
2026-10-17 11:35:07.832 T:860143 warning <general>: OutputPicture - timeout waiting for buffer
2026-10-17 11:35:08.188 T:858988  info <general>: DVBridge signal committed: connector=402 crtc=151 dv=1 max_bpc=8 rgb=1 mode=3840x2160 clock_khz=296703
2026-10-17 11:35:08.200 T:860143  info <general>: DVBridge playback health: video_queue_pct=92 video_queue_s=3.670
... 经过 10 秒 ...
2026-10-17 11:35:18.449 T:858988  info <general>: DVBridge renderer reset: prepared=4 direct=4 presented=4
```

### 6.2 现象与因果剖析
1. **画面以 0.4 fps 龟速极度卡顿**：
   - 10 秒时间内，渲染器仅成功 `prepared=4 direct=4 presented=4`（只播出了 4 帧！正常 24fps 应当播出 240 帧）。
2. **解复用队列非常健康，但渲染器缓冲区死等**：
   - `video_queue_pct = 92%`（视频队列里积蓄了 3.67 秒的视频包，解复用供包完全充足）；
   - 瓶颈出现在视频渲染输出端：`OutputPicture - timeout waiting for buffer` 持续发生；
   - 证明 `m_renderManager.WaitForBuffer()` 拿不到空闲的渲染表面，解码器无法把新解出的帧提交给渲染器。
3. **关键怀疑点**：
   - **点 A（VAAPI Surface 句柄未及时释放或泄露）**：
     在 `CDVDVideoCodecFFmpeg::GetPicture()` 或 `DVBridgeGLES.cpp` 中，`base->doviEnhancement` 持有的次级 VAAPI Surface，或者 `dvbridge_fel_vpp` 租赁的两个表面（`two leased surfaces`），在送入 Direct Scanout 之后，**是否被 DRM KMS 页面翻转（PageFlip）死锁霸占而未归还**？
   - **点 B（HDMI 24Hz Modeset 阻塞）**：
     在提交 `dv=1 rgb=1 24Hz` 信号时，底层调用了 `eglClientWaitSyncKHR`，日志报：
     ```log
     Error: EGL_BAD_PARAMETER Command: eglClientWaitSyncKHR Type: EGL_DEBUG_MSG_ERROR_KHR Message: _eglClientWaitSyncCommon
     ```
     Kodi GLES 上下文在等待栅栏（Fence）同步时超时，导致渲染主线程阻塞，每个缓冲区需要等满超时时间才被迫丢弃或推进。
   - **点 C（`m_dvPending` 等待条件或时间戳微秒偏差）**：
     检查 `base->doviSourcePts` 与 `enhancement->pts` 在进入渲染器后的时钟对齐逻辑，是否有因为某种 PTS 容差不匹配导致的等待循环。

---

## 7. 下一步排查与调试指引

接手此任务时，建议按以下步骤聚焦：
1. **排查 `OutputPicture - timeout waiting for buffer`**：
   - 查看 `xbmc/cores/VideoPlayer/VideoRenderers/RenderManager.cpp` 的 `WaitForBuffer`；
   - 观察是否是因为 `m_renderBuffers` 池被耗尽，检查 `CVideoBuffer` 的引用计数（Release 链条）。
2. **排查 `dvbridge_fel_vpp` 表面归还时机**：
   - 检查 `DVBridgeGLES.cpp` 第 1280~1325 行中，`imported->scaled` 与 `imported->vpp` 是在何时调用 `dvbridge_fel_vpp_release` 的。如果未在帧提交显示完成后释放，VPP 的 2 个租赁表面会被瞬间耗尽，导致后续帧无法缩放而阻塞。
3. **禁用 FEL VPP 对比测试**：
   - 可在设置或代码中将 `m_felVppEnabled` 临时置为 `false`（走纯 Shader 缩放分支），观察帧率是否能恢复到 24fps。如果走 Shader 恢复 24fps，则问题 100% 锁死在 `dvbridge_fel_vpp` 的表面归还与 VAAPI 释放泄露上。

---

## 8. 实机验证严酷现实与结论修正 (2026-10-17 12:55)

### 8.1 严酷事实与认知纠偏（切勿轻信假性指标！）
在 12:52 实测中，虽然 Kodi JSON-RPC 报告播放时间在推进、健康日志输出 `output_attempts=241`，但日志明确标注 **`attempts are not display flips`**。
**电视端实际肉眼可见的残酷现实**：
1. **周期性冻结与重缓冲**：每隔 10~15 秒，画面就会发生卡死，并伴随弹出长达 3~4 秒的 `caching state 1` 缓冲转圈！
2. **底层日志真相（12:52 实录）**：
   ```log
   2026-10-17 12:52:21.822 T:912887 warning <general>: OutputPicture - timeout waiting for buffer
   2026-10-17 12:52:22.408 T:912887 warning <general>: OutputPicture - timeout waiting for buffer
   2026-10-17 12:52:22.949 T:912888    info <general>: CVideoPlayerAudio::Process - stream stalled
   2026-10-17 12:52:23.012 T:912851   debug <general>: CVideoPlayer::SetCaching - caching state 1
   2026-10-17 12:52:26.357 T:898702   debug <general>: CVideoPlayer::SetCaching - caching state 0
   2026-10-17 12:52:27 ~ 12:52:33: 连续 7 次爆发 OutputPicture - timeout waiting for buffer
   2026-10-17 12:52:37.594 T:912888    info <general>: CVideoPlayerAudio::Process - stream stalled
   2026-10-17 12:52:38.076 T:912851   debug <general>: CVideoPlayer::SetCaching - caching state 1
   ```
3. **因果链确立**：
   - 渲染缓冲池耗尽（`OutputPicture - timeout waiting for buffer`） $\rightarrow$ 视频渲染线程卡死 $\rightarrow$ 驱动时钟停摆 $\rightarrow$ 音频队列断流（`stream stalled`） $\rightarrow$ 播放器强行触发 `SetCaching state 1`（重缓冲 3~4 秒）。

### 8.2 新 Agent 攻坚第一优先级路线
1. **排查 `CRenderManager::WaitForBuffer()` 为何拿不到空闲缓冲表面**：
   - 查看 `RenderManager.cpp` 中 `m_free.empty()` 的原因：`m_pRenderer->NeedBuffer(*it)` 是否因某种 EGL 同步或表面持有未返回 `false`？
   - 检查 `DVBridgeGLES.cpp` 中 Direct Scanout 页面翻转后，Kodi 渲染缓冲区的归还生命周期。
2. **测试禁用 FEL VPP 缩放对比**：
   - 将 `m_felVppEnabled` 置为 `false`，让 FEL 走 Shader 缩放通路，观察 `OutputPicture - timeout waiting for buffer` 是否彻底消失。若是，则 100% 证明是 VPP 与 Direct Scanout 之间的某种表面泄露。
