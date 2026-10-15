# Nintendo 3DS build

Donkey Kong 64 Recompiled for the Nintendo 3DS family, with stereoscopic
3D. A New 3DS or New 2DS XL is recommended; an Old 3DS or 2DS runs it, but
slowly (see Performance). The renderer and platform layer are
[rt64-3ds](https://github.com/oneup03/rt64-3ds) (`lib/rt64-3ds`).

## Installing

- **CIA** (`DK64Recompiled3D-3ds-<commit>.cia`): install it with FBI or another
  CIA installer. It appears on the HOME Menu as "DK64: ReKongpiled". On an
  Old 3DS or 2DS, use the CIA (see Performance).
- **.3dsx** (`DK64Recompiled3D-3ds-<commit>.zip`): extract it to the root of
  the SD card and start `3ds/DK64/DK64.3dsx` from the Homebrew
  Launcher.

Both need your own ROM: the US version of the N64 release of Donkey Kong 64,
in .z64 format, at `sdmc:/3ds/DK64/DK64.z64`. The game checks the ROM's hash
at start and deletes a file that does not match, so copy it rather than move
it. Saves and `settings.ini` are kept in the same folder.

For sound, the console's DSP firmware must be on the SD card at
`sdmc:/3ds/dspfirm.cdc`, as for any homebrew with audio. Dump it once with
Luma3DS: open the Rosalina menu (L + D-pad down + SELECT), then
Miscellaneous options > Dump DSP firmware. Without it the game runs silent.

## Controls

| 3DS | N64 |
|---|---|
| A / B | A / B |
| L | Z |
| R | R |
| START | START |
| X, D-pad up | C-Up |
| Y, D-pad down | C-Down |
| ZL, D-pad left | C-Left |
| ZR, D-pad right | C-Right |
| Circle Pad | analog stick |
| C-Stick | C buttons or the analog camera (Controls settings) |

SELECT opens the settings on the bottom screen: 3D depth, controls,
camera, gyro aiming, the game options and the built-in mods. With Tag
Anywhere on, D-pad left/right switch kongs instead.

Holding SELECT for a second saves a screenshot to
`sdmc:/3ds/DK64/screenshots/`: both eyes when 3D is on, and the touch
screen, as BMPs. Use it rather than Luma's Rosalina screenshot, which has
frozen the console with this game running.

## Stereo 3D

The console's 3D slider turns the 3D on and sets how strong it is; with
the slider down the game draws one image. The settings' 3D page tunes it:

| Setting | What it does |
|---|---|
| 3D depth | The separation between the eyes at the top of the slider (default 50). Raise it for more depth, lower it if the image is hard to focus. |
| Convergence | How far into the scene the screen's plane sits (default 10.0): things nearer than it come out of the screen, things beyond it go in. |
| HUD depth | Where the HUD, menus and text sit (50 is on the screen's plane; higher brings them out of the screen, lower pushes them in). |
| Auto convergence | On by default: when something comes close to the camera (a wall, a tree, a character in a close-up), the screen's plane moves nearer so it doesn't pop out too far. Convergence stays the farthest it goes. |
| Comfort target | How far out of the screen Auto convergence lets near things come (0 by default; higher for more pop-out, lower to keep everything behind the screen). |
| Ghost reduction: contrast, Ghost black floor | Ease ghosting (a faint double image of high-contrast edges on some screens) by softening contrast and lifting blacks. Off by default (100% and 0%). |

In first person (the scope, the guns), the crosshair sits at the depth of
whatever it is aimed at.

## Performance

**New 3DS / New 2DS XL:** holds 60 fps most of the time. The game itself
runs at 30, and by default an interpolated frame is drawn between each two
of its frames (the Game page's Frame rate: 60 interp.). The heaviest scenes
drop back toward 30. Set Frame rate to Game for the original 30 fps.

**Old 3DS / 2DS:** runs at 15-20 fps, with the occasional audio glitch
and some input delay. Install the CIA rather than running the .3dsx: an Old
3DS gives a game only 64 MB, too little to hold the 32 MB ROM, so the game
reads the ROM from the SD card as it needs it and keeps as much of it as it
can in memory, and the CIA leaves it more room for that than the Homebrew
Launcher does. Loads take a little longer than on a New 3DS. It starts at
the game's own frame rate, as it can't afford the frames in between, and
with particles at Quarter.

To make it run on the 3DS, this build keeps a few things the desktop build
turns off or ignores. The game's own culling stays on, widened to the
wider screen, and far objects use their low-detail models, as on the N64.
The Game page's **Particles** row (Full, Half, Quarter) thins bursts of
sparkles, dust, splashes and explosions: each particle costs a draw, which
an Old 3DS can't afford by the hundred. Only their drawing is skipped; the
game still handles them as usual.

## Built-in mods

The 3DS has no mod loader, so a few community mods (all CC0) are built in,
each switched on and off on the settings' Mods page:

| Setting | Mod |
|---|---|
| Tag Anywhere | [DK64TagAnywhereRecomp](https://github.com/Killklli/DK64TagAnywhereRecomp): switch kongs anywhere with D-pad left/right |
| No coin door | [RecompNoCompanyCoins](https://github.com/theballaam96/RecompNoCompanyCoins): Helm's coin door opens without the Nintendo and Rareware coins |
| Auto bonuses | [RecompAutocompleteBonuses](https://github.com/theballaam96/RecompAutocompleteBonuses): a bonus barrel gives its reward at once |
| Easy beetle | [RecompEasierBeetle](https://github.com/theballaam96/RecompEasierBeetle): slower beetle races |
| Fixed beavers | [RecompFixedBeaverBother](https://github.com/theballaam96/RecompFixedBeaverBother): gold beavers, beavers that go down the hole |
| Slow shoe | [RecompSlowShoe](https://github.com/theballaam96/RecompSlowShoe): a slower toe sequence in the K. Rool fight |
| Slow DK phase | [RecompSlowDKPhase](https://github.com/theballaam96/RecompSlowDKPhase): a wider window in K. Rool's DK phase |

No coin door sets the door's flag in the save when a file starts, so the
door stays open after switching it off. Slow shoe and Slow DK phase apply
from the next time the fight's map loads.

## AI use

The 3DS port, including this build's code and documentation and the
rt64-3ds renderer it uses, was written with the help of AI tools (Claude),
under the developer's direction and with their testing on real consoles.

## Building

Needs devkitARM with libctru, citro3d and picasso; makerom and bannertool
for the CIA; clang with the MIPS target and ld.lld for the patches; a host
C++20 compiler, CMake and Ninja; the submodules
(`git submodule update --init --recursive`; the mods in `lib/mods` are
skipped by that and fetched by `make patches`); and the decompressed US ROM
`donkeykong64.decompressed.us.z64` at the repository root, as for the
desktop build.

```sh
make -C platform/3ds nmr-patch host-tools   # once: runtime patch series, recompiler
make -C platform/3ds codegen                # recompile the ROM and the patches
make -C platform/3ds cia                    # build-3ds/DK64Recompiled3DS.3dsx and .cia
```

`make -C platform/3ds` without a target builds only the .3dsx. The header of
`platform/3ds/Makefile` lists the other targets. The `build-3ds` job in
`.github/workflows/release.yml` runs the same steps in devkitPro's container.
