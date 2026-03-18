# Text And Focus Audit

## Purpose

Define what is missing before TensorUI can treat text editing and focus as framework responsibilities instead of page-specific wiring.

## Current State

### What exists

- `TextField` now supports editable text, cursor movement, selection, clipboard-like copy/cut/paste, placeholder rendering, and multiline layout.
- `Keyboard` can target a `TextField` directly and insert or delete text through framework APIs.
- The demo editor already uses shared buffer editing instead of drawing text manually.

### What is still framework-fragile

1. The focus model is still only first-pass
   - The framework now tracks focused frame state, but it is still a minimal contract rather than a fully defined focus manager.
   - Focus transfer, restoration, and eligibility outside editable text controls still need clearer rules.

2. Text input attachment is only partially framework-owned
   - The shared text session now exists and drives the main path.
   - `Keyboard` still retains a compatibility `TextField *targetField` path.
   - Opening or closing the keyboard is still page logic rather than a full input-service decision.

3. Editing enablement and focus are conflated
   - `setTextFieldEditingEnabled()` currently means "show caret and accept editing gestures".
   - That is not the same as "this field is currently focused".
   - Long-term, editability should be a capability, while focus should be runtime state.

4. Pointer editing behavior is still local to `TextField`
   - Long press, caret placement, and drag-based selection are implemented inside the widget.
   - That is reasonable for widget-local editing behavior, but the surrounding focus/session rules are not yet defined.

5. Window transitions do not own text session cleanup
   - Pushing or popping pages does not automatically detach focused text targets or dismiss the keyboard layer.
   - Current behavior depends on page structure being simple and cooperative.

## Immediate Refactor Direction

1. Introduce a framework-level focus model
   - Define which frame can become focused.
   - Track current focused frame separately from editability.
   - Make focus change explicit on pointer press, window push/pop, and close.

2. Introduce a text input session
   - Replace raw `Keyboard -> TextField*` ownership with a shared text-input target contract.
   - Let keyboard UI attach to the active text session instead of a page-selected pointer.

3. Split capability from state
   - Keep "editable" as a control capability.
   - Add "focused" and "text input attached" as runtime state managed by the framework.

4. Define lifecycle rules
   - Focus should clear or transfer predictably when a window is paused, resumed, or destroyed.
   - Keyboard ownership should follow the active text session rather than surviving accidentally.

## Proposed First Delivery

- Add a small focus manager or shared focus state in the framework core.
- Teach `TextField` to request focus on pointer interaction instead of relying on page setup alone.
- Make `Keyboard` query the active text target from framework state.
- Document which parts stay widget-local and which become framework services.

## First Delivery Status

- Completed.
- The framework now has shared focused-frame and active text-target state in `hal/screen`.
- `TextField` can attach itself to that shared text session on interaction.
- `Keyboard` can consume the shared text session instead of depending only on manual `TextField *` wiring.
- Window transitions now clear focus/text session state instead of leaving it attached implicitly.
- The main editor flow no longer needs page-level keyboard rebinding.

## Remaining Gaps After This Pass

- Focus is still frame-level state, not yet a richer focus manager with eligibility rules.
- Widgets other than `TextField` do not participate in focus yet.
- The shared text target currently uses callback registration, but there is not yet a formal input-service layer or system keyboard policy.
- Resume/restore behavior exists for common window and dialog flows, but more complex nested system-surface policies are still future work.

## Current Contract

The framework now follows these rules:

1. Editable text controls
   - A `TextField` becomes focusable when editing is enabled.
   - Pointer interaction can focus the field and attach it as the active text-input target.

2. Pointer presses
   - Pressing the active text field keeps the current text session.
   - Pressing a frame marked `preservesTextInput` keeps the current text session.
   - Pressing any other frame clears the active text session.
   - If the new frame is not focusable, framework focus clears as well.

3. Window transitions
   - Pushing a window saves the previous top window's focus and text session before pausing it.
   - Popping a window restores the resumed window's saved focus/text session when still valid.
   - Popping to home restores the home window's saved focus/text session when still valid.

4. Modal dialogs
   - Showing a dialog clears active focus/text session for the covered surface.
   - Closing the dialog restores the previously active focus/text session when still valid.

5. Destruction
   - Destroying the focused frame or active text target clears the related runtime state automatically.

## Milestone Alignment

This work now maps to:

- Milestone 2: text, focus, and editing contract

Milestone 2 is complete when:

- focus movement is explainable by framework rules in common cases
- text input no longer depends on page-specific rebinding in normal flows
- text session cleanup and restoration rules exist for window transitions and modal overlays
- the control contract is clearer than `setTextFieldEditingEnabled()` alone

Milestone 2 status:

- Complete
