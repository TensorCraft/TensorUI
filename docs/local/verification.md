# Verification Baseline

## Purpose

Milestone 6 turns framework hardening into a repeatable maintenance baseline. The goal is not a large test harness yet, but a small verification routine that can be rerun after framework changes without depending on prior conversation context.

## Current Repeatable Checks

### 1. Build verification

Run:

```bash
make check
```

This confirms that the current framework, HAL, and demo integration surface still compile together into the SDL runtime target.

### 2. Manual smoke path

Run:

```bash
make run
```

Then sanity-check these behaviors:

- home screen appears and paging still works
- calculator opens and buttons still respond
- editor opens and text entry still flows through the shared text session
- dialogs and toast overlays still render above app content
- back and home gestures still behave according to the current framework rules

## Why This Is The Baseline

- The repo currently ships one main integration surface, so build integrity plus targeted smoke coverage catches most regressions that matter to framework evolution.
- This verification path is intentionally lightweight so it remains realistic to run after each hardening step.
- Deeper automated testing can come later, but milestone work should keep this baseline green at minimum.

## Maintenance Rule

When a milestone changes framework behavior, update:

- `docs/progress.md`
- `docs/local/development-log.md`
- this verification note if the expected check routine changes
