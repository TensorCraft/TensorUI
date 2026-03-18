# TensorUI

A lightweight C UI framework for embedded-style operating system interfaces.

![TensorOS Demo](assets/demo.gif)

## Overview

TensorUI is being hardened into a durable UI framework for a self-built embedded OS. The current codebase already provides a working frame renderer, window stack, layout containers, text input controls, and a small widget set, with SDL2 used as the current desktop runtime.

The project goal is not to preserve a flashy demo, but to establish clear framework contracts around:

- input and gesture ownership
- focus and text input session behavior
- layout and safe-area behavior
- rendering and invalidation rules
- widget and system-surface contracts

## Current Capabilities

- Frame-based rendering and composition
- Stack-based window manager
- Shared input session and gesture arbitration baseline
- Framework-owned focus and active text-input session baseline
- Layout containers such as `VStack` and `HStack`
- Common widgets including buttons, labels, sliders, cards, dialogs, keyboard, canvas, and text fields
- Safe-area helpers for round and rectangular displays

## Repository Layout

- `TensorUI/`: framework widgets, containers, and window management
- `hal/`: hardware abstraction for screen, input, memory, time, math, and fonts
- `examples/`: current SDL demo app used as a framework integration surface
- `docs/public/`: GitHub-suitable public framework docs
- `docs/local/`: local audit notes, repair logs, and milestone planning
- `tools/`: asset and font utilities

## Build

### Requirements

- C compiler with C99 support
- SDL2 development libraries

On macOS with Homebrew:

```bash
brew install sdl2
```

### Commands

```bash
make
make check
make run
```

`make check` is the current repeatable verification baseline and confirms that the framework plus demo integration surface still compile together.

## Documentation

- Usage guide: [docs/public/usage-guide.md](docs/public/usage-guide.md)
- Architecture notes: [docs/public/architecture.md](docs/public/architecture.md)
- Documentation index: [docs/README.md](docs/README.md)

## Status

TensorUI has completed its first framework hardening pass through input, focus, layout, rendering, and component contract cleanup. The next priority is strengthening verification and publication quality so the repository stands on its own without local conversation context.

## License

See [fonts/SIL Open Font License.txt](fonts/SIL%20Open%20Font%20License.txt) for font licensing details. Other project licensing should be clarified before external publication.
