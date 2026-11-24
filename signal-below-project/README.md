# Signal Below

An original, one-room Nintendo DS homebrew prototype, with a standalone browser edition. Dive into an unlit wreck. A sonar pulse maps the escape hatch, but wakes the thing near it. It moves only when you do.

## Play

- **Nintendo DS:** Open `signal-below.nds` in melonDS or load it through a homebrew-compatible setup for your exact DS model. Use the D-pad to move, tap the lower screen's sonar or press **A** to ping, and press **Start** to restart.
- **Browser:** Open `signal-below.html` in a browser. No internet connection or server is required. Use arrow keys, WASD, or the on-screen D-pad. Tap the sonar or press **Space** to ping. Press **Enter** or tap **Reset** to restart.

A successful ping reveals the hatch for the rest of the dive; the sonar echo lasts eight moves and can be fired again after three. Once awake, the creature takes a step every two successful player moves.

## Build the DS ROM from source

Install [BlocksDS](https://blocksds.skylyrac.net/docs/setup/) and its Wonderful Toolchain, then run `make` in this directory. The result is `signal-below.nds`. Use the SDK shell on Windows or set `BLOCKSDS` to the SDK's `core` directory if your installer doesn't set it automatically.

Alternatively, with Docker from this directory:

```sh
docker run --rm -v "$PWD:/work" -w /work skylyrac/blocksds:slim-latest make
```

Native core smoke test on a machine with a C compiler:

```sh
cc -std=c99 -Wall -Wextra -Isource tests/test_game.c source/game.c -o /tmp/signal-test
/tmp/signal-test
```

## Before ordering cartridge boards

This is a homebrew ROM, not a cartridge PCB design and not a claim that arbitrary DS boards can be flashed. Prove the loader and ROM on your exact retail console first. Then test a *single sample* of the intended board with its firmware/loader and power-cycle the console before ordering in bulk. The prototype has no save-file requirement. The HTML version is a separate implementation of the same gameplay, not a DS emulator.

- [BlocksDS guide to testing on hardware](https://blocksds.skylyrac.net/tutorial/basic/first_program/)
- [DSpico open hardware reference](https://github.com/LNH-team/dspico-hardware) if designing your own compatible flashcart
