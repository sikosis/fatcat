<p align="center">
  <img src="artwork/fatcat-icon.svg" alt="Fat Cat Pomodoro" width="256">
</p>

# Fat Cat Pomodoro [![platform](https://img.shields.io/badge/platform-Haiku-blue)](https://www.haiku-os.org/) ![build](https://img.shields.io/badge/build-active-brightgreen) [![license](https://img.shields.io/badge/license-MIT-blue)](LICENSE) ![version](https://img.shields.io/badge/version-v0.17-orange)

A cozy Pomodoro timer that fills your breaks with collectible animated cats.

Fat Cat Pomodoro is a native Haiku application with focus, short-break, and
long-break phases, persistent timer state, a live Deskbar control, and animated
cat break scenes. It is a `BApplication` with a Deskbar add-on; no Qt, QML,
Electron or other framework is used.

During breaks, Fat Cat hides its settings window and places animated cats over
a snapshot of the selected screen. Completing breaks unlocks new cats, while
renaming and favourites let you make the sanctuary your own.

---

This project is a native Haiku recreation of
[Fat Cat Pomodoro](https://github.com/jeremielumandong/omarchy-fat-cat). The
bundled sprite sheets come from the MIT-licensed upstream project, whose
copyright notice is retained in [LICENSE](LICENSE).

**The goal is simple:** to make focus sessions productive and breaks delightful
on Haiku.

> Sikosis · **Designer and maintainer** · [GitHub](https://github.com/sikosis)

---

## Features

- Focus, short-break, and long-break phases. Defaults are 25 minutes focus, 5 minutes short break, 15 minutes long break, and one long break after every 4 completed focus sessions.
- Intense, Chill, Defaults, and Custom timer profiles.
- A live Deskbar readout with the Fat Cat HVIF icon and responsive timer
  controls. A green dot means the background service is responding; a grey
  **Off** state means it stopped without a clean quit.
- A background application that keeps working when its settings window closes.
- Persistent, suspend-aware timers. Paused timers, interval progress, settings,
  collection progress, names, and favourites survive restarts.
- Four cats with distinct personalities and six activities: walk, stretch,
  groom, yawn, loaf, and sleep.
- Cat unlocks that match the original: Mochi immediately, Miso after 1 completed
  break, Patches after 3, and Pepper after 6.
- Cat renaming, favourites, screen selection, reduced motion, and a no-streak
  collection model.
- Full-screen break scenes with a compact control panel near the Deskbar.
  Escape and **Skip break** dismiss a break; previews and skipped breaks never
  earn progress.
- Bounded clock-change handling, atomic state writes, and predictable recovery
  after sleep or an overdue interval.
- An original cat-and-Pomodoro HVIF icon embedded in the application, Deskbar
  add-on, and native About box.

---

## Build

Install the Haiku development tools, then run from the repository root:

```sh
make
make test
make install
```

The install target places:

- `fatcat` and its sprites in `~/config/non-packaged/apps/FatCat/`;
- `fatcat-cli` in `~/config/non-packaged/bin/`;
- `FatCatDeskbar.so` in `~/config/non-packaged/add-ons/deskbar/`.

> [!IMPORTANT]
> If the Deskbar item does not appear immediately, run `quit Deskbar`. Haiku
> restarts Deskbar automatically; relaunch Fat Cat afterwards to reinstall its
> item. You can open the settings window directly with
> `~/config/non-packaged/apps/FatCat/fatcat`.

---

## Package

HaikuPorts is the intended release format and produces a standard installable
`.hpkg`. On Haiku, run:

```sh
scripts/build-haiku-package.sh
```

The script creates a local source archive and checksummed recipe in `dist/`,
copies the recipe to `haiku-apps/fatcat` in the configured HaikuPorts tree, and
invokes HaikuPorter. It reads `TREE_PATH` from
`~/config/settings/haikuports.conf`, searches common checkout locations, or
accepts an explicit tree:

```sh
HAIKUPORTS_TREE=/boot/home/haikuports scripts/build-haiku-package.sh
```

If HaikuPorts or HaikuPorter is missing, the script offers to clone the official
repository. Pass `--bootstrap` to accept that setup without prompts. Extra
arguments after the script options are passed to HaikuPorter. Use
`--prepare-only` to generate and validate the archive and recipe without
building a package.

The versioned recipe template is
[`packaging/haikuports/fatcat-0.17.recipe.in`](packaging/haikuports/fatcat-0.17.recipe.in).

After pushing a release tag, prepare and submit its public HaikuPorts recipe:

```sh
make haikuports-submit VERSION=0.17
```

The submission script downloads and verifies the tagged source archive, builds
the public recipe in an isolated HaikuPorts checkout, creates or reuses your
fork, pushes a feature branch, opens the upstream pull request, creates the
GitHub release when needed, and uploads the validated `.hpkg`. It never
force-pushes or replaces an existing release asset.

The script securely prompts for a GitHub personal access token when
`GITHUB_TOKEN` is not already set. A classic token needs the `public_repo`
scope. Token input remains hidden and is confirmed using only its final four
characters.

To inspect the public recipe without building, pushing, or publishing anything:

```sh
scripts/submit-haikuports.sh --prepare-only 0.17
```

The resulting package installs the application, sprites, command-line client,
Deskbar add-on, documentation, and application-menu entry. `make install`
remains available for development builds outside package management.

---

## Using Fat Cat

The **Timer** tab starts, pauses, resumes, and stops the timer; previews the
sanctuary; and configures intervals, display, blocking, and motion. Choosing
Intense, Chill, or Defaults from the Profile menu immediately saves its timer
values. Custom preserves manually entered values for **Save settings**.
Interval edits affect the next phase rather than the current phase.

Closing the settings window only hides it, so Fat Cat continues running with
its Deskbar item. Primary-click the Deskbar item to reveal Fat Cat and
secondary-click it for timer controls. **Quit Fat Cat** exits the application
and removes the Deskbar item; the next launch reinstalls it.

The **Cats** tab shows collection progress. Names save when Enter is pressed or
the field loses focus. If any unlocked cats are favourited, only favourites
visit during real breaks; previews always show all four cats.

Breaks and previews cover the selected screen with a snapshot-backed scene, so
the cats appear to walk over the desktop without requiring window transparency.
The control panel stays near the top-right Deskbar area. **Keep break overlay in
front** makes real breaks modal; when disabled, other applications can be
activated normally. Haiku system shortcuts remain available in either mode.

Settings and timer state live in `~/config/settings/FatCat/`. Uninstalling keeps
this directory so progress returns after reinstalling:

```sh
make uninstall
```

---

## Command line

`fatcat-cli` provides the same control surface as the original plugin IPC and
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

---

## Project layout

- `src/FatCatApp.*` — application lifecycle, IPC, persistence, and phase
  transitions
- `src/DeskbarView.*` — archived Deskbar replicant and live timer display
- `src/Session.*` — deterministic timer state machine
- `src/MainWindow.*` — native settings and collection UI
- `src/BreakWindow.*`, `src/CatView.*` — break windows and animated sanctuary
- `artwork/fatcat-icon.svg`, `artwork/fatcat-icon.hvif` — editable artwork and
  embedded Haiku vector icon
- `tests/session_test.cpp` — transition, reward, restore, and clock-adjustment
  coverage

Bug reports and feature requests are welcome in the
[Fat Cat issue tracker](https://github.com/sikosis/fatcat/issues).

---

## License

Fat Cat Pomodoro is free software licensed under the [MIT License](LICENSE).
