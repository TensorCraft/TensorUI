# Layout And Safe-Area Notes

## Purpose

Define the baseline layout rules that keep TensorUI usable across round and rectangular displays without page-by-page retuning.

## Current Framework Rules

### 1. Screen safe insets are framework-owned

`hal/screen` now exposes shared safe insets through `getScreenSafeInsets()`.

Current baseline:

- round display:
  - top: 20
  - right: 18
  - bottom: 24
  - left: 18
- rectangular display:
  - top: 12
  - right: 12
  - bottom: 16
  - left: 12

These values protect:

- round-edge readability
- left-edge back gesture space
- bottom home-gesture space
- a minimum visual breathing room for cards and fields

### 2. Content width should come from safe insets

Pages should derive their main content width from:

- `getScreenContentWidth(getScreenSafeInsets())`

This replaces scattered `SCREEN_WIDTH - 24`, `SCREEN_WIDTH - 40`, and similar page-local tuning.

### 3. Page padding should come from safe insets

Full-screen surfaces should prefer:

- `applyFramePaddingInsets(frameId, getScreenSafeInsets())`

This gives a shared top/bottom/left/right content envelope for app pages and system surfaces.

### 4. Stack containers now have explicit content-inset APIs

`VStack` and `HStack` now expose:

- `setVStackContentInsets(...)`
- `setHStackContentInsets(...)`

This allows container padding to be described as layout policy instead of indirect field mutation.

## Current Example Direction

The main demo screens now follow the shared layout baseline:

- settings
- about
- material
- editor
- paint
- home paging rows
- calculator

The goal is not to perfect the demo, but to validate that shared layout rules can replace page-local geometry tuning.

## Remaining Gaps

1. Measurement is still mostly frame-size-first
   - Containers still rely on child frame sizes rather than a richer measure/layout negotiation model.

2. Some widgets still assume fixed heights
   - Several controls remain easier to use with hand-picked heights than with intrinsic sizing.

3. Safe insets are static
   - Current safe-area values are framework constants, not device-reported geometry or runtime policy.

4. Mixed-height and multiline growth still need more validation
   - The new safe-area baseline reduces obvious clipping pressure, but it does not fully solve vertical density and content-growth rules.

## Milestone Alignment

Milestone 3 is complete when:

- the framework has shared safe-area primitives
- major example screens derive their main content geometry from those shared primitives
- layout heuristics are reduced enough that round vs rectangular behavior is not driven by page-specific magic numbers alone
