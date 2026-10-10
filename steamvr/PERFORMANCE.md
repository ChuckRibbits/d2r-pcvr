# October 10 CPU performance comparison

## Afternoon retest and Windows handoff

The owner returned and tested the restored baseline: **"Still stalls and loses
image quality"**. After a full PC reboot, fresh SteamVR and Frame connection,
the same baseline still failed: **"Still happening"**. The fresh driver log
recorded **119 stream reset requests before D2R launched**, from SteamVR startup
at 15:40:34 to the launch at 15:41:22 PDT. At least one failure exists outside
the mod; separate native game-update stalls remain unresolved.

| Restored-baseline capture | Pairs | Mean ms | p95 ms | Max ms |
| --- | ---: | ---: | ---: | ---: |
| Before reboot / 10201 | 360 | 23.759 | 31.631 | 37.521 |
| After reboot / 10202 | 360 | 22.932 | 30.004 | 35.161 |

Views/routes differ, and these short windows miss larger intermittent stalls
visible elsewhere in the logs. They do not demonstrate a reboot performance
gain or refute the owner's feedback. Graphics/DLL settings were unchanged.

A manual 50 Mbps streaming test was started but **not evaluated**: the owner
chose a clean Windows installation before providing a comparison. SteamVR Link
was restored to automatic bandwidth, target 200 Mbps, with OpenVR readback
verified. No conclusion can be drawn about the 50 Mbps setting.

Current next step is [WINDOWS_HANDOFF.md](../WINDOWS_HANDOFF.md), baseline first;
native Windows is untested. Portable numeric evidence, DLL hashes, validation
and limitations are in [evidence/2026-10-10-handoff.json](evidence/2026-10-10-handoff.json).
Raw logs are retained locally, excluded from GitHub. The handoff release keeps
the baseline distinct from the current-source experimental performance DLL.

## Current disposition: rollback after severe stalls

After the captures below, the owner reported random stalls during movement,
combat and menus, stuck/catching-up frames and severe temporary image-quality
loss. He requested further investigation without headset use and went to bed.
The game was closed; the pre-performance DLL `16e24f55…` was restored with
`defer_gpu_wait=0`, `model_radius=300` and all profiling off. Other settings,
game graphics, saves and SteamVR/network configuration were preserved. No game
launch or headset test was performed during this follow-up. The rollback is a
comparison baseline, not a demonstrated cure. Both candidates and all source
changes are preserved; `build-frame/d2rl-vrcam.dll` now contains the later,
**staged-only** `bc51fd37…` candidate, not the installed DLL.

Records: `build-frame/latest-perf-regression.json` and
`build-frame/latest-perf-offline-stage.json`. Original logs, ten Link histories,
configuration and replaced DLL are preserved under
`build-frame/backups/perf-regression-20261010-023124`.

### Streaming was also failing

Saved SteamVR Link histories contain 10,711 unique driver rows from
02:20:54.816 to 02:24:53.801 PDT, after merging 18,993 overlapping input rows.
The Link log records **147 video stream reset requests** in that interval,
including bursts of retransmission requests for messages no longer retained.

| Driver stage | Median ms | p95 ms | Maximum ms |
| --- | ---: | ---: | ---: |
| Gate release → app submit | 17.799 | 23.226 | 126.808 |
| Encode start → end | 7.576 | 8.866 | 12.371 |
| Send start → end | 0.528 | 0.795 | 1.564 |
| Send start → reported receive | 7.772 | 80.810 | 734.133 |
| Receive → decode start | 0.059 | 0.189 | 146.378 |
| Decode start → end | 5.560 | 7.129 | 26.534 |

These stages have different valid sample counts. Missing decode/receive fields
are unknown, not zero work; some frames never acquired those timestamps. The
driver clocks wrapped during this interval and must be handled as 32-bit
microseconds. Do not subtract remote-clock timestamps from host timestamps.
The displayed receive values are the driver's already-aligned fields, not a
direct measurement of pure radio latency.

For example, the frame released at 02:23:54.746 had gate→app submission of
16.401 ms and encode time of 7.493 ms, but send→reported receive of 674.954 ms.
At 02:23:55.546, the same measurements were 16.240, 6.319 and 734.133 ms;
that row had no decode completion. Game rendering alone does not explain those
delivery delays. The evidence implicates transport/receiver handling, but
does not distinguish wireless problems, driver queuing/retransmission or client
handling. Actual bitrate/resolution is absent, so the reported picture collapse
cannot be quantified from these logs.

Reset requests also occurred at 01:52–01:54 before installing the first CPU
candidate, and before enabling deferred wait at 02:01:10. Thus the streaming
problem cannot be attributed solely to the new DLL or wait setting. The last
session's compositor summary records 14,376 presents and 733 drops, but includes
startup/menus and uses different accounting from stereo-pair profiles; it is
not a like-for-like FPS benchmark against the earlier sessions.

