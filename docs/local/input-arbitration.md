# Input Arbitration

## Purpose

Define how TensorUI decides whether a touch sequence belongs to:

- a system back swipe
- a system home swipe
- a scroll container
- a normal tap
- a cancelled interaction

This is the first major framework-hardening area because implicit gesture ownership causes unstable behavior across apps.

## Current Rules

The current implementation now centers around a shared `InputSession` in `hal/screen`, which tracks one pointer sequence from press to release:

- press origin
- current pointer position
- click candidate frame
- current owner frame
- locked gesture intent
- drag/timing state

### On press

- Record the initial touch position and timestamp.
- Start a fresh shared input session.
- Allow immediate `onTouch(true)` only for direct-touch widgets with `scrollType == 0`.
- Do not let scroll containers capture the touch on initial press.

### When motion exceeds drag threshold

The gesture is locked into one intent:

1. `GESTURE_SYSTEM_BACK`
   Condition:
   - starts within a modest left-edge zone
   - moves right past a small lock threshold
   - horizontal movement still clearly dominates, so ordinary vertical scrolls do not get stolen

2. `GESTURE_SYSTEM_HOME`
   Condition:
   - starts within a modest bottom-edge zone
   - moves upward past a small lock threshold
   - vertical movement is clearly stronger than horizontal drift

3. `GESTURE_SCROLL_HORIZONTAL` or `GESTURE_SCROLL_VERTICAL`
   Condition:
   - not claimed by a system gesture
   - a matching scroll container is found under the original press point

4. `GESTURE_CANCEL_CLICK`
   Condition:
   - drag started, but no system gesture or matching scroll container owns it

### On release

- Home navigation is only allowed if the drag intent was already locked as `GESTURE_SYSTEM_HOME`.
- Home completion is intentionally more tolerant than lock acquisition: a gesture may finish through either a reasonably fast flick or a clearly long enough upward travel.
- Scroll containers receive `onTouch(false)` only if they had actually captured the drag.
- Taps only fire when no significant movement happened.

## Why This Matters

This makes the framework more deterministic:

- edge gestures stop competing as much with content scrolling
- edge gestures remain deliberate, but no longer require such narrow, unforgiving emulator-perfect flicks
- scroll containers stop stealing taps before drag intent is known
- the active pointer sequence now has a single shared owner/context instead of loose globals
- gesture behavior becomes documentable instead of trial-and-error

## Next Improvements

1. Continue retiring legacy compatibility globals in favor of `InputSession`.
2. Expose framework-level helpers for pointer ownership queries instead of open-coded frame scans.
3. Document which gestures are framework-level and which are app-level.
4. Connect future focus/text session work to this same input ownership model.
