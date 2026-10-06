# Fat Cat Pomodoro v0.07 for Haiku

<p align="center">
  <img src="artwork/fatcat-icon.svg" alt="Fat Cat icon" width="128" height="128">
</p>

A cozy Pomodoro timer that fills your breaks with collectible animated cats.

A native Haiku recreation of [Fat Cat Pomodoro](https://github.com/jeremielumandong/omarchy-fat-cat). It is a `BApplication` with a Deskbar add-on; no Qt, QML, Electron or other framework is used.

## Features

- Focus, short-break, and long-break phases. Defaults are 25 minutes focus, 5 minutes short break, 15 minutes long break, and one long break after every 4 completed focus sessions.
- A live Deskbar readout with the Fat Cat HVIF icon. A green dot means the
  background service is responding; a grey **Off** state means it is stopped.
  Primary-click launches or reveals Fat Cat, and secondary-click starts a safe
  15-second preview.
- Fat Cat runs as a background application in Deskbar and keeps working after
  the settings window closes.
- An original cat-and-Pomodoro HVIF icon is embedded in `fatcat.app` and displayed in its native About box.
- Persistent, suspend-aware timers. Paused timers, interval progress, settings, collection progress, names, and favorites survive restarts.
- Four cats with distinct personalities and six activities: walk, stretch, groom, yawn, loaf, and sleep.
- Cat unlocks match the original: Mochi immediately, Miso after 1 completed break, Patches after 3, and Pepper after 6.
- Cat renaming, favorites, screen selection, reduced motion, and a no-streak collection model.
- Full-screen break scenes place correctly paced cats over a snapshot of the desktop, with a compact control panel near the Deskbar. Escape and **Skip break** always dismiss a break; previews and skipped breaks never earn progress.
- Clock changes are bounded, sleep time counts, overdue snapshots advance only one phase, and state writes are atomic.

The bundled sprite sheets are copied from the MIT-licensed upstream project. The original copyright notice is retained in [LICENSE](LICENSE).

## Build on Haiku

Install the Haiku development tools, then run:

```sh
make
make test
make install
```

The install target places:

- `fatcat.app` and the sprites in `~/config/non-packaged/apps/FatCat/`;
- `fatcat-cli` in `~/config/non-packaged/bin/`;
- the replicant in `~/config/non-packaged/add-ons/deskbar/FatCatDeskbar.so`.

Restart Deskbar once if the item does not appear immediately:

```sh
quit Deskbar
```

Deskbar is restarted automatically by Haiku. Launch `~/config/non-packaged/apps/FatCat/fatcat.app` directly if you want to open the settings window before restarting Deskbar.
Fat Cat installs its Deskbar item once at application startup. Relaunch Fat Cat
after manually restarting Deskbar to restore the item.

## Packaging

HaikuPorts is the intended release format and produces a standard installable
`.hpkg`. The recipe template is in
[`packaging/haikuports/fatcat-0.07.recipe.in`](packaging/haikuports/fatcat-0.07.recipe.in).

After publishing the GitHub repository and creating the `v0.07` tag, replace
the template's `@HOMEPAGE@`, `@SOURCE_URI@`, `@CHECKSUM_SHA256@`, and
`@SOURCE_DIR@` values. Copy the completed recipe to
`haiku-apps/fatcat/fatcat-0.07.recipe` in a HaikuPorts tree and build it on
Haiku with:

```sh
haikuporter -S fatcat-0.07
```

The resulting package installs the application, sprites, `fatcat-cli`, Deskbar
add-on, documentation, and application-menu entry. The existing `make install`
target remains available for development builds outside package management.

## Use

The **Timer** tab starts, pauses/resumes, and stops the timer; previews the sanctuary; and configures intervals, display, blocking, and motion. Interval edits affect the next phase, not the phase currently in progress. **Quit Fat Cat** exits the background application; the window's close button only hides it, so the Deskbar item stays available for the next launch.

The **Cats** tab shows collection progress. Names save when Enter is pressed or the field loses focus. If any unlocked cats are favorited, only favorites visit during real breaks; previews always show all four cats.

Breaks and previews cover the selected screen with a snapshot-backed scene, so
the cats appear to walk over the desktop without requiring window transparency.
Fat Cat hides its settings window before taking that snapshot. The control panel
stays near the top-right Deskbar area. **Keep break overlay in
front** makes real breaks modal; when disabled, another application can still be
activated normally. Haiku system shortcuts remain available in either mode.

Settings and timer state live in `~/config/settings/FatCat/`. Uninstalling keeps this directory so progress returns after reinstalling:

```sh
make uninstall
```

## Command-line actions

`fatcat-cli` provides the same control surface as the original plugin IPC. It
starts the background application automatically when a command needs it:

```sh
fatcat-cli start
fatcat-cli about
fatcat-cli pause
fatcat-cli resume
fatcat-cli stop
fatcat-cli preview
fatcat-cli dismiss
fatcat-cli quit
fatcat-cli configure 25 5
fatcat-cli status
```

## Source layout

- `src/FatCatApp.*` — application lifecycle, IPC, persistence, and phase transitions
- `src/DeskbarView.*` — archived Deskbar replicant and live timer display
- `src/Session.*` — deterministic timer state machine
- `src/MainWindow.*` — native settings and collection UI
- `src/BreakWindow.*`, `src/CatView.*` — break/preview windows and animated sanctuary
- `artwork/fatcat-icon.svg`, `artwork/fatcat-icon.hvif` — editable source and embedded Haiku vector icon
- `tests/session_test.cpp` — transition, reward, restore, and clock-adjustment coverage

## License

MIT. See [LICENSE](LICENSE).
