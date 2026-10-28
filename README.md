# Freequay

Freequay runs Windows games and programs on Apple Silicon Macs, for free: Wine built from the open source part of CodeWeavers' CrossOver 26.3.0, with a small app and settings per game from open sources. It is not affiliated with CodeWeavers or Apple.

> [!WARNING]
> **Experimental.** Freequay has been tested on one Mac so far, a MacBook Air M1 with macOS 27.0.1, and not on an older macOS. Small games mostly run. Large or older games often need settings per game, some do not run yet, and games with anti-cheat usually do not start. [What runs](#what-runs) lists what was tried. Did a game run, or not? [Report it](https://github.com/zienert/freequay/issues/new?template=game-report.yml).

<img src="app/screenshot.png" width="660" alt="Freequay.app with the bottle games: Steam and Notepad in the list, below it a strip to drop installers on">

## Download

You need an Apple Silicon Mac with macOS 14 or newer, Rosetta 2 and about 1.5 GB of free disk space; Steam takes another 1.5 GB, plus your games. Neither Xcode, its Command Line Tools nor Homebrew are needed. Most Macs got Rosetta 2 with the first Intel app they opened; otherwise run `softwareupdate --install-rosetta` once in Terminal.

1. Download `Freequay-0.4.1.dmg` from the [latest release](https://github.com/zienert/freequay/releases/latest) and open it.
2. Drag Freequay into Applications and open it there. The first time, macOS refuses it because the app is not notarized by Apple: open System Settings, Privacy & Security, click Open Anyway next to the note about Freequay and confirm. After that it opens normally.
3. The app lists once which servers Freequay downloads from. Then click Create a bottle, a Windows drive of its own (about a minute, with the download of Wine Mono), and install Steam with its button, or drop the installer of a Windows program on the window.

A bottle is not a sandbox. Wine shows your whole Mac to Windows programs as drive Z:, so they can read and change all your files, just as a Mac program can. Only start Windows programs you trust.

Freequay itself collects no data. It downloads from AppleGamingWiki (settings for your Steam games), WineHQ (Wine Mono for a new bottle), Valve (Steam, when you install it), GitHub (winetricks), Microsoft and others through winetricks (runtime packages games ask for) and, only if you add it yourself, Apple (D3DMetal).

Everything Freequay needs is inside the app. What it downloads later, your bottles and D3DMetal live in `~/Library/Application Support/freequay`, so a newer Freequay replaces the app and keeps them.

## What runs

Tested on a MacBook Air M1 with 16 GB and macOS 27.0.1, not yet on an older macOS or another Mac. Small free games mostly run; large or older games often need settings per game. Reports are welcome as [game reports](https://github.com/zienert/freequay/issues/new?template=game-report.yml), also of games that run.

| Program | Result |
|---|---|
| Steam for Windows | installs, updates itself and signs in. The first start downloads about 575 MB in two updates; after each one Steam starts a new copy of itself and ends. On a busy Mac that copy sometimes did not come, and the old Steam stayed without a window; Freequay then ends it 30 seconds after the update and starts Steam again. In fresh test bottles on 2026-10-19 the sign-in window came after about two minutes. |
| Alien Swarm through Steam (Direct3D 9, 32 bit) | runs: about 95 FPS in the menu and about 20 FPS in a mission at 2048×1280, measured with Wine's frame counter (0.3.1, 2026-10-19). Cmd+Tab back to the fullscreen game worked 3 of 4 times, with the fix of 0.4.0 |
| Fallout Shelter through Steam (Unity, Direct3D 11) | runs through DXMT: 40 minutes without an error (0.3.1, 2026-10-19) |
| 14 small free games through Steam: Supermarket Together, Stumble Guys, Yu-Gi-Oh! Duel Links, Lords Mobile, Shop Titans, Gems of War, BLEACH Brave Souls, ONE PIECE Bounty Rush, Idle Slayer, NGU IDLE (all Unity, Direct3D 11 through DXMT), Tell Me Why (Unreal Engine 4), Leaf Blower Revolution (GameMaker), Legends of IdleOn (Electron), Infinitode 2 (OpenGL) | each opened a window within a minute and stayed open for three minutes; not played through, the picture was not checked (0.3.1, 2026-10-19). Two more gave no clear result |
| Team Fortress 2 through Steam (Direct3D 9) | ran smoothly against bots, best in fullscreen, on the first build (2026-10-13); not tested since |
| FINAL FANTASY XIV: Dawntrail Benchmark (Direct3D 11, .NET launcher) | runs through: 2679 points at 1920×1080 "High (Desktop)", 23 to 35 FPS in the foreground |
| Rockstar Games Launcher with Social Club | installs, updates itself, signs in and opens its main window. Downloading a game stalls on a MacBook Air M1 (GTA IV, 2026-10-19): the launcher has its background service write every piece of a file, the service manages about five pieces a second under Rosetta, and the downloads time out. With two displays the launcher sometimes hangs at its start; started in a virtual desktop (`freequay wine <bottle> explorer /desktop=Rockstar,1600x900 "C:\Program Files\Rockstar Games\Launcher\Launcher.exe"`) it opens |
| Grand Theft Auto IV through the Rockstar Games Launcher (Direct3D 9, 32 bit) | installed only with a workaround that is not part of Freequay (2026-10-20). Playable with wined3d and FusionFix, but at a low frame rate and with audio lag; with DXVK the picture shows black squares and the GPU crashes after some minutes. Work on the frame rate and the audio is not part of 0.4.1 |
| Black Myth: Wukong Benchmark Tool (Direct3D 12, D3DMetal) | hangs while loading, then Unreal's crash reporter comes up |
| Totally Accurate Battlegrounds through Steam | does not start: the launcher of its Easy Anti-Cheat crashes (2026-10-19). Games with anti-cheat usually do not run |

Videos in games play through GStreamer, for Media Foundation and DirectShow alike: MP4, AVI, Matroska, MPEG program streams, FLV and WAV, with H.264, H.265, MPEG-2, Motion JPEG and ProRes decoded by Apple's VideoToolbox and AAC and MP3 by AudioToolbox. There is no FFmpeg, so videos in WMV, VP8, VP9, Theora or MPEG-1 do not play. Tested through Wine so far is H.264 with AAC in MP4, with a test clip (see Tests), not yet the videos of a game. Apple's D3DMetal (Direct3D 11 and 12) is not included; the app leads you to Apple's own download and installs it from there. Without it, Direct3D 12 runs on Wine's own d3d12 with vkd3d, tried so far only with small test programs.

## Using the app

- The list shows the programs of the chosen bottle: Steam, its games, every program whose installer put a shortcut into the Start menu or on the desktop, and every `.exe` started from the app except installers (setup, install or unins in the name).
- Open starts a program. Programs start one at a time: the row shows a spinner until the program's window appears, and Open on a program whose window is still open brings it forward.
- An installer or any other `.exe` dropped on the window or the Dock icon, or double-clicked in Finder, starts in the chosen bottle. A click on the strip at the foot of the list picks one instead.
- Every bottle has your whole Mac as drive Z:, so a Windows program can read and change all your files. Open only programs you trust.
- A right click takes a program off the list. Nothing is deleted or uninstalled.
- Show files opens the bottle's drive C: in Finder; Stop all programs ends every program of the bottle. Programs keep running when the app quits.
- If a program lies in Downloads, Documents or on the Desktop, macOS asks whether Freequay may access that folder. Allow it, or Wine cannot read the program.
- Add D3DMetal opens Apple's download page for "Evaluation environment for Windows games", free with an Apple Account. Freequay finds the disk image in Downloads, notes that D3DMetal is Apple's and under Apple's license, which allows using it to develop, test and evaluate games, and installs it when you click Install. If macOS shows Apple's license as the image opens, agree to it.
- When it starts, the app lifts the download quarantine from what it carries inside. Without that, macOS would stop the unsigned `wine`. Opened right on the disk image, it asks to be moved to Applications first.
- The first time it opens, the app lists the servers Freequay downloads from, and why. Freequay itself collects no data.
- The settings for your Steam games come from AppleGamingWiki: the app looks up each installed game there, at most once a week (see Rules from open sources).

Everything else, such as the graphics per program, is done in Terminal with `freequay`, the launcher the app calls as well.

## With Terminal

```bash
cd ~
curl -LO https://github.com/zienert/freequay/releases/download/v0.4.1/Freequay-0.4.1.dmg
hdiutil attach -nobrowse -mountpoint /tmp/freequay Freequay-0.4.1.dmg
cp -R /tmp/freequay/Freequay.app /Applications/
hdiutil detach /tmp/freequay
alias freequay=/Applications/Freequay.app/Contents/SharedSupport/freequay/scripts/freequay
freequay new games
```

curl sets no quarantine, so this needs no Open Anyway. After a browser download, macOS quarantines everything copied out of the disk image and stops the unsigned `wine` with a dialog until the app or this command lifts it: `xattr -dr com.apple.quarantine /Applications/Freequay.app`.

The first `freequay new` downloads Wine Mono (82 MB) from WineHQ. `freequay fix` installs the runtime packages that rules ask for with winetricks, without the Command Line Tools or Homebrew: Freequay brings its own `cabextract`, which unpacks with the bsdtar of macOS.

The commands run from anywhere; in a checkout of the repository, `scripts/freequay` is the same launcher.

```bash
freequay new games
freequay run games --backend dxmt "/path/to/Game.exe"
freequay run games "$HOME/Library/Application Support/freequay/Bottles/games/drive_c/Program Files (x86)/Steam/steam.exe"
freequay run games steam:440
freequay programs games
freequay programs games "/path/to/Game.exe"
freequay programs games "/path/to/Game.exe" -
freequay app games tf_win64.exe wined3d
freequay info games steam:440
freequay info "Grand Theft Auto IV"
freequay fix games --list
freequay applegamingwiki
```

Bottles live in `~/Library/Application Support/freequay/Bottles`. `run` starts a Windows program, a shortcut (`.lnk`) or a Steam game by its ID (`steam:440`, which Steam starts with `-applaunch`), and takes `--backend`, `--avx` (Rosetta reports AVX to the game), `--hud` (Metal performance HUD) and `--debug`. A program without a rule gets the bottle's default backend (`dxmt`, change it with `freequay set <bottle> backend <name>`) or the one from `--backend`. `dxmt` and `dxvk` have no Direct3D 12, so a program that imports `d3d12.dll` gets `d3dmetal` instead when it is installed, otherwise `wined3d`; without D3DMetal, programs that import `d3d11.dll` as well, as Unreal Engine games do, keep the default (details in `compat/README.md`).

`programs` lists what a bottle has, one name and `run` target per line, as the app shows it. With a program after the bottle it adds that to the list, and with `-` after the program it takes it off again: an added program is forgotten, a found one hidden. The bottle keeps both in `programs.txt`.

`freequay app <bottle> <program.exe|steam:ID> <backend>` sets the graphics for one program, `-` instead of the backend deletes it, and without a program the command lists the bottle's rules. `freequay rule` adds any directive, `freequay info` shows everything that applies to a program. New rules apply at the next program start, also while Steam is running. `freequay applegamingwiki` looks up the installed Steam games in AppleGamingWiki at once; `run` and `programs` do it by themselves.

Steam for Windows without the app: install it with `SteamSetup.exe /S` and start `steam.exe` with `run`. Before Steam starts, `run` catches up on the one-time setup the rules ask for (`freequay fix`). Until Steam's window is up, `run` also follows Steam's restarts after its updates: if no new copy of Steam has started 30 seconds after an update, it ends what is left of Steam in the bottle and starts Steam again, at most twice, and notes that in `steam-watch.log` in the bottle. `freequay kill` stops this as well.

## About

A quay is where ships come ashore; here Windows games land on your Mac. Quay is pronounced like "key".

Freequay is not affiliated with, endorsed by or supported by CodeWeavers or Apple. CrossOver is a trademark of CodeWeavers, Inc. CodeWeavers publish the sources this builds on and do most of the work on Wine for the Mac; if you want something that just works, with support, buy CrossOver.

### What is in it

- Wine 11.0 with all CodeWeavers patches from `crossover-sources-26.3.0.tar.gz`: msync, the D3DMetal hooks in winemac.drv, Rosetta tweaks and their fixes for single programs (Steam, the Rockstar Games Launcher, Epic, Ubisoft Connect, GTA IV and others). Built as x86_64 for Rosetta 2; 32-bit programs run through WoW64.
- FreeType, GnuTLS, nettle and gmp as x86_64 libraries from the same package. nettle 3.10 comes from the GNU release because the package lacks its `Makefile.in`.
- GStreamer 1.24.4 with GLib 2.78.0 from the same package, for Wine's winegstreamer, which plays the videos and sounds of Media Foundation and DirectShow: the core, base and good plugins for the containers, and from the bad plugins the parsers and applemedia, which decodes through Apple's VideoToolbox and AudioToolbox. pcre2 10.42 and proxy-libintl 0.4, which GLib needs and the package lacks, come from their own releases.
- Graphics backends, chosen per program:
  - `dxmt`: DXMT 0.80 (Feifan He, MIT; versions after 0.80 are LGPL-2.1), Direct3D 10 and 11 on Metal.
  - `dxvk`: DXVK 1.10.3, CodeWeavers' Mac version from the same package (zlib), Direct3D 9 and 11 over Vulkan, with CodeWeavers' MoltenVK (Apache-2.0), which emulates geometry shaders.
  - `wined3d`: Wine's own Direct3D up to version 11; on the Mac CodeWeavers send Direct3D 10 and 11 through Vulkan. Direct3D 12 runs on Wine's d3d12 with vkd3d over MoltenVK.
  - `d3dmetal` and `d3dmetal3`: Apple's D3DMetal from the Game Porting Toolkit 4.0 beta 2 and 3.0, Direct3D 11 and 12. Never included: you download it from Apple yourself, and Apple's license applies; the app then installs it.
- Wine Mono from WineHQ for .NET programs; every new bottle gets it. Freequay does not ship it, the first new bottle downloads it.
- Rules per program: the open counterpart of CrossOver's closed compatibility database, filled from umu-protonfixes, Proton, ProtonDB and DXVK. The rules from AppleGamingWiki are not included; Freequay looks them up on your Mac (see below).
- `Freequay.app`, with all of the above inside, and `scripts/freequay`, the launcher for bottles that it calls, with its helpers: `applegamingwiki`, `fix` for the one-time setup with winetricks, and `cabextract`, which unpacks Microsoft's cabinet files with the bsdtar of macOS.

Wine is built for macOS 11 and newer, MoltenVK for macOS 12 and the app for macOS 13. DXMT names macOS 14 as its minimum and recommends macOS 15; its release 0.80 is built for macOS 15. So every part is meant to run on macOS 14 and newer.

### What is not in it

Nothing from the CrossOver app itself: not its user interface, no CrossTie installers, no D3DMetal from CrossOver and not CodeWeavers' compatibility database. No FFmpeg, and nothing from GStreamer's ugly plugins or gst-libav, whose codecs are under the GPL or have open patent questions.

### Compared with CrossOver 26.3

| Part | CrossOver | Freequay |
|---|---|---|
| Wine with CodeWeavers' patches | yes | yes, the same source |
| D3DMetal (Apple) | included | from Apple's download |
| DXMT | yes | yes (release 0.80) |
| DXVK with CodeWeavers' MoltenVK | yes | built from the same package |
| wined3d | yes | yes |
| msync, 32-bit through WoW64 | yes | yes |
| .NET through Wine Mono | yes | yes, from WineHQ |
| Settings per game | closed compatibility database | open rules (patch 0005, `compat/`) from protonfixes, Proton, ProtonDB and DXVK, and from AppleGamingWiki, made on your Mac |
| One-click installers (CrossTie) | yes | no; `freequay fix` installs the runtime packages the rules ask for with winetricks |
| User interface | yes | one window that starts programs (`Freequay.app`) and the command `freequay` |
| GStreamer (videos in games) | yes | yes, without FFmpeg: H.264, H.265, MPEG-2 and AAC through Apple's VideoToolbox and AudioToolbox |

## Build

You need Xcode (MoltenVK is built with xcodebuild), Homebrew with `bison pkgconf mingw-w64 autoconf automake libtool m4 ninja glslang cabextract python` and about 4 GB of disk space for sources, build and result. All commands run from the root of the repository.

```bash
scripts/fetch-sources.sh      # CodeWeavers' sources, nettle, pcre2, proxy-libintl and gvdb, unpacked and patched in sources/
scripts/build-deps.sh         # gmp, nettle, GnuTLS, FreeType in deps/
scripts/build-gstreamer.sh    # GLib and GStreamer for the videos in games, in deps/
scripts/build-moltenvk.sh     # CodeWeavers' MoltenVK in deps/lib; Wine's configure looks for it
scripts/build-wine.sh         # Wine in dist/wine
scripts/build-dxvk.sh         # DXVK in dist/backends/dxvk
scripts/install-dxmt.sh       # DXMT in dist/backends/dxmt
scripts/install-mono.sh       # Wine Mono for new bottles
scripts/compat/update.sh      # rules from the open sources, optional: compat/ has a recent copy
```

`build-gstreamer.sh` builds GLib and GStreamer as x86_64 for macOS 11 with meson, as a cross build because meson runs on Homebrew's arm64 Python; Wine's configure finds them through pkg-config. The plugins decode through Apple's frameworks, so no FFmpeg is built. `build-wine.sh` builds Wine for macOS 11 and newer and leaves out `pipe2`. The macOS 27 SDK has that function and older macOS does not; Wine calls it without checking, so with it Wine would most likely not start on macOS 26 and older. Releases up to 0.2.0 were built with it. `build-wine.sh` also builds the 32-bit Windows DLLs, the ones a 32-bit game loads, with SSE2 floating point (`i386_CFLAGS`). For 32-bit code gcc writes x87 instructions by default, which Rosetta emulates through slow runtime routines; Wine's Direct3D fixed-function pipeline, its D3DX math and its XAudio2 were several times slower in 32-bit programs than in 64-bit ones (see Tests). Every Mac with Rosetta or an Intel processor has SSE2. make does not rebuild anything when only flags change, so in a tree built before, delete the 32-bit object files first: `find build/wine -path '*/i386-windows/*' \( -name '*.o' -o -name '*.a' \) -delete`.

D3DMetal: the app leads to Apple's download and installs it. Without the app, download "Evaluation environment for Windows games" from developer.apple.com, mount it and run `scripts/install-d3dmetal.sh "/Volumes/<volume>"`. It installs D3DMetal 4 as `d3dmetal` and version 3 as `d3dmetal3` into `~/Library/Application Support/freequay/backends`, where it stays when Freequay is updated.

`scripts/build-app.sh` builds `Freequay.app` into the root of the repository with swiftc from Xcode or the Command Line Tools, and the helpers in `scripts/compat`: `applegamingwiki`, `fix` and `cabextract`. `scripts/package.sh 0.4.0` builds them as well, puts the build with the launcher, the rules and the license texts into `Freequay.app/Contents/SharedSupport/freequay`, signs the app with `scripts/sign.sh` and packs it into `out/Freequay-0.4.1.dmg`, the release disk image. `sign.sh` signs every Mach-O file with the hardened runtime and the entitlements Wine needs (`app/Freequay.entitlements`); without an identity the signature is ad hoc, with a Developer ID (Apple's developer program) the same call signs for notarization, which then only needs `xcrun notarytool` and `xcrun stapler`.

## Patches

- `0001-ntdll-priority-dll-path`: `FREEQUAY_PRIORITY_DLLPATH` puts a backend's DLLs before Wine's builtin d3d11 and dxgi. CrossOver does the same from its closed compatibility database through `prepend_dll_path()`.
- `0002-win32u-zero-fill-monitor-device-name` and `dxvk-0002-dxgi-output-device-name-size`: the display name (`\\.\DISPLAY1`) arrives padded with zeros as on Windows. Before, DXMT, DXVK and D3DMetal passed on garbage behind it, and the FFXIV benchmark launcher cut the game's command line there.
- `0003-kernelbase-chromium-helpers-software-rendering`: `steamwebhelper.exe` and Rockstar's `SocialClubHelper.exe` get `--disable-gpu --disable-gpu-compositing --in-process-gpu`. Their Chromium otherwise draws from a separate GPU process, which winemac.drv does not show: the windows stayed black.
- `0004-registry-hkcu-software-shared-view`: `HKCU\Software` is the same key for 32 and 64 bit, as on Windows 7 and later, even when a `Wow6432Node` exists below it. Steam creates one; Wine then redirected 32-bit access there, `steam_api64.dll` did not find the running Steam, and Team Fortress 2 crashed at start.
- `0005-ntdll-compat-rules-per-program`: rules per program. Every program looks up rules for its path or Steam app ID in the files from `FREEQUAY_COMPAT_FILES` when it starts and gets its graphics backend, environment variables, DLL overrides and launch arguments from them; a broken launcher can be replaced by the game itself. ntdll sets environment variables before a child process starts, because Rosetta reads `ROSETTA_ADVERTISE_AVX` only at exec. That is how Steam runs with wined3d and a game started from it with DXVK. A program that imports `d3d12.dll` and has no backend rule gets D3DMetal or wined3d, because DXMT and DXVK have no Direct3D 12 and Wine's d3d12 does not work with their dxgi. Format and commands: `compat/README.md`.
- `0006-cfgmgr32-reject-unknown-notification-handles`: `CM_Unregister_Notification` rejects unknown handles as Windows does. Steam's `steamclient64.dll` unregisters a stale handle when a game quits; Wine wrote into freed memory and an error dialog came up after Team Fortress 2.
- `0007-winemac-defer-display-mode-while-hiding`: Cmd+Tab out of a game in exclusive fullscreen, which captured the displays to change their mode, comes back to the game. Wine hides the app on Cmd+Tab, but macOS deactivates it only a moment later. A game that restored the desktop mode and minimized itself in that moment, as Direct3D 9 games do, made winemac.drv capture the displays again, which brought the app back to the front: its window minimized, the screen black without Dock and menu bar, and Cmd+Tab leading away from the game. The mode is now kept for when the app is active again, as for an app in the background. This does not cover every case: with Alien Swarm 3 of 4 returns by Cmd+Tab worked, once the game stayed minimized and neither Cmd+Tab nor the Dock brought it back (2026-10-19).
- `0008-mfreadwrite-stride-of-converted-video-type`: the Media Foundation source reader gives a video type it converts to, such as RGB32 from NV12, the stride of that format, as Windows does; Wine's own tests mark this as a known difference. Before, it copied the NV12 stride, and a program that reads `MF_MT_DEFAULT_STRIDE` read every frame a quarter wide.
- `0009-winecoreaudio-microphone-on-request`: Windows programs get no microphone unless a rule allows it for that program (`env=FREEQUAY_MICROPHONE=1`, see `compat/README.md`). Without it winecoreaudio.drv lists no capture device and refuses to open one, also one that Windows remembers from an earlier start; sound output is unchanged. Before, every program could record, and a game with voice chat kept the microphone open as long as it ran. macOS did not ask when Freequay was started from an app that already had access to the microphone, because it counts the game as part of that app.
- `0010-dsound-sse2-floating-point-on-i386`: dsound, Wine's DirectSound, is built with SSE2 floating point for 32-bit programs too. It mixes every sound in floating point and converts a sound at another rate than the output's, such as 44.1 kHz on the Mac's 48 kHz, with a filter of about 66 points for every sample. For 32-bit code gcc writes x87 instructions, which Rosetta emulates slowly: the mixer of a 32-bit program took a whole core for a single such sound and still made only half of it in time, so the sound of 32-bit games stuttered and lagged behind, whatever the graphics backend. GTA IV is one of them. Rosetta translates SSE2 into native instructions; the 32-bit mixer now keeps up with about 60 such sounds on one core of an M1, the 64-bit one, which always had SSE2, with about 130 (see Tests). `build-wine.sh` now builds all 32-bit DLLs with SSE2 (see Build), so the patch only matters for a build without that.
- `0011-opengl32-batch-calls-of-32-bit-programs`: a 32-bit program reaches the Mac's OpenGL through WoW64, and every call switches the processor from 32-bit to 64-bit mode and back, which Rosetta emulates: in a frame of `tests/d3d9-cpu` 90 to 150 ns more per state call than in a 64-bit program, more than many of these calls cost the driver. wined3d makes about 8,000 calls for 1000 draws. opengl32 now collects in each thread the calls that return nothing and take only values (state, bindings, and `glUniform*v` with a copy of the values) and hands them over with the next other call, such as a draw, a query or SwapBuffers, in the same switch; the Unix side runs them in their order before that call. Draw calls stay direct because they read the program's memory, and so do all functions whose thunks do more than pass the call on. A 32-bit `glDisable` now takes 28 instead of 129 ns (64-bit: 60 ns), and wined3d's command thread needs about 0.8 ms less for a frame of `tests/d3d9-cpu` (see Tests). Nothing changes for 64-bit programs; `FREEQUAY_GL_BATCH=0` turns it off.
- `0012-opengl32-buffer-size-only-for-whole-maps`: in a 32-bit program Wine places a mapped OpenGL buffer below 4 GB, and at the first map of a buffer it asked the driver for the buffer's size, also when the map gives its range. On the Mac that query waits for the driver, 50 µs and more. A 32-bit wined3d maps no buffer for good and makes a new one for every discarding map of a dynamic buffer, which games do several times a frame, so this wait was most of the 56 µs such a map took. The size is now asked for only for a map of the whole buffer; four such maps take 61 to 67 µs a frame instead of 265 to 320.
- `dxvk-0003-config-from-environment`: DXVK also reads options from `DXVK_CONFIG` (separated by `;`), as DXVK 2.x does, so rules can set DXVK options per game.
- `moltenvk-0002-spirv-cross-alias-overlapping-samplers`: DXVK declares a depth-compare twin of every Direct3D 9 sampler on the same binding. SPIRV-Cross turned the twin's texture into an alias of the argument buffer slot but forgot its sampler, so every Direct3D 9 pixel shader with a texture failed to compile in MoltenVK and DXVK drew nothing. The sampler now gets the same alias.
- `moltenvk-0003-split-render-pass-for-render-target-reads`: Direct3D 9 games sometimes sample a texture that is also their render target, and DXVK puts a barrier before each such draw. On Apple GPUs MoltenVK ignored barriers inside a render pass, so every such draw read the texture as it was before the pass: 16 draws that each add to their own pixel left the value of one. MoltenVK now splits the Metal render pass at such a barrier, as it already did for a barrier into the GENERAL layout.
- `dxvk-0004-d3d9-copy-render-target-hazards`: within one draw a split does not help. Apple GPUs render in tiles and write a tile to memory as soon as they are done with it, so a draw that reads neighboring pixels of its own render target gets new values at the tile edges. GTA IV has such a full-screen pass on its HDR image, and the errors grew into black blocks on a 16-pixel grid (2026-10-20, found by switching single passes off in 24 frames taken from the game). With MoltenVK, DXVK now copies the render target before such a draw and lets the draw sample the copy. This applies to 2D render targets with one mip level, the others keep the barrier; the option `d3d9.copyRenderTargetHazards` switches it.
- `dxvk-0001`, `moltenvk-0001` and `glib-0001` keep DXVK, MoltenVK and GLib buildable with mingw-w64 14, Xcode 27 and Python 3.12 and newer.

## Rules from open sources

`compat/` holds the rules tested here (`manual.txt`) and rules from four open sources; the rules from a fifth, AppleGamingWiki, are made on your Mac. `scripts/compat/update.sh` downloads the four sources again and regenerates their files; what a source does not give, or what only applies on Linux, goes to `notes.tsv` and shows up in `freequay info`.

| File | Source | License | Taken (2026-10-13) |
|---|---|---|---|
| `applegamingwiki.txt`, made on your Mac | AppleGamingWiki, reports on CrossOver on Apple Silicon | CC BY-NC-SA 3.0 | graphics backend for 80 games, where the reports recommend exactly one and none reports problems with it; `ROSETTA_ADVERTISE_AVX` and MoltenVK variables |
| `protonfixes.txt` | umu-protonfixes, fixes per game | BSD-2-Clause | 204 of 313 fixes: winetricks verbs, DLL overrides, variables, registry, launch arguments, replaced launchers |
| `proton.txt` | Valve's Proton, launcher script | BSD-3-Clause | 77 games: heap zeroing, FNA on Direct3D 11, OpenSSL mask, DLL overrides |
| `protondb.txt` | ProtonDB, open export of all reports | ODbL 1.0 | 94 games: launch arguments, DLL overrides and Large Address Aware that at least three reports and a quarter of the working reports since 2023 agree on |
| `dxvk.txt` | DXVK, current game profiles | zlib | 165 of 261 profiles with options DXVK 1.10.3 knows; only with `backend=dxvk` |

Most of these come from Linux, where Proton runs with DXVK and Vulkan drivers; only the AppleGamingWiki rules are about the Mac. CodeWeavers' compatibility pages sit behind bot protection and are not read automatically.

AppleGamingWiki's license allows no commercial use, so Freequay does not ship its rules. It looks up the Steam games installed in a bottle one by one instead, each at most once a week, with two requests: the infobox of a game's page links to steamdb.info with its Steam ID, and AppleGamingWiki's list of pages with that link names the pages Freequay then reads. `freequay run <bottle> steam:<ID>` looks up the game right before it starts; the other installed games are looked up in the background when the app lists them, and so are the games Steam installs while it runs. The rules are kept in `~/Library/Application Support/freequay/compat`. Without network a game starts without them, and the next start tries again. The rules are the same as the ones Freequay 0.2.1 made from all pages, apart from Steam IDs that a page lists only as further editions, nearly all DLC that no game starts with. Of the 85 games with a rule on 2026-10-18, all matched except One Piece Odyssey, whose rule came from a misspelled page holding a copy of GTA V's text. The program behind it is `scripts/compat/applegamingwiki.swift`, a port of the Python importer of 0.2.1.

The one-time setup (winetricks verbs, registry values, Windows version) is done by `freequay fix <bottle>` for all installed Steam games. All games share a bottle, so the DLL overrides a winetricks verb sets apply only to the programs of the game that asked for it.

## Tests

Graphics tests from 2026-10-13 on a MacBook Air M1 with macOS 27.0.1; the programs in `tests/` ran again with every backend on 2026-10-18 for 0.2.1, 0.2.2 and 0.3.0, and for 0.3.1 and 0.4.0 without `--backend` and with `dxvk`:

| Test | Result |
|---|---|
| `cmd /c ver` | Microsoft Windows 10.0.19045, msync active |
| Direct3D 11, 64-bit, `dxmt` | feature level 11_0, adapter "Apple M1", triangle with shaders correct |
| Direct3D 11, 64-bit, `wined3d` over OpenGL | feature level 10_1, image correct |
| Direct3D 11, `wined3d` over Vulkan (CrossOver's default) | image correct with CodeWeavers' MoltenVK; crashed with the official MoltenVK |
| Direct3D 9, `wined3d` | image correct |
| Direct3D 9 with shaders and a texture (`tests/d3d9-shader`), 32- and 64-bit | correct with `wined3d`; with `dxvk` correct since patch moltenvk-0002, before that nothing was drawn |
| Direct3D 9 shadow map: a depth texture sampled with comparison (`tests/d3d9-shadow`), 32- and 64-bit | correct with `wined3d` and `dxvk` |
| Direct3D 9, a texture sampled while it is the render target (`tests/d3d9-feedback`), 32- and 64-bit, 2026-10-20 | correct with `wined3d`; with `dxvk` correct since patches moltenvk-0003 and dxvk-0004, before that every draw saw the texture from before the render pass. With only moltenvk-0003, 4 of 786,432 pixels at tile corners read their neighbor too early; `wined3d` has 3 |
| Direct3D 9, an INTZ depth buffer sampled while it is the read-only depth buffer of the draw (`tests/d3d9-depth-read`), 32- and 64-bit, 2026-10-20 | correct with `wined3d` and `dxvk`, also with an A16B16G16R16F render target and stencil writes |
| Direct3D 9 Reset (`tests/d3d9-reset`), 32-bit, 2026-10-20 | 60 resets in a window and 30 in fullscreen with display mode changes between 1440×900, 1280×800 and 1024×640: no lost device with `wined3d` and `dxvk` |
| Direct3D 9 and 11, `dxvk`, 32- and 64-bit | image correct with CodeWeavers' MoltenVK; the official one lacks geometry shaders |
| Direct3D 11, 32-bit, `dxmt` and `wined3d` | image correct (WoW64) |
| Direct3D 12, offscreen triangle with shaders (`tests/d3d12-draw`) | renders correctly with `d3dmetal`, `d3dmetal3` and with vkd3d over MoltenVK (Wine's own d3d12 with `wined3d`), with vkd3d also as a 32-bit program. `dxmt` and `dxvk` have no Direct3D 12; up to 0.2.0 `D3D12CreateDevice` failed with them, so a program without a rule did not start |
| Direct3D 12 without `--backend` or with `dxmt` or `dxvk` (`tests/d3d12-smoke`, `d3d12-draw`, `d3d12-draw32`), 0.2.1 | the 64-bit programs got `d3dmetal` with D3DMetal installed and `wined3d` without it, the 32-bit one `wined3d`; all correct, also when started by another program. The Unreal Engine 5 executable of the Wukong benchmark imports `d3d11.dll` and delay-loads `d3d12.dll`: it got `d3dmetal`, and kept `dxmt` without D3DMetal |
| Direct3D 11, `d3dmetal` (Apple 4.0 beta 2) | image correct; the adapter is called "AMD Compatibility Mode" |
| Direct3D 11, `d3dmetal3` (Apple 3.0) | image correct |
| Display name (`tests/devname`), all backends | padded with zeros with patches 0002 and dxvk-0002; garbage before with DXMT, DXVK and D3DMetal |

The programs under What runs need these backends, which their rules set: Steam and the Rockstar Games Launcher run with `wined3d`; with `dxmt` both abort at `SwapDeviceContextState`, the launcher with `dxvk` at `DXGID3D10CreateDevice`. Steam's window shows with patch 0003. Team Fortress 2 runs with `wined3d`, starts with patch 0004 and quits cleanly with patch 0006; with `dxvk` its image stayed black before patch moltenvk-0002 and was not tried again. The FFXIV benchmark ran with `dxmt`, its launcher on Wine Mono; a clean comparison of all backends is still open.

The release archives were tested like a new user would: downloaded with the quarantine a browser sets, unpacked into a new folder with a space in its path, with an empty home directory. With 0.1.0 all 29 graphics tests passed (Direct3D 9, 11 and 12, 32 and 64 bit, `dxmt`, `dxvk`, `wined3d`, vkd3d, and `d3dmetal` and `d3dmetal3` installed with the archive's `scripts/install-d3dmetal.sh`), and Steam installed, updated itself and showed its sign-in window. With 0.2.0 the app lifted the quarantine of its folder, created a bottle in 34 seconds with the Wine Mono download, opened Notepad from Finder, the Dock, a drop and its list within 2 to 4 seconds, installed Steam with its button and showed Steam's sign-in window about 80 seconds later. Programs started from the app stayed in the list after a restart, installers stayed out and removed entries stayed away; quick repeated clicks gave one program, and a click on an open program brought it forward. Not tested: Open Anyway for an app really downloaded with a browser. With 0.2.1, which changes the backend for Direct3D 12 and builds Wine without `pipe2`, the build folders were hidden during the test; the bottle took 31 seconds with Wine Mono copied from the build, and all 12 programs in `tests/` passed with `dxmt`, `dxvk`, `wined3d` and without `--backend`, then also with `d3dmetal` and `d3dmetal3` after `scripts/install-d3dmetal.sh` from the archive. The app and Steam were not tried again.

0.2.2 looks up the rules from AppleGamingWiki on the Mac, game by game. Its archive was tested the same way, with the build folders out of the test's reach. The bottle took 31 seconds and all 12 programs passed with every backend: 48 runs without D3DMetal, 72 with it. Started as one of seven Steam games, a program got the same backend, variables and AVX setting as under 0.2.1. The 85 games with a rule on 2026-10-18 got the same rules as from all pages, except One Piece Odyssey (see Rules from open sources). A game started with `run steam:<ID>` was looked up in about a second, without network the start went on after two seconds without the rules, and a game Steam installed while it ran was looked up within a minute. Steam installed from the temp folder as the app's button does it. The app lifted the quarantine of 1,933 files when it opened, looked up nothing before its note on downloads was read and the installed game three seconds after, and started Notepad from Finder in two seconds.

0.3.0 is one app on a disk image. The image carried the quarantine of a browser download; the app was copied out of it and opened with an empty home directory, with the build folders out of reach and the Command Line Tools' `python3`, `lipo` and `otool` blocked. The app lifted the quarantine of what it carries. The first bottle took 37 seconds with Wine Mono downloaded from WineHQ into the data folder, and the app's signature stayed valid, so nothing was written into the app. All 12 programs passed with every backend: 48 runs without D3DMetal, 72 after the app installed D3DMetal 4.0 beta 2 and 3.0 from disk images in Downloads named and laid out like Apple's. After the app was moved to another folder, they passed again with `dxmt`, `d3dmetal` and `d3dmetal3`. `freequay fix` set up a Steam game with DirectX packages in 62 seconds, without python3 or Homebrew. Steam installed and showed its sign-in window on its second start, and the app started Notepad from Finder in under a second. Not tried: Apple's download page itself, which needs an Apple Account, and Open Anyway for an app downloaded with a browser.

0.3.1 fixes the `cabextract` of 0.3.0, which left its work folder in the temporary folder of macOS after every call, several hundred MB for each DirectX package a game asked for. On Microsoft's DirectX package (`directx_Jun2010_redist.exe`) it now gives the same 40 cabinets and 40 DLLs as the real cabextract, byte for byte, and leaves no folder behind, also when a call fails. The image was tested like 0.3.0's, with the app approved by the Terminal steps above and Homebrew blocked as well: the first bottle took 61 seconds with the Wine Mono download, `freequay fix` set up a game with `d3dcompiler_43` and `d3dx9_43` in 48 seconds and left nothing in the temporary folder, the launcher removed a work folder older than an hour and kept a newer one, and all 12 programs passed without `--backend` and with `dxvk`.

Steam's first start was tested again on 2026-10-19. After an update, Steam's updater starts a new copy of itself and ends with exit code 42. Without a change, the new copy came within two seconds in 8 fresh bottles in a row, started from the launcher and from the app, with the CPU busy, with a second click on Steam during an update and next to a Steam in another bottle; the sign-in window came after 94 to 143 seconds. Once, with the Mac very busy and its swap almost full, the old Steam wrote "Shutdown" after its second update, stayed without a window and started no new copy; the hangs of 2026-10-17 and 2026-10-18 also came while the Mac was short of memory or disk. With the change, the launcher started Steam again 30 seconds after the update in that case and when the new copy was killed or frozen on purpose at the first update; the sign-in window then came after 338 and 158 seconds. In five more fresh bottles in a row Steam came back by itself each time and the sign-in window came after 114 to 119 seconds, through the app's Install Steam after 137 seconds, and in a bottle where Steam was set up already after 20 seconds. With three first starts at once on one connection, Steam's download of its two largest packages broke off and Steam showed "Failed to load steamui.dll"; a full disk gives the same message. Steam then has to be started again.

Patch 0007 was tested on 2026-10-19 with the build from this repository in a new bottle, with `tests/minimize-on-deactivate` as a 32- and a 64-bit program; `tests/macswitch` pressed Cmd+Tab and clicked into the window and on the Dock, each run only after 45 seconds without input at the Mac. Without a mode change, Cmd+Tab back brought the minimized window back already before the patch: borderless windows do go into the Dock. With a mode change to 2048×1280, the unpatched winemac.drv brought the program back to the front by itself in 2 of 16 switches, its window minimized; with a test build that sets the desktop mode in the moment macOS hides the app, in 4 of 4, once with the displays still captured and three times with Cmd+Tab then leading away from the program. With the patch, in 48 runs and 86 switches, with and without the mode change and that test build, on `WM_ACTIVATEAPP` and on `WM_ACTIVATE`, and with Direct3D 9 in exclusive fullscreen through `wined3d`, where Direct3D minimizes the window itself as in Alien Swarm, the program never came back by itself, and 79 of 81 returns by Cmd+Tab or a click on its Dock icon restored the window. The other two were in one run of the test build: macOS restored the window, but the program was not told, and seven reruns with logging did not show it again. All 12 graphics programs passed without `--backend` and with `dxmt`, `dxvk` and `wined3d`. Alien Swarm itself was tried later that evening, see Patches.

Video tests from 2026-10-19 on the same Mac. `tests/make-clip.swift` writes a 2-second MP4 with Apple's encoders: H.264 in the High profile with B-frames, 320×240 at 25 frames per second, with four colored quadrants that move on after one second, and AAC in stereo at 48 kHz, 440 Hz on the left and 1000 Hz on the right. `tests/mf-video` plays it through Media Foundation, `tests/dshow-video` through DirectShow; both check the decoded frames and sound themselves, without a screen capture, and count a hang as an error.

| Test | Result |
|---|---|
| Microphone (`tests/microphone`), 32- and 64-bit, 2026-10-20 | without a rule no waveIn device and no default capture endpoint, three output devices; started with `start`, every capture endpoint Windows remembered from earlier was refused with `AUDCLNT_E_DEVICE_INVALIDATED`. With `env=FREEQUAY_MICROPHONE=1` for the test program the Mac's microphone was listed, and another program in the same bottle at the same time still got none |
| Media Foundation source reader, to RGB32 and float, 32- and 64-bit | all 50 frames with the right colors, 1.99 s of sound, 440 Hz left and 1000 Hz right. Correct with patch 0008; before it the reader reported a stride of 320 bytes instead of 1280 |
| Media Foundation media session into sample grabbers, the way Unreal Engine 4 plays videos, 32- and 64-bit | played in real time (2.04 s); all frames and both tones correct |
| DirectShow, with Wine's GStreamer splitter, sample grabbers and null renderers, 32- and 64-bit | played in 2.24 s; all frames (RGB32, bottom-up) and both tones (16-bit PCM) correct |
| The same programs with 0.3.1 | Media Foundation refuses the file (`MF_E_UNSUPPORTED_BYTESTREAM_TYPE`) |

The disk image with GStreamer was tested like 0.3.1's: the app copied out of it, an empty home directory and the build folders renamed. A new bottle took 26 seconds and all four video programs passed. The first run took 7 seconds instead of 4, because Rosetta translates GStreamer once and GStreamer lists its plugins once; the list is kept in the data folder. A bottle made by 0.3.1 got winegstreamer by itself at its first start and passed as well. As a check, all 12 graphics programs passed without `--backend` and with `dxvk`. GStreamer alone, outside Wine, also decoded H.265, MPEG-2, Motion JPEG, ProRes and MP3 test files to the end, and refused VP9 for lack of a decoder. GStreamer makes the disk image about 5 MB larger: 0.4.0's is 150 MB, 0.3.1's 146 MB. Not tried yet: the videos of a real game.

0.4.0 combines the three changes above: the launcher that follows Steam's restarts, patch 0007 and GStreamer with patch 0008. Wine, GStreamer, the other libraries and DXVK were built fresh from the sources for it; MoltenVK, whose sources did not change, is the build of 0.3.1. The disk image was tested like 0.3.1's: the app copied out of it with the Terminal steps above and run with an empty home directory, the build folders renamed, and the build trees, Homebrew and the developer tools out of reach. The first bottle took 37 seconds with the Wine Mono download. The four video programs passed with the results in the table above, and all 12 graphics programs passed without `--backend` and with `dxvk`. Steam installed from Valve's installer in three new bottles, as the app's button does it. In the first, both restarts came within a second, the sign-in window came 99 seconds after the start, and the launcher did not step in. In the second, the new copy after the first update was stopped for half a minute after it had started; the launcher left it alone, and the window came after 140 seconds. In the third, the new copy was killed right after the update, before it had come up; the launcher started Steam again 30 seconds after the update, and the window came after 142 seconds. `tests/minimize-on-deactivate` ran 8 times, as a 32- and a 64-bit program: with the mode change to 2048×1280, with Direct3D 9 in exclusive fullscreen, without a mode change and minimizing on `WM_ACTIVATE`. In all 16 switches away by Cmd+Tab the window minimized and the program stayed in the background with the displays released. All 16 returns, 10 by Cmd+Tab and 6 by a click on its Dock icon, restored the window. The app itself, started from the copy with an empty home directory, showed its note on downloads first and listed Steam in a test bottle; its signature was still valid after all tests, so nothing was written into it. Not tried: the videos of a real game.

0.4.1 is 0.4.0 with patch 0009 and the microphone entitlement. Wine was rebuilt with patches 0001 to 0009 in the tree of the GStreamer build; GStreamer, the other libraries, DXVK, DXMT and MoltenVK come from that tree, built from the same sources as 0.4.0. The app was copied out of the disk image and run with an empty home directory, and no file in it links into a build folder. The first bottle took 53 seconds with the Wine Mono download. `tests/microphone`, 32- and 64-bit: without a rule no waveIn device, no default capture endpoint and three output devices; with the rule for it the Mac's microphone was listed. The Direct3D 11 and Direct3D 9 smoke tests drew correctly with the default backend, and the 2-second clip played through Media Foundation as a 32- and a 64-bit program and through DirectShow, both sound channels as expected. Not tried: macOS asking for the microphone when an allowed game records, Steam's first start, and Open Anyway after a browser download.

Sound tests from 2026-10-20 on the same Mac, its speakers at 48 kHz in stereo. `tests/dsound-mix` plays a number of silent DirectSound buffers at 44.1 kHz, three of four in mono, as a game plays its sounds, so that dsound has to convert each of them to 48 kHz. It measures how fast the play cursor of one more such buffer moves against the clock and how much CPU the process takes, nearly all of it the mixer thread. Below 100 % of real time the output runs dry, and the sound stutters and falls behind. Nothing can be heard.

| Test | Result |
|---|---|
| 32-bit, dsound of 0.4.1 | the mixer took a whole core with any number of buffers and fell behind already with one: 51 % of real time with 1 buffer, 34 % with 2, 21 % with 4 and 7 % with 16, with stalls of up to 1.1 s. 16 buffers at 48 kHz, which need no conversion, made 82 % |
| 32-bit with patch 0010 | real time up to 48 buffers (4 % CPU with 1, 25 % with 16, 50 % with 32, 73 % with 48), at the limit with 64 (99 %, a whole core), 51 % with 128. 32 buffers at 22.05, 32 and 44.1 kHz: real time at 46 % CPU; 64 buffers at 48 kHz: 24 %; 32 buffers next to four busy threads at normal priority: real time |
| 64-bit, unchanged by the patch | real time up to 128 buffers (12 % CPU with 16, 43 % with 64, 91 % with 128), 53 % with 256; 64 buffers at 48 kHz: 5 % |

GTA IV, a 32-bit game, with the dsound of 0.4.1 on 2026-10-20 (Rockstar Games Launcher, wined3d): its mixer thread took a whole core all the time, measured from outside, and in a one-second `sample` 75 of its 77 stacks ended in Rosetta's runtime routines, not in translated code. Its sound stuttered and lagged, also with DXVK. Not tried yet: GTA IV with the patch.

Floating point in 32-bit programs, tests from 2026-10-20 on the same Mac, all with `wined3d`: the 32-bit DLLs of 0.4.1, built with x87 (dsound already with patch 0010), against the same DLLs built with SSE2 as `build-wine.sh` builds them now. `tests/d3d9-cpu` draws 1000 small cubes a frame, one draw call each, in a small window without vsync, so that the frame time is what Direct3D 9 costs on the CPU: with shaders and their constants, with the fixed-function pipeline (a world transform per cube, two lights, fog), or with shaders and the matrices of every cube built by D3DX each frame. `tests/xaudio2-mix` plays silent XAudio2 voices at 44.1 kHz, three of four in mono, which the engine converts to the Mac's 48 kHz, and measures them like `tests/dsound-mix`, also the share of the time the engine spends mixing.

| Test | x87 (0.4.1) | SSE2 | 64-bit program |
|---|---|---|---|
| Direct3D 9 with shaders, time per frame | 6.4 to 7.0 ms | the same in each pair of runs | 4.7 to 5.2 ms |
| Direct3D 9 fixed-function | 21.6 to 22.0 ms | 6.8 to 6.9 ms | 5.2 to 5.6 ms |
| Direct3D 9 with D3DX math | 26.5 to 27.5 ms | 6.4 to 6.8 ms | 4.6 to 4.7 ms |
| XAudio2, 16 voices | real time at 56 % CPU (8 voices: 39 %) | real time at 3 % | real time at 2 % |
| XAudio2, 64 voices | 55 % of real time, a whole core | real time at 6 % | real time at 5 % |
| XAudio2, 128 voices | 28 % of real time | real time at 15 % | real time at 9 % |
| XAudio2, 512 voices | | real time at 34 % | real time at 22 % |
| XAudio2, 64 voices at 22.05, 32 and 44.1 kHz | 56 % of real time | real time at 9 % | real time at 5 % |
| XAudio2, 64 voices with a low-pass filter each | 24 % of real time | real time at 17 % | real time at 8 % |

With x87, the program's own thread took a whole core in the fixed-function and D3DX runs, and wined3d's command thread waited half of the time; in a `sample`, 102 of 321 stacks of the program's thread ended in Rosetta's runtime routines, with SSE2 6 of 114. With SSE2 both threads work all the time, as in the 64-bit program. With shaders alone nothing changed: there the 32-bit program loses its time in the calls into OpenGL, not in floating point. One DLL at a time: wined3d alone brought the whole gain of the fixed-function run; in the D3DX run d3dx9 took it from 26.5 to 9.7 ms, and ucrtbase, whose `sin` and `cos` D3DX calls, on to 6.8 ms; d3d9 and opengl32 alone changed nothing. XAudio2 gains through FAudio, its mixer inside Wine (`libs/faudio`), which has SSE2 code for converting, resampling and mixing but uses it only when it is compiled with SSE2. The images drawn with SSE2 are the same bit for bit as those of the 64-bit program; with x87, 21 and 31 of their 76,800 pixels differed. DirectSound mixed as with patch 0010.

With the SSE2 build all 12 graphics programs passed without `--backend` and with `wined3d` and `dxvk`, and the four video programs passed. Some of Wine's own tests, built as 32-bit programs, gave the same results against both builds: d3d9 `visual` (211,215 checks, the same 47 failures, from OpenGL on the Mac) and `stateblock`, d3dx9 `math`, `effect` and `line`, ucrtbase `misc` and `printf`, msvcrt `misc` and `printf`, msvcr120 and xaudio2, all of these without failures. Not tried yet: a 32-bit game with the SSE2 build.

OpenGL calls of 32-bit programs, tests from 2026-10-20 at night on the same Mac, all with `wined3d`: the SSE2 build above against the same build with patches 0011 and 0012. `tests/gl-call-cost` times millions of single OpenGL calls in a small window, as a 32- and a 64-bit program; `tests/d3d9-cpu` as above. Every value is from runs right after one another; the first run of a changed DLL is left out, because Rosetta translates it then.

| Call, ns per call | 32-bit before | 32-bit with patch 0011 | 64-bit program |
|---|---|---|---|
| Far call into 64-bit mode and back, as Wine's WoW64 thunk does it, without Wine's code | 68 | 68 | |
| Syscall `NtQueryPerformanceCounter` | 133 to 135 | 134 to 135 | 53 |
| `glDisable` without a current context | 94 | 21 to 25 | 49 |
| `glDisable` | 129 | 28 to 33 | 60 |
| `glActiveTexture` | 117 to 118 | 27 | 56 |
| `glBindTexture`, two textures in turn | 137 to 138 | 64 | 70 to 71 |
| `glBlendColor` | 121 | 31 | 57 |
| `glUniform4fv` | 193 to 194 | 127 to 128 | 122 to 123 |
| `glGetError`, still direct | 128 | 131 | 58 |

| `tests/d3d9-cpu`, 1000 draws, time per frame | 32-bit before | with 0011 | with 0012 | with both | 64-bit program |
|---|---|---|---|---|---|
| Shaders | 7.61 to 7.99 ms | 5.61 to 5.85 ms | 7.26 to 7.54 ms | 5.67 to 5.79 ms | 5.55 to 5.56 ms |
| Fixed-function | 8.33 ms | | | 6.68 to 6.96 ms | 5.56 ms |
| D3DX math | 7.61 to 8.06 ms | | | 5.61 to 5.69 ms | 5.55 to 5.56 ms |

The 32-bit runs before the patches are slower than in the SSE2 table above (6.4 to 7.0 ms), the 64-bit ones too (4.7 to 5.2 ms); during these runs the window server took about 40 % of a core, and why the Mac was slower is not known. The 64-bit program stayed at 180 frames per second, three times the 60 Hz of the display, with 1000 and with 1100 draws (5.56 and 5.57 ms) and was faster with 800 and 900 (4.22 and 4.63 ms), so at 1000 draws its time per frame shows the pace of presenting, not its CPU cost. A temporary build of wined3d with timers in its command thread (rdtsc around every command, which adds some 20 ns to each) shows the CPU cost, in µs per frame, shaders, three runs each:

| wined3d's command thread | 32-bit before | 32-bit with both patches | 64-bit program |
|---|---|---|---|
| 1004 draws, with their state and OpenGL calls | 5,090 to 5,390 | 4,360 to 4,495 | 3,790 to 4,020 |
| 4 maps of the dynamic vertex buffer | 265 to 320 | 61 to 67 | under 55 |
| Shader constants and the other commands | 308 to 343 | 313 to 342 | 152 to 194 |
| All of it without the present | 5,670 to 6,050 | 4,730 to 4,900 | 3,940 to 4,220, and commands under 1 % of the time each |
| Present (`wglSwapBuffers`, also waiting for the display) | 820 to 1,150 | 480 to 575 | 1,050 to 1,330 |
| The program's thread waiting for the maps | 570 to 860 | 97 to 157 | 556 to 917 |

Before the patches the command thread was the limit: it worked through nearly all of the frame, and the program's thread waited for it at its maps in every frame. With timers around every OpenGL call as well, the 32-bit command thread spent 5.8 ms of a frame in its 8,012 calls, the 64-bit one 4.7 ms, without SwapBuffers 5.1 against 3.7 ms. Per call: `glActiveTexture` 238 against 90 ns, `glBindTexture` 192 against 100, `glBindSampler` 195 against 86, `glDisable` 184 against 70, the other state calls 160 to 190 against 60 to 70, `glUniform4fv` 282 against 163, the draw call 3.1 against 2.8 µs, `glMapBufferRange` 56 µs against under 2 µs. wined3d's own code on the command thread took 0.2 ms more than in the 64-bit program. So the 32-bit program lost its time in the switches to 64-bit mode, which cost more in the game-like loop than in `tests/gl-call-cost`, and in the maps of new buffers, of which the size query of patch 0012 was most. Of the 8,012 calls, about 3,560 set a state to the value it had already (`glActiveTexture` every time, `glBindSampler` in 97 % and `glDisable` in 87 % of the calls, the blend, depth and raster state of every eighth draw nearly always); wined3d does not filter them, and since patch 0011 each costs a 32-bit program about 30 ns. A draw call still makes one switch.

With both patches all 12 graphics programs passed without `--backend` and with `wined3d` and `dxvk`, and the four video programs passed; their images were the same as before (`tests/d3d9-cpu` gave the hash of the 64-bit program). Wine's own tests, built as 32-bit programs: d3d9 `visual` gave the same 211,215 checks with the same 47 failures, line for line, and `stateblock` 14,738 checks without failures. opengl32's test gave the same output line for line with both patches, with patch 0011 turned off (`FREEQUAY_GL_BATCH=0`) and with the DLLs of the SSE2 build, apart from handle values: 15,425 checks with 2,062 failures, all from OpenGL on the Mac, also those of `test_gl_error`, which checks the order of OpenGL errors. Its `test_memory_map` ends the process with every one of these builds and was left out of these runs. Not tried yet: a 32-bit game with the patches, and the batching for 64-bit programs, where a switch costs about 50 ns.

`tests/` has the small programs behind these tests; build them as their first comment says, with mingw-w64 and `macswitch` with clang, and the video clip with `tests/make-clip.swift`. `tests/rulecheck` shows what the rules changed for a program: its path, command line, environment variables and where `d3d11.dll` comes from. `tests/minimize-on-deactivate` is a fullscreen window that minimizes itself when the program loses the focus, and `tests/macswitch` presses Cmd+Tab and clicks the Dock for it; it needs Accessibility for the program that starts it.

## Folders

`downloads/` source packages, `sources/` unpacked and patched sources, `build/` build trees, `deps/` x86_64 libraries, `dist/` the finished build with its backends, `out/` release disk images, `tools/` meson, `logs/` logs, `Freequay.app` the app built from `app/`, `scripts/compat/applegamingwiki`, `fix` and `cabextract` the programs built from the `.swift` files next to them. Only `app/`, `scripts/`, `patches/`, `compat/`, `tests/`, `licenses/`, `.github/` (the issue forms) and the documents are versioned.

## Reports and help

- A game that runs or does not run: [game report](https://github.com/zienert/freequay/issues/new?template=game-report.yml).
- A bug in Freequay itself: [bug report](https://github.com/zienert/freequay/issues/new?template=bug.yml).
- A security problem: in private, see [SECURITY.md](SECURITY.md).
- Rules for games and changes to the code: see [CONTRIBUTING.md](CONTRIBUTING.md).

There is no support: a report may stay unanswered, and a fix is not promised.

## License

- Scripts, app, tests and documents of this repository: MIT, see `LICENSE`.
- `patches/0*.patch` change Wine and are under Wine's license, LGPL-2.1-or-later, `patches/glib-*.patch` under GLib's, LGPL-2.1-or-later as well. `patches/dxvk-*.patch` are under DXVK's zlib license, `patches/moltenvk-*.patch` under MoltenVK's Apache-2.0.
- The generated files in `compat/` keep the license of their source, named in each file's header: `protondb.txt` ODbL 1.0, `protonfixes.txt` BSD-2-Clause, `proton.txt` BSD-3-Clause, `dxvk.txt` zlib. Each row of `notes.tsv` names its source and keeps that source's license.
- The rules from AppleGamingWiki are not part of Freequay: Freequay looks them up on your Mac, where they keep AppleGamingWiki's license, CC BY-NC-SA 3.0.
- Builds contain Wine, GStreamer, GLib, DXVK, MoltenVK, DXMT and other components under their own licenses. If you pass a build on, those licenses apply, including the LGPL's source requirements. D3DMetal is Apple's and under Apple's license.
- The app has the license texts of all its parts in `Contents/SharedSupport/freequay/licenses/`; `licenses/README.md` lists them and where their source code is.
