# Component Contracts

## Purpose

Milestone 5 formalizes a small but important rule set:

- system surfaces should be declared through framework helpers instead of ad hoc frame flag mutation
- app surfaces should declare app-level privilege through framework helpers
- widget customization should prefer explicit component APIs over direct struct field mutation when the setting is part of the intended public contract

## Current Framework Surface Rules

### App surfaces

- Use `configureFrameAsAppSurface(frameId, isApp)` to mark top-level app roots.
- App surfaces may participate in app-level navigation behavior such as home handling.
- A frame configured as an app surface is not treated as a system overlay.

### System surfaces

- Use `configureFrameAsSystemSurface(frameId, enabled)` for overlays such as dialogs and toast layers.
- System surfaces render above app content and should not pretend to be app roots.
- Dialog overlay frames and toast manager frames now use this shared helper instead of setting `Frames[frameId].isSystemLayer` directly in open-coded paths.

## Current Widget Contract Direction

### Buttons

- `setButtonCornerRadius()` is now the intended way to configure button shape.
- `setButtonColors()` is the intended way to update the button color triple after creation.
- Direct mutation of `Button` internals should be treated as framework-internal or legacy compatibility behavior, not the preferred public path.

### Cards

- `setCardCornerRadius()` now controls card shape explicitly.
- `setCardContentInsets()` now controls the content container inset instead of relying on a hardcoded 12px margin in `Card`.
- Card content placement is derived from stored insets during `preRender`, which makes card spacing a framework contract instead of an implementation accident.

## Remaining Gaps

- Some widgets still expose raw structs with mutable fields because TensorUI is still in hardening mode.
- Surface declaration is clearer now, but publication-ready API review still needs one final pass across remaining widgets.
- Milestone 6 should turn these internal notes into a concise public-facing contract reference once naming and maintenance guidance settle.
