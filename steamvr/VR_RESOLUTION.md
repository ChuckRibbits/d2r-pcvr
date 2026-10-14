# Independent VR render resolution

Open **VR Settings → Video → VR render resolution**, choose a size, and
press **Apply**. Applying rebuilds the VR session and closes the panel; allow
several seconds for the view to return. Reopen Video to see the actual
**VR source** dimensions per eye.

| Choice | Source dimensions | INI value |
|---|---|---|
| Desktop (default) | Physical game-window client size | 0 |
| Headset | OpenXR runtime recommendation | 1 |
| 1536 × 1536 | Fixed square preset | 2 |
| 2048 × 2048 | Fixed square preset | 3 |
| 2560 × 2560 | Fixed square preset | 4 |
| 3072 × 3072 | Fixed square preset | 5 |
| 4096 × 4096 | Fixed square preset | 6 |

The preference is `[render] vr_resolution` in `d2rloader/plugins/d2r_vr.ini`.
Missing or invalid values retain Desktop. Start with a moderate preset and
increase only if frame timing permits. Independent sizes respect both eyes'
OpenXR limits and the system's swapchain limits, with an additional cap of
4096 pixels per side. Headset preserves the runtime's recommended aspect ratio.
The recommendation can change with runtime quality settings and is not
necessarily the headset panel's physical pixel count.

No custom monitor mode, EDID change, virtual monitor, or Settings.json edit is
needed. The physical window stays its existing size; the desktop mirror may
therefore appear stretched. Returning to Desktop restores the physical client
size. The override also releases on normal window close or XR session stop.
The saved preference does not change unmodded desktop play.

## What changes

Previously, the native OpenXR path copied the game's desktop-sized backbuffer
into equally sized eye textures. Raising compositor resolution could resample
that image, but could not supply detail missing from the game's source.

This setting requests a logical size through the game's own `WM_SIZE` path,
dispatched on the window thread. D2R recreates its native render resources;
ReShade's resize notifications recreate our OpenXR eye textures at that size.
The source-to-eye transfer remains a same-size copy. ReShade shaders are not
used to synthesize a larger scene. Native `ScreenToClient` and `ClientToScreen`
calls originating from the game executable are remapped for the game window,
so the game's cursor coordinates follow the resized image. Other windows and
the mod's physical desktop laser positioning retain their original coordinates.

D2R's resolution scale, dynamic resolution and DLSS still affect internal scene
detail. The displayed source size describes the final game output per eye; it
does not imply that every intermediate pass renders at that resolution.
Streaming bitrate, compression, lens correction and headset optics still
affect perceived clarity. Higher source sizes cost GPU time and memory.

## Validation and remaining headset checks

Automated Windows fixtures cover runtime limits, invalid choices, asynchronous
window resize/coalescing, physical-window preservation, desktop restoration,
setting persistence, and real user32 pointer conversion hooks including inverse
cursor warps and unrelated windows.

The October 13 Steam/SteamVR title-screen probe recreated the native source,
a depth target and OpenXR eye textures at 2560 × 2560 while the physical client
remained 3840 × 2160. Selecting Headset live used the runtime's 2160 × 2160
recommendation. The 4096 × 4096 preset also recreated matching depth and eye
textures with a 1280 × 1024 physical client, and Desktop restored 1280 × 1024
eye textures. No character was entered. This verifies resource allocation
and session rebuilding, not in-headset clarity or gameplay performance.

Pending offline headset checks: compare clarity and GPU frame time at a fixed
scene, test both eye images and aspect, click native menu edges/corners and
inventory slots, check controller laser alignment, apply and cancel choices,
relaunch, resize/minimize the desktop window, and reconnect the headset.
Validate VDXR separately. Keep the old DLL available for comparison.

Diagnostics in `d2rloader/logs/d2r-vr-frame.log` report the requested, source,
physical and recommended dimensions; matching depth allocations; and the
OpenXR session's actual eye size. A missing pointer hook or unavailable runtime
limits retains the desktop path rather than enabling misaligned input.
