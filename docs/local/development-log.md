# Development Log

## 2026-03-17

### Documentation bootstrap

- Created `docs/` structure for local and public documentation.
- Added roadmap, framework audit, and progress tracking.

### Framework work started

- Chosen first hardening target: input and gesture arbitration.
- Initial direction: reduce premature touch capture by scroll containers so the input system behaves more predictably before drag intent is known.
- Implemented the first arbitration fix in `hal/screen/screen.c`: scroll containers no longer receive `onTouch(true)` during the initial press path, which reduces accidental event capture before drag direction is known.
- Added explicit gesture-intent locking in `hal/screen/screen.c`, separating system back, system home, container scroll, and cancelled drags into documented framework behavior.
- Promoted gesture intent into a shared public input concept in `hal/screen/screen.h` and updated `TensorUI/WindowManager.c` to consume `currentGestureIntent` instead of inferring system-back ownership from raw drag data alone.

### Input session consolidation

- Added `InputSession` in `hal/screen/screen.h` to group active pointer-sequence state instead of scattering ownership across `startTouchX`, `startTouchY`, `dragFocus`, `isDragging`, and `currentGestureIntent`.
- Refactored `hal/screen/screen.c` press/move/release handling around the shared input session, including click candidate tracking and owner-frame assignment.
- Updated `TensorUI/WindowManager.c` to consume the shared input session for back-swipe progress and release decisions.
- Kept legacy globals as compatibility shims for now so the framework can migrate incrementally without destabilizing existing components.

### Text/focus audit opened

- Added `docs/local/text-focus-audit.md`.
- Captured the main architectural gap: text editing currently works through manual `Keyboard -> TextField` wiring, while framework-level focus ownership is still undefined.
- Defined the next refactor direction around a framework-owned focus model and text input session.

### Text/focus first pass started

- Added shared focused-frame state and a shared active text-input target in `hal/screen`.
- Updated `TextField` so editable fields can register themselves as the active text target on interaction instead of relying only on page wiring.
- Updated `Keyboard` to prefer the shared text session, keeping direct `TextField *` targeting only as compatibility behavior.
- Added basic window-transition cleanup so pushing or popping windows clears stale focus/text session state.

### Focus lifecycle and milestone framing

- Added first-pass focus lifecycle rules in the frame model: `focusable`, `preservesTextInput`, `onFocus`, and `onBlur`.
- Adjusted press handling so keyboard presses preserve the active text session instead of blurring it on every pointer-down.
- Removed editor demo rebinding of `Keyboard -> TextField` to validate that the shared framework text session now carries the main text-entry path.
- Defined Milestone A: framework-owned input and text session baseline.

### Milestone 1 closed and roadmap expanded

- Removed remaining framework consumer dependence on legacy gesture globals, leaving `InputSession` as the primary shared input abstraction.
- Rewrote `docs/local/roadmap.md` from a short repair list into a long-range milestone plan through the OS-ready baseline.
- Closed Milestone 1 and moved the active workstream to Milestone 2: text, focus, and editing contract.

### Milestone 2 closed

- Added explicit focus/text restoration behavior for resumed windows in `TensorUI/WindowManager.c`.
- Added modal dialog focus/text suspension and restoration rules in `TensorUI/Dialog/Dialog.c`.
- Added explicit `TextField` focus helpers so runtime focus attachment can be requested directly.
- Documented the resulting text/focus contract and closed Milestone 2.
- Moved the active workstream to Milestone 3: layout and safe-area system.

### Milestone 3 closed

- Added shared screen safe-inset helpers and frame-padding application helpers in `hal/screen`.
- Added explicit content-inset APIs for `VStack` and `HStack`.
- Reworked major example screens and the calculator to derive core widths and page padding from shared safe-area geometry instead of scattered width subtraction constants.
- Added `docs/local/layout-safe-area.md` to document the new layout baseline and the remaining gaps.
- Closed Milestone 3 and moved the active workstream to Milestone 4: rendering, invalidation, and runtime cost.

### Milestone 4 closed

- Added explicit `continuousRender` frame state so `preRender` no longer runs every frame for every enabled frame.
- Limited continuous ticking to frames that actually need it, including stacks during drag/fade, focused text fields, slider drag, canvas drag, and toast management.
- Added `docs/local/render-invalidation.md` to document the current full-surface render contract, invalidation model, and remaining runtime-cost limits.
- Closed Milestone 4 and moved the active workstream to Milestone 5: component contract cleanup and system surfaces.

