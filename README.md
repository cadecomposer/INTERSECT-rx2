# INTERSECT (RX2 + MIDI fork)

A fork of [tucktuckg00se/INTERSECT](https://github.com/tucktuckg00se/INTERSECT) with REX2 loop import and MIDI export.

## Disclaimer - This fork was made with the assistance of AI.

I forked this project because I didn't really feel comfortable making a PR; if I actually wrote it myself then I probably would have made a PR. I don't know how to write code and I just threw this together with Pi Coding Agent. So I can't really comment on the quality of the code, and I can't promise to maintain it; I just wanted to have this feature for my music production workflow on Linux with REAPER. It happens to work for my use case, and this at least demonstrates that it's possible. In any case, I did still want to share this at least, just in case someone else finds this useful. I spent a lot of time trying to find something that already had this functionality, and needless to say I couldn't find anything.

**Full documentation:** <https://tucktuckg00se.github.io/INTERSECT/>

**Support development:** [Sponsor on GitHub](https://github.com/sponsors/tucktuckg00se) · [Buy me a coffee](https://buymeacoffee.com/tucktuckgoose)

INTERSECT is a sample slicer instrument plugin (VST3/AU/Standalone) with multi-sample sessions, per-slice locking, slice note ranges, multiple time/pitch algorithms, and MIDI-triggered slice playback.

This fork adds **REX2 (`.rx2`) loop import** via the [VelociLoops](https://github.com/kunitoki/VelociLoops) library: loading a `.rx2` file decodes the full loop and automatically creates a slice for every slice embedded in the REX2 metadata, with the loop tempo applied as the kit BPM.

It also adds **MIDI export**: a **MIDI** button in the header bar (next to SAVE). Drag it out of the plugin window to drop a `.mid` file into your DAW, or right-click it to save the file via a file browser. The exported file contains every active slice as a note — its assigned MIDI note, timed from its position in the sample, with velocity taken from its volume — plus the kit tempo, so dropping it on a DAW timeline creates a playable MIDI clip of the whole pattern.

![INTERSECT screenshot](.github/assets/screenshot.png)
*Theme shown: Open Color (`oc.intersectstyle`)*

## Quick Start

[Watch the Quick Start Guide on YouTube](https://youtu.be/zsdtyIff2PQ)

## Install

Download the latest release zip for your system from [Releases](https://github.com/tucktuckg00se/INTERSECT/releases) (Windows and Linux come in `x64` and `arm64`; macOS in `arm64` and `x64`) and copy the plugin files into your system plugin folder.

| Format | Windows | macOS | Linux |
| --- | --- | --- | --- |
| VST3 | `C:\Program Files\Common Files\VST3\` | `~/Library/Audio/Plug-Ins/VST3/` | `~/.vst3/` |
| AU | n/a | `~/Library/Audio/Plug-Ins/Components/` | n/a |

After copying, rescan plugins in your DAW.

**macOS first launch:** release builds are unsigned, so macOS may report INTERSECT as "damaged" on first launch. Clear quarantine flags from a Terminal:

```bash
xattr -cr ~/Library/Audio/Plug-Ins/VST3/INTERSECT.vst3
xattr -cr ~/Library/Audio/Plug-Ins/Components/INTERSECT.component
xattr -cr /Applications/INTERSECT.app
```

For per-platform release contents, ONNX Runtime bundles for stem separation, and GPU runtime requirements, see the [Installation guide](https://tucktuckg00se.github.io/INTERSECT/installation/).

## Build

```bash
git clone --recursive https://github.com/tucktuckg00se/INTERSECT.git
cd INTERSECT
cmake -B build
cmake --build build --config Release
```

Requires CMake 3.22+, a C++20 compiler, and Git. For per-OS toolchain setup, build outputs, the release workflow, and the dependency list, see [Build from source](https://tucktuckg00se.github.io/INTERSECT/building-from-source/).

## License

INTERSECT is licensed under the [GNU General Public License v3.0](LICENSE).

## Support / Known limitations

- INTERSECT project recall stores sample file paths for every file in the session; if files move, relink is required.
- Builds are unsigned; platform security prompts (especially macOS) may require manual trust/quarantine removal.
- Report bugs or request features via [GitHub Issues](https://github.com/tucktuckg00se/INTERSECT/issues).
