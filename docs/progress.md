# Progress

Last updated: 2026-03-17

## Current Phase

Phase 1: framework hardening for embedded OS use

Current milestone: Post-milestone maintenance, verification discipline, and future optimization

## Active Priorities

1. Build a durable documentation workflow.
2. Stabilize input and gesture arbitration in the framework core.
3. Define the next refactor targets after input is less fragile.

## Completed In This Pass

- Created a dedicated documentation workspace under `docs/`.
- Added a roadmap, framework audit, development log, and a public usage guide draft.
- Started phase 1 framework repair on input arbitration.
- Changed input routing so scroll containers no longer capture touch on initial press; they now wait for drag intent to be established.
- Added an explicit gesture-intent locking step so back-swipe, home-swipe, scroll, and cancelled drags are separated by framework rules.
- Promoted gesture intent into a shared framework input model via `GestureIntent`, and updated the window manager to consume that shared state instead of recomputing edge-gesture rules.
- Consolidated pointer-sequence ownership into a shared `InputSession`, so origin, live position, click candidate, active owner frame, drag state, and gesture intent now travel together.
- Updated framework consumers to read the shared input session first, keeping old touch globals only as compatibility shims.
- Added a dedicated text/focus audit to define the next refactor stage after gesture cleanup.
- Started the first framework-owned focus/text-input pass: shared focused-frame state, a shared active text target, `TextField` self-registration on interaction, and `Keyboard` consumption of the shared text session.
- Added first-pass focus lifecycle rules: focusable frames, text-session-preserving frames, focus/blur hooks, and demo validation by removing editor-page keyboard rebinding.
- Removed remaining framework consumer dependence on legacy gesture globals so `InputSession` is now the primary shared input path.
- Closed Milestone 1: framework-owned input and text session baseline.
- Added explicit text/focus lifecycle rules for pointer presses, window pause/resume, and modal dialog overlays.
- Added framework-side restoration of saved focus/text session when windows resume and when dialogs close.
- Added explicit `TextField` focus helpers so runtime focus is no longer only implied by editability.
- Closed Milestone 2: text, focus, and editing contract.
- Added shared safe-area primitives in `hal/screen` and shared content-inset APIs in stack containers.
- Moved major example screens and calculator layout widths/paddings onto shared safe-area-derived geometry.
- Added internal layout/safe-area notes documenting the new layout baseline and remaining gaps.
- Closed Milestone 3: layout and safe-area system.
- Added explicit render/invalidation notes documenting the current full-surface render contract and its limits.
- Changed frame lifecycle so `preRender` only runs continuously for frames that explicitly opt into `continuousRender`.
- Marked drag-driven and time-driven components such as stacks, text fields, slider, canvas, and toast manager as continuous only when needed.
- Closed Milestone 4: rendering, invalidation, and runtime cost.
- Added shared helpers for declaring app surfaces and system surfaces instead of open-coded frame flag mutation.
- Added explicit component configuration APIs for button corner radius/color updates and card content inset/corner radius updates.
- Moved dialog and toast system layers plus main demo button-shape usage onto those explicit framework contracts.
- Added internal component-contract notes documenting the current surface and widget API direction.
- Closed Milestone 5: component contract cleanup and system surfaces.
- Added a repeatable `make check` verification entry point and documented the current smoke-check baseline.
- Rewrote the repository and public documentation entry points so they describe TensorUI as an embedded-OS-oriented framework instead of a demo-first project.
- Added a public architecture note and a local verification note to reduce dependence on prior conversation context.
- Closed Milestone 6: verification, publication, and OS-ready baseline.
- Audited the SDL demo against the hardened framework contracts and fixed remaining mismatches in app-surface declaration, continuous rendering, safe-area usage, and text-field click wiring.
- Fixed a post-milestone demo regression where app-page safe-area padding was accidentally applied twice through both container padding and child left margins, causing a framework-wide blank strip on the left.
- Hardened the SDL simulation layer so mouse drags survive leaving the window, releasing outside the window, and re-entering while still pressed.
- Relaxed back and home gesture completion thresholds so edge navigation is less brittle while still keeping lock acquisition stricter than normal scroll drags.
- Corrected the post-home fake-drag fix so it now preserves the original container input model and cancels lingering stack drag state through lifecycle pause handling instead of demo-local input surgery.
- Added a framework-level interaction cancellation path and moved window/home/dialog transitions onto it, so pointer/drag cleanup no longer depends on demo-local hook composition.

## In Progress

- Keep the new verification baseline green as deeper runtime and API work continues.
- Expand publication-quality docs only when framework contracts are stable enough to stay durable.
- Use milestone closure as the new baseline for future optimization and portability work.
- Keep the demo aligned with framework contracts so it exercises the real runtime model instead of legacy shortcuts.

## Next Up

1. Preserve the milestone baseline while continuing deeper runtime optimization.
2. Broaden framework contract cleanup across remaining widgets when needed.
3. Add stronger automated checks only when they can be maintained realistically.
