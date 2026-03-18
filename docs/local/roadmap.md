# TensorUI Framework Roadmap

## End Goal

Make TensorUI a lightweight but durable UI framework for a custom embedded operating system:

- framework behavior is rule-driven instead of demo-tuned
- input, focus, text, layout, and rendering have explicit ownership models
- screens survive round and rectangular hardware without screen-by-screen hacks
- public APIs read like stable framework contracts, not temporary experiment glue
- the repository is publishable and maintainable without hidden local knowledge

## Long-Term Product Shape

The finished TensorUI should provide:

- predictable pointer, gesture, and focus routing
- reusable system-grade text input controls and keyboard attachment rules
- safe layout behavior across mixed content, safe areas, and display shapes
- a documented render and invalidation model suitable for constrained hardware
- a stable widget and window contract for applications and system surfaces
- enough documentation and verification to evolve toward a real embedded OS UI runtime

## Milestones

### Milestone 1: Framework-Owned Input And Text Session Baseline

Status:
- Completed on 2026-03-17

Intent:
- Move pointer ownership, focus state, and active text target state into framework-owned primitives instead of page-specific wiring.

Delivered:
- Shared `InputSession` for press-to-release pointer ownership.
- Shared focused-frame and active text-target state in the framework core.
- `TextField` self-registration into the shared text session.
- `Keyboard` typing through the shared text session on the main path.
- First-pass focus/blur and text-session preservation rules.
- Removal of framework consumer dependence on legacy gesture globals.

Why it matters:
- TensorUI now has a real framework baseline for gesture ownership and text entry instead of treating them as demo-local behavior.

### Milestone 2: Text, Focus, And Editing Contract

Status:
- Completed on 2026-03-17

Intent:
- Turn the current first-pass focus/text session into a clear framework contract for editable controls and system keyboards.

Scope:
- Define focus transfer rules for taps, overlays, window pause/resume, and destruction.
- Separate editability, focus, selection, and text-input attachment into explicit framework state.
- Define how widgets declare that they can receive focus or text input.
- Remove remaining page-level text input assumptions from demo code and framework internals.
- Decide how keyboard surfaces attach to and detach from active text sessions.

Exit criteria:
- Focus movement is explainable by framework rules in the common cases.
- Text input no longer depends on page-specific rebinding in normal flows.
- Text session cleanup and restoration rules are documented and implemented for window transitions.
- The framework has a clearer contract for editable controls than `setTextFieldEditingEnabled()` alone.

### Milestone 3: Layout And Safe-Area System

Status:
- Completed on 2026-03-17

Intent:
- Replace layout-by-tuning with reusable sizing and safe-area rules that survive real device variation.

Scope:
- Audit fixed paddings, heights, and demo-shaped layout assumptions.
- Define safe-area handling for round displays, edge gestures, and system surfaces.
- Clarify container measurement behavior for mixed heights, multiline text, and nested scrolling.
- Reduce screen-specific positioning hacks in example content.

Exit criteria:
- Core screens remain usable across round and rectangular displays without per-screen retuning.
- Layout containers have clearer sizing expectations.
- Safe-area behavior is documented and reusable.

### Milestone 4: Rendering, Invalidation, And Runtime Cost

Status:
- Completed on 2026-03-17

Intent:
- Make rendering behavior and redraw cost explicit enough for embedded OS use.

Scope:
- Document buffer ownership and root-surface rendering rules.
- Clarify invalidation triggers and reduce unnecessary full redraws.
- Review animation, off-screen buffering, and redraw coupling.
- Identify where current SDL-side behavior hides future embedded runtime cost.

Exit criteria:
- The framework has a documented invalidation model.
- Buffer ownership and redraw responsibilities are explicit.
- Common interactions avoid avoidable full-screen work where practical.

### Milestone 5: Component Contract Cleanup And System Surfaces

Status:
- Completed on 2026-03-17

Intent:
- Stabilize the API surface so TensorUI components behave like durable framework building blocks.

Scope:
- Audit widgets for inconsistent lifecycle, sizing, input, and state contracts.
- Separate framework primitives from demo-specific helpers and styling shortcuts.
- Clarify window, overlay, dialog, and system-surface responsibilities.
- Tighten naming and behavior around public APIs before publication.

Exit criteria:
- Core widgets follow more consistent contracts.
- System surfaces are more clearly separated from app-level components.
- Public API cleanup reduces prototype leakage into framework usage.

### Milestone 6: Verification, Publication, And OS-Ready Baseline

Status:
- Completed on 2026-03-17

Intent:
- Turn the hardened framework into a publishable and maintainable baseline for ongoing OS work.

Scope:
- Expand internal docs into durable framework references.
- Keep public docs limited to publishable, future-GitHub-friendly content.
- Add enough regression checks, smoke coverage, and maintenance guidance to support continued refactors.
- Prepare the repository for external consumption without relying on prior conversation context.

Exit criteria:
- The repo communicates its architecture and usage clearly.
- Framework evolution is backed by documentation and repeatable verification.
- TensorUI is credible as the UI layer for an embedded self-made OS, not just a demo showcase.

## Current Order

1. Continue post-milestone maintenance and deeper optimization work as needed

## Definition Of Better

- A gesture conflict is explainable by framework rules, not trial and error.
- A widget has a clear lifecycle and ownership model.
- A screen survives shape, font, and content changes without custom hacks.
- Public APIs read like stable framework building blocks, not demo helpers.
