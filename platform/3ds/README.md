# Nintendo 3DS build

Donkey Kong 64 Recompiled for the New Nintendo 3DS and New 2DS XL, with
stereoscopic 3D. The renderer and platform layer are
[rt64-3ds](https://github.com/oneup03/rt64-3ds) (`lib/rt64-3ds`).

## Installing

- **CIA** (`DK64Recompiled3D-3ds-<commit>.cia`): install it with FBI or another
  CIA installer. It appears on the HOME Menu as "DK64: ReKongpiled".
- **.3dsx** (`DK64Recompiled3D-3ds-<commit>.zip`): extract it to the root of
  the SD card and start `3ds/DK64/DK64.3dsx` from the Homebrew
  Launcher.

Both need your own ROM: the US version of the N64 release of Donkey Kong 64,
in .z64 format, at `sdmc:/3ds/DK64/DK64.z64`. The game checks the ROM's hash
at start and deletes a file that does not match, so copy it rather than move
it. Saves and `settings.ini` are kept in the same folder.

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

## Performance

A New 3DS runs the game at its full 30 fps and, by default, draws an
interpolated frame between each two of the game's (the Game page's Frame
rate: 60 interp.). The heaviest scenes drop back toward 30. An Old 3DS
defaults to the game's own frame rate, as it can't afford the frames in
between. `dbg_old_clock=1` in `settings.ini` drops a New 3DS to the Old
3DS clock to see how an Old 3DS would fare.

The game's own culling stays on here where the desktop build draws
everything for widescreen: objects off the 5:3 screen (the test's bounds
pushed out by the margin the screen shows beyond the 4:3 frame) and map
chunks behind the camera are not drawn, and the RSP's own per-object
cull is honoured by the renderer, which also drops triangles beyond the
screen's edges before they reach the GPU. The game's own level of detail
is kept (far objects use their far models, as on the N64), and on an Old
3DS the camera's depth buffer is written every other frame.

The Game page's **Particles** row (Full, Half, Quarter; an Old 3DS starts
at Quarter) thins bursts of particle sprites, the sparkles, dust, splashes
and explosions: of a burst, the first is drawn and then one in two or
four. Each sprite is a draw of its own for the renderer, which is what an
Old 3DS cannot afford by the hundred. Sprites spawned singly, HUD sprites
and sprites attached to characters are never thinned, and the thinned
ones still exist for the game's own logic; only their drawing is skipped.

An Old 3DS gives a game 64 MB, too little to hold the 32 MB ROM, so there
the game reads the ROM from the SD card as it needs it; loads take a
little longer. `rom_stream` in `settings.ini` chooses: 0 automatic (the
SD card on an Old 3DS), 1 in memory, 2 from the SD card. A real Old 3DS
is untested.

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