### Milestone 5 closed

- Added shared frame-surface helpers in `hal/screen` so app roots and system overlays can be declared through framework APIs instead of direct flag mutation.
- Updated `Dialog` and `Toast` to use the shared system-surface helper and adjusted dialog card spacing through explicit card inset configuration.
- Added `setButtonCornerRadius()` and `setButtonColors()` so button shape and color updates have an intended public API.
- Added `setCardCornerRadius()` and `setCardContentInsets()` so card shape and content padding are no longer hardcoded implementation details.
- Updated the main demo and calculator paths to consume the new button/app-surface APIs instead of directly poking internal widget/frame fields where those settings are part of the framework contract.
- Added `docs/local/component-contracts.md` and closed Milestone 5.
- Moved the active workstream to Milestone 6: verification, publication, and OS-ready baseline.

### Milestone 6 closed

- Added a `make check` target so the current compile verification step is explicit and repeatable.
- Added `docs/local/verification.md` to document the current build and manual smoke baseline.
- Rewrote `README.md`, `docs/README.md`, and `docs/public/usage-guide.md` so the repo now presents TensorUI as an embedded-OS-oriented framework instead of a demo-first showcase.
- Added `docs/public/architecture.md` as a concise public description of layers, frames, input, focus/text ownership, layout, and system-surface roles.
- Closed Milestone 6 and established the current repository state as the OS-ready baseline for future work.

### Post-milestone demo alignment

- Audited `examples/TensorOS_Demo` against the new framework contracts instead of treating the demo as exempt glue code.
- Fixed the editor landing field to use `TextField`'s own click callback path instead of overwriting the frame callback after creation.
- Marked calculator, snake, and bounce roots as app surfaces so the demo app stack matches the framework's current surface-role model.
- Fixed snake and bounce to opt into `continuousRender`, which became required after the render/invalidation contract was tightened.
- Reworked snake and bounce play bounds to respect shared screen safe insets instead of assuming the full raw screen is always the playable area.
- Stopped recreating snake and bounce root frames on every launch so the demo no longer leaks new full-screen surfaces during repeated app opens.

### Safe-area regression fix

- Fixed a demo-wide left blank-strip regression caused by treating safe-area insets as both root container padding and child left margins at the same time.
- Normalized app-page child placement back onto the parent container's shared safe-area padding so content and edge gestures start at the correct visual boundary again.

### SDL simulation input fix

- Fixed an emulator-only mouse robustness gap in `hal/screen/screen.c` where drag sessions depended too heavily on in-window SDL mouse-up and motion events.
- Added SDL mouse capture during active drags and a global-button-state reconciliation pass so releasing outside the window now ends the session cleanly.
- Added motion reconciliation for the active pointer sequence so dragging outside the window and returning without releasing no longer leaves the framework in a stale partial-drag state.

### Edge gesture tuning

- Relaxed edge-gesture thresholds in `hal/screen/screen.c` so back and home no longer require such strict emulator-perfect flicks.
- Kept lock acquisition conservative enough to avoid obviously stealing ordinary `HStack` and `VStack` drags, but made completion more tolerant once the intent is already locked.
- Lowered the back-swipe completion threshold in `TensorUI/WindowManager.c` so successful edge drags are less likely to snap back after a clearly intentional back gesture.

### Drag cancellation correction

- Reverted the overly aggressive demo-local change that removed `onTouch` from passthrough home-page containers, because that damaged the existing input model and degraded home paging behavior.
- Moved the fake-drag cleanup back into framework behavior by teaching `VStack` and `HStack` to cancel active drag state on `onPause`, so lingering drag state is cleared during window transitions without changing container gesture semantics.

### Systemic interaction cancellation

- Added `cancelAllFrameInteractions()` in `hal/screen` as a framework-owned way to end active touch/drag state across currently enabled frames, reset the shared input session, and release SDL mouse capture.
- Updated `pushWindow()`, `popWindow()`, `popToHome()`, `showDialog()`, and `closeDialog()` to go through this shared cancellation path before changing surface visibility or ownership.
- Reverted the temporary home-page-specific drag cleanup back to simple scroll-position save/restore, because the transition-level cancellation path now owns this concern centrally.
