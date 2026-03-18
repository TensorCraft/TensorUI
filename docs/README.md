# Documentation

This folder tracks the work needed to move TensorUI from demo-grade UI experiments to an embedded-OS-ready framework baseline.

Structure:

- `progress.md`: live status tracker for ongoing framework work.
- `local/`: local-only working notes, audit documents, and development records.
- `public/`: outward-facing documentation intended to be cleaned up and published to GitHub.

Public docs should stay concise and durable. Internal repair notes, milestone details, and audit material belong under `local/`.

Current hardening sequence:

1. Input and gesture arbitration
2. Text and focus system stability
3. Layout and safe-area behavior across round and rectangular displays
4. Rendering efficiency and ownership clarity
5. Component API cleanup and long-term maintainability
6. Verification, publication, and OS-ready baseline
