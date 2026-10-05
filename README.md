# Fat Cat Pomodoro for Haiku

A cozy Pomodoro timer that fills your breaks with collectible animated cats.

A native Haiku recreation of [Fat Cat Pomodoro](https://github.com/jeremielumandong/omarchy-fat-cat). It is a long-running `BApplication` with a real Deskbar add-on; no Qt, QML, Electron, account, or network connection is used at runtime.

## Features

- Focus, short-break, and long-break phases. Defaults are 25/5/15 minutes with a long break after every four completed focus sessions.
- A live Deskbar readout. Primary-click opens Fat Cat; secondary-click starts a safe 15-second preview.
- The Deskbar item starts the timer service quietly at login and keeps it running after the settings window closes.
- An original cat-and-Pomodoro HVIF icon is embedded in `fatcat.app` and displayed in its native About box.
- Persistent, suspend-aware timers. Paused timers, interval progress, settings, collection progress, names, and favorites survive restarts.
- Four cats with distinct personalities and six activities: walk, stretch, groom, yawn, loaf, and sleep.
- Cat unlocks match the original: Mochi immediately, Miso after 1 completed break, Patches after 3, and Pepper after 6.
- Cat renaming, favorites, screen selection, reduced motion, and a no-streak collection model.
- Blocking break windows or gentle, non-modal break reminders. Escape and **Skip break** always dismiss a break; previews and skipped breaks never earn progress.
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

## Use

The **Timer** tab starts, pauses/resumes, and stops the timer; previews the sanctuary; and configures intervals, display, blocking, and motion. Interval edits affect the next phase, not the phase currently in progress.

The **Cats** tab shows collection progress. Names save when Enter is pressed or the field loses focus. If any unlocked cats are favorited, only favorites visit during real breaks; previews always show all four cats.

Gentle mode uses a compact floating window so the rest of the desktop remains interactive. Blocking mode covers the selected screen(s), while Haiku system shortcuts remain available. It is a break prompt, not a security lock.

Settings and timer state live in `~/config/settings/FatCat/`. Uninstalling keeps this directory so progress returns after reinstalling:

```sh
make uninstall
```

## Command-line actions

`fatcat-cli` provides the same control surface as the original plugin IPC. When Fat Cat is running:

```sh
fatcat-cli start
fatcat-cli about
fatcat-cli pause
fatcat-cli resume
fatcat-cli stop
fatcat-cli preview
fatcat-cli dismiss
fatcat-cli configure 25 5
fatcat-cli status
```

## Source layout

- `src/FatCatApp.*` — application lifecycle, IPC, persistence, and phase transitions
- `src/DeskbarView.cpp` — archived Deskbar replicant and live timer display
- `src/Session.*` — deterministic timer state machine
- `src/MainWindow.*` — native settings and collection UI
- `src/BreakWindow.*`, `src/CatView.*` — break/preview windows and animated sanctuary
- `artwork/fatcat-icon.svg`, `artwork/fatcat-icon.hvif` — editable source and embedded Haiku vector icon
- `tests/session_test.cpp` — transition, reward, restore, and clock-adjustment coverage

## License

MIT. See [LICENSE](LICENSE).