Current adapter inspection found Wi-Fi power saving already off. Kernel logs
covering 01:45–02:26 showed no GPU error or USB reset; their connection-time WMM
parameter warnings do not establish a stall cause. No network tuning or global
driver changes were made. D2R settings remain resolution scale 100, FXAA,
DLSS off and dynamic resolution off. The owner briefly tried room rings 4 and 1
in the VR settings during the last session and returned to 2; final INI changes
from baseline were only wait/radius and disabled hook profiling. The later
menu/settings intervals should not be treated as identical gameplay scenes.

`steamvr/stream_report.py` reproduces these results offline. It merges repeated
histories, retains repeated present IDs at distinct times, prefers completed
records, excludes absent/invalid timings, handles timestamp rollover and scopes
reset counts to the selected interval. Five tests pass; saved full report is
`build-frame/perf-stream-report.json`. These auto-dumps are **fault-triggered
histories**, not an unbiased whole-session FPS or packet-loss survey.

### Additional code finding, staged only

Every INI reload previously marked render distance dirty, even when changing
only a profiler switch, foveation option or unrelated control. Applying model
radius forces the game's visibility sort every tick for 500 ms. The new guard
marks it dirty only for actual ring/radius changes, retaining initial and failed
application retries. This also removes a confound at the start/end of future
profiling captures. It does not fix the independent Link delivery failures or
establish a cure for large native simulation ticks. The candidate cross-build
and 14 report/launcher tests pass; this guard is not runtime/headset verified.

Next session should first establish the restored baseline's delivery behavior,
collecting app timings and Link telemetry together. If resets persist, compare
a fresh streaming session before adding DLL optimizations; then isolate stream
quality/bandwidth if needed. Avoid changing rendering, wait mode, radio settings
and refresh rate together. No further headset testing requested tonight.

## Earlier comparison and its limitations

Linux, Proton Experimental, SteamVR, Steam Frame at 90 Hz; offline Nekro in
town. Owner confirmed the installed candidate's image looked normal, then
reported definite hitches while running. No game graphics settings or
saves were modified. Resolution stayed 100%, shadows/AO 4, DLSS off, fixed and
eye-tracked foveation on, fog end 200.

Each row contains 360 stereo pairs with `profile_gpu=0`. `profile_hooks=0`
except the attribution row. Total includes game update, eye rendering and XR
pacing/waiting; reciprocal throughput is not a compositor drop/reprojection
measurement. GPU profiling was deliberately disabled because it forces the
completion fence being compared.

| Capture / sequence | Wait deferred | Model radius | Mean pair ms | Eye passes ms | Pair p95 ms |
| --- | --- | --- | ---: | ---: | ---: |
| Previous DLL / 10101 | No | 300 | 31.38 | 18.03 | 35.40 |
| Candidate / 10102 | No | 300 | 31.69 | 17.57 | 34.96 |
| Candidate + thread counters / 10103 | No | 300 | 31.72 | 17.42 | 35.28 |
| Deferred wait / 10104 | Yes | 300 | 24.60 | 17.74 | 32.31 |
| Shorter radius / 10105 | Yes | 210 | 22.70 | 15.33 | 27.41 |
| Radius repeat / 10106 | Yes | 300 | 22.53 | 15.38 | 26.41 |
| Radius repeat / 10107 | Yes | 210 | 22.46 | 14.75 | 25.87 |

The code-only comparison did **not** demonstrate increased frame rate. It also
was not an exact scene match: baseline hero position was (8791.1,11261.1),
candidate (8794.6,11255.8), and head direction changed. Launcher polling changes
were already used by both runs, so this does not measure that improvement.

Deferred waiting produced the clearest mean-cycle reduction in this session,
but was subsequently rolled back after the owner's severe-stall report. Candidate and
deferred-wait views were close (125–127 and 128 degrees). The explicit fence
fell from 4.29 ms to zero; `xrEndFrame` grew from 0.68 to 2.68 ms and
`xrWaitFrame` fell from 7.19 to 2.00 ms. This is improved overall pacing, not
7 ms of eliminated rendering work. Image copies still wait before allocator
reuse and shutdown.

The first radius comparison kept nearly the same view (128–129 degrees) and
reduced eye-pass time by 2.40 ms. During the repeats head direction changed to
154–163, then 118–155 degrees. Those repeats cannot establish a clean percentage
gain from radius alone. Fog coverage does not prove all distant models/shadows
are invisible; walk/turn and check silhouettes before calling this verified.

## What the worker counters found

Sequence 10103 covered 18 threads including input. Roughly 19,291 pipeline
binds/pair reached 17 render threads, but only 277 VRS pipeline-map lookups/pair
remained after the existing same-pipeline fast path and new generation cache.
It counted 1,368 guarded memory reads/pair. Only two memory writes/pair occurred
in this scene (~23.8 KB); none of those worker buckets reached a timing sample.
The reporter displays missing samples as `n/a`, never as evidence of free work.

