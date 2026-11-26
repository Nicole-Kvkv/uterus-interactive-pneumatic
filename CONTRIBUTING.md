# Contributing

This repository documents an experimental art–technology installation. Contributions are welcome when they improve **clarity, reproducibility, correctness or safety**.

## Before submitting a change

- Explain the proposed changes and their effect on hardware behavior in an issue or pull request.
- Keep **Master ↔ Slave protocol** compatible, or update both endpoints and `docs/PROTOCOL.md` together.
- Update pin mappings, interaction timing, and the 180-second score docs whenever related code changes.
- Test changes **with pump power disconnected** before any low-pressure hardware checks.
- Do not submit credentials, personal information, third-party medical/cadaveric imagery, or artwork without verifiable publication rights.
- Avoid claiming that software-only timers or touch gestures provide pressure safety.

The project currently has no automated hardware-in-the-loop validation. Record explicitly what was and was not tested.
