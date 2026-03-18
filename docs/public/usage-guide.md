# TensorUI Usage Guide

TensorUI is a lightweight C UI framework aimed at embedded-style interfaces and custom operating system surfaces.

## What TensorUI Is

TensorUI provides:

- a hardware abstraction layer
- a frame-based renderer
- a stack-based window manager
- layout containers such as `VStack` and `HStack`
- widgets such as buttons, labels, sliders, cards, dialogs, keyboard, canvas, and text fields

## Recommended Use

- single-purpose embedded interfaces
- internal device UI prototypes
- early custom OS shells and system surfaces

TensorUI is not positioned as a general desktop UI toolkit. It is optimized around constrained environments, explicit ownership, and small framework surface area.

## Core Concepts

- Frames are the basic render and interaction units.
- Widgets and containers allocate frames and implement rendering plus touch behavior.
- The window manager owns stacked root screens.
- The HAL isolates platform-specific behavior such as screen, time, input, and font access.
- Input is standardized around shared framework state instead of page-local gesture tuning.
- Focus and active text input are framework-owned concepts, not just page wiring.
- Safe-area and surface rules are intended to survive both round and rectangular displays.

## Build And Verify

```bash
make
make check
make run
```

`make check` is the current baseline verification step and should stay green as framework refactors continue.

## Documentation Map

- Repository overview: `README.md`
- Architecture notes: `docs/public/architecture.md`
- Local repair notes and milestone logs: `docs/local/`
