# Host-side checks

These checks run on a regular computer using a **mock Arduino runtime**, and require `g++` and Python 3. They are intended to catch syntax errors and simple regressions in gesture and I²C packet logic **without energizing any pumps**.

Run from the repository root:

```bash
bash tests/run_tests.sh
```

**These are host-model checks, not a real Arduino Uno firmware build.** Arduino IDE/AVR core compilation and actual pin, board, electrical, I²C and pneumatic behavior must still be verified on the intended hardware with the external motor supply disconnected first.
