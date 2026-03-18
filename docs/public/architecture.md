# TensorUI Architecture

## Layers

TensorUI is organized into three main layers:

- `hal/`: platform-facing services such as screen, input, time, memory, strings, and fonts
- `TensorUI/`: framework widgets, containers, text input, overlays, and window management
- `examples/`: current integration surface used to exercise the framework in SDL

## Core Runtime Model

### Frames

Frames are the basic units of rendering and interaction. A widget typically owns one or more frames and attaches render, touch, and lifecycle callbacks to them.

### Input

Pointer ownership is tracked through a shared `InputSession`. This lets gesture intent, click candidates, drag state, and active ownership move through a single framework abstraction instead of scattered global flags.

### Focus And Text

Framework focus and the active text-input target are owned by shared framework state. Editable controls such as `TextField` attach themselves to the current text session, and keyboard surfaces consume that session instead of relying on page-specific rebinding on the main path.

### Layout

`VStack` and `HStack` provide the current container baseline. Safe-area helpers in `hal/screen` are used to adapt content to round and rectangular displays without tuning every screen independently.

### Windows And System Surfaces

The window manager owns a stack of root surfaces. App roots and system overlays are separate surface roles:

- app surfaces represent top-level application roots
- system surfaces represent overlays such as dialogs and toast layers

This distinction keeps system UI responsibilities separate from normal app content.

## Current Direction

TensorUI is intentionally being shaped toward embedded OS use rather than a demo-only UI layer. The current priorities are:

- maintain explicit ownership models
- preserve a small and understandable API surface
- keep public documentation concise and durable
- support repeatable verification during continued refactors