The input tick's sampled inclusive wall time was 9.81 ms/pair, including waiting
and work. The transport lock still spans blocking XR/fence calls. This is a
useful follow-up target, but the number is not exclusive CPU time or a measured
saving available from lock removal. Worker/hook scopes overlap and nest; do
not sum their estimates into a frame cost.

## Candidate changes and validation (now rolled back)

- Generation-aware thread-local pipeline caches and native SRW callback locks.
- Guarded local copies instead of writes routed through wineserver; no page
  permission changes. Invalid, freed, read-only and crossing-page buffers fail
  without entering application exception handlers in the Proton test.
- Bounded per-instance portal classification reused within particle batches,
  retaining mixed portal/nonportal lanes and the verified return-portal math.
- Sparse routine CPU-time sampling; idle mouse cursor queries skipped.
- Optional UI tracing decided at startup, with no live callback-list mutation.
- Full launcher process scans every five seconds during play; known-game PID
  identity/exit checks still run every half second.
- Bounded per-thread hook/memory diagnostics with backward-compatible reports.

The DLL cross-build, host/Proton counter/cache/lock tests, Proton memory tests,
portal tests, five launcher tests and four report tests passed. An isolated
64-byte copy microbenchmark improved from 15.2 to 0.14 microseconds on one
thread. This does not imply an equivalent game frame-time saving.

Candidate SHA256:
`a3bc1aeacb1859144d160c3169347b4f8c6286234b518b6bc08885c7cfdd414e`.
Previous DLL:
`16e24f5564a822531682629a5753c0b5ef248ef48dc33f0fbe358e1e30ed15fc`.
Local ignored evidence: `build-frame/perf-*.log/.json/.csv`,
`build-frame/perf-comparison.json`, and
`build-frame/latest-perf-headless-deployment.json`. The deployment's ready and
backup directories retain both DLLs and settings. Replace DLLs only after
Save & Exit. Settings-only rollback is `defer_gpu_wait=0`, `model_radius=300`.

Shadow quality 2, transport lock separation, a dedicated XR frame thread,
shared inter-eye work, different eye image dimensions and 96/108 Hz remain
untested. Source defaults were not changed by the owner's live settings trial.

## Movement follow-up

The owner ran for three further 360-pair captures with the original candidate,
deferred waiting on and radius 210. These are separate moving scenes, not a
matched static A/B.

| Sequence | Mean pair ms | p95 ms | Maximum ms | Input stage over 4 ms |
| --- | ---: | ---: | ---: | ---: |
| 10108 | 22.94 | 24.88 | 197.56 | 4 |
| 10109 | 22.82 | 28.22 | 35.88 | 8 |
| 10110 | 23.45 | 32.22 | 36.54 | 8 |

The 20 input-stage spikes were typically 12–13 ms. CPU/GDI painting of the
1024-square interaction atlas still ran synchronously in that stage. A local
Proton test measured 10.427 ms per repaint with actual artwork and icons.
The new candidate paints on a bounded worker and uploads completed pixels on
the frame thread, with a fast RGBA row copy. No game pointers, D3D12 objects or
XR calls are passed to the worker. Changed skill/character/page contents cannot
be clicked using an obsolete image. Vitals and hover refreshes retain the wheel.

Host/Proton tests cover latest-only work, failures, shutdown/restart and stale
UI contents. Twelve worker-painted images exactly matched the synchronous
images with varying pages, hover and vitals. Installed after Save & Exit:
`d44762d4cc4a9b2cefad350a6ada3b6779b3c9bf83211be835e7b1430b28442f`.
Record: `build-frame/latest-perf-ui-deployment.json`. Subsequent moving captures
10111–10113 had **zero input stages over 4 ms in 1,080 pairs** (maximum 1.811 ms).
Two HUD refreshes measured background paint 10.12/10.28 ms and frame-thread
upload 1.27/1.16 ms. Mean pair times were 23.21, 23.62 and 22.46 ms; the routes
and views differ from the earlier runs, so no overall FPS gain is inferred.
Evidence: `build-frame/perf-running-ui-{1,2,3}.log/.json` and
`build-frame/perf-ui-comparison.json`. Headset wheel/wrist usability feedback
is pending. The owner subsequently reported severe global stalls/quality loss,
and this candidate was rolled back; the narrow UI result does not establish
overall smoothness.

The 198 ms pair included a 173 ms LevelTranslationLayer simulation tick;
other traversal ticks in the live log reached 208–753 ms. Their recorded
pipeline creations, measured room discovery and extra VR tiling were zero.
The remaining game-update cost is unresolved; native room/model activation
or loading is a follow-up hypothesis, not a demonstrated diagnosis. The
asynchronous atlas change does not address these larger stalls.
