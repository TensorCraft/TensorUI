# Render And Invalidation Notes

## Purpose

Define the current rendering contract in TensorUI and make redraw cost easier to reason about while the framework is still using a full-frame compositor.

## Current Render Model

TensorUI currently uses:

- frame-local `preRender` hooks
- optional root buffering for window roots
- a screen compositor pass when `renderFlag` is set

This is not yet a dirty-rect renderer. The framework still redraws full composed surfaces when a render is requested.

## Current Invalidation Contract

### 1. `renderFlag` means a composed frame should be presented

Visual state changes should set `renderFlag = true`.

Examples:

- gesture and touch movement
- tween progress
- text edits and caret changes
- toast appearance and expiry
- canvas or slider updates

### 2. `continuousRender` means a frame still needs ticking even when the rest of the UI is idle

Frames now opt into continuous `preRender` work only when necessary.

This is used for cases such as:

- active drag tracking
- focused text-field caret / placeholder motion
- toast queue timing

Default behavior is now:

- do not run `preRender` every frame for every enabled frame
- run `preRender` every frame only for frames that declare `continuousRender`

### 3. Window-root buffering remains the main coarse optimization

Window roots can render into their own buffers and then be composited to screen.

This keeps:

- stacked windows easier to animate
- paused windows from doing unnecessary foreground work

## What Improved In This Pass

1. Pre-render work is no longer unconditional
   - idle frames without `continuousRender` no longer run `preRender` every loop

2. Long-lived ticking cases are explicit
   - toast manager
   - drag-driven stacks
   - drag-driven sliders and canvas
   - focused/animated text fields

3. Rendering behavior is easier to document
   - there is now a clearer distinction between:
     - needing a render
     - needing per-frame ticking

## Remaining Gaps

1. Composition is still full-surface
   - There is no dirty-region tracking yet.

2. Buffer invalidation is still coarse
   - Window-root buffers are rebuilt wholesale when rendering happens.

3. Some components still communicate redraw need through direct `renderFlag` mutation
   - That is acceptable for now, but a cleaner invalidation API would be better later.

4. Runtime cost is still platform-hidden
   - SDL makes the current path workable, but an embedded target will make full-surface redraw cost more visible.

## Milestone Alignment

Milestone 4 is complete when:

- the current invalidation model is explicitly documented
- frames that genuinely need continuous ticking declare it explicitly
- idle UI avoids unnecessary `preRender` work
- the remaining redraw-cost limits are clear enough to guide the next renderer changes
