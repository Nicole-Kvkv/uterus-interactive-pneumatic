# The Five-Stage Breathing Score

This is a **programmed actuation score** derived from the current Master V17.4 `updateShow()` and `breathing()` functions. It describes commanded pump activity, **not measured physical bag volume, air pressure, airflow or tissue physiology**.

## Timing and method

- Performance duration: **180,000 ms (180 seconds)**.
- Base cycle: **5,000 ms**.
- Within a bag's cycle: first **inflation**, then **100 ms off**, then **suction**, then remaining cycle off.
- Offset phases differ between bags. PWM values are `255` for active pumps and `0` for off.
- A given bag's inflation and suction pumps are never intentionally enabled simultaneously.
- `G` disables the extra inflation **growth bias in Stage II**, not normal inflation in other intervals.

## Five stages

| Stage | Time (s) | Programmed behavior |
| --- | --- | --- |
| I — Fetal Breath Initiation | `0–5` | Bag 2 receives continuous inflation; Bags 1, 3, 4 have offset cycles |
| II — Uterine Expansion | `5–30` | Different bag-specific inflation/suction timings. Bags 2 and 4 receive extra inflation from 2.4–3.0 seconds of each local 5-second cycle when the growth bias is enabled |
| III — Uterine Wall Response | `30–60` | Four distinct timing-offset actuation sequences |
| IV — Fetal Breath Resonance | `60–170` | Repeated interleaved 5-second cycle for all four bags |
| V — Return to Stillness | `170–180` | Reduced durations of commanded pump-on intervals; all pumps stop at 180 s |

## Actuation schedule by bag

Each cell has the format **`phase offset / inflation duration / suction duration`** in milliseconds, relative to the stage start. The first 5 seconds contain a special Bag 2 instruction.

| Stage | Bag 1 | Bag 2 | Bag 3 | Bag 4 |
| --- | --- | --- | --- | --- |
| I: `0–5 s` | `0 / 1800 / 1800` | **Continuous inflation** | `1500 / 1600 / 1800` | `3000 / 1800 / 1800` |
| II: `5–30 s` | `0 / 2600 / 1400` | `1200 / 3000 / 1000` | `2500 / 2200 / 1700` | `3700 / 2800 / 1100` |
| III: `30–60 s` | `0 / 2100 / 1900` | `2500 / 2600 / 1400` | `1200 / 1800 / 2200` | `3700 / 2600 / 1400` |
| IV: `60–170 s` | `0 / 2200 / 1700` | `1250 / 2400 / 1500` | `2500 / 1900 / 2000` | `3750 / 2300 / 1600` |
| V-A: `170–175 s` | `0 / 1700 / 1700` | `1250 / 1800 / 1600` | `2500 / 1500 / 1800` | `3750 / 1700 / 1700` |
| V-B: `175–180 s` | `0 / 1000 / 1000` | `1250 / 1200 / 1000` | `2500 / 900 / 1100` | `3750 / 1100 / 1000` |

**Implementation nuance:** The extra growth-bias condition overrides the underlying Bag 2 / Bag 4 base cycle during the specified interval, rather than summing a numeric pressure signal.

## Suggested scientific figure caption

> **Programmed pneumatic actuation of the four-airbag installation.** The five-stage control schedule is based on the Arduino Master firmware and illustrates on/off pump commands, phased timing and the temporary Stage II growth bias. It does not represent measured air pressure, airflow, deformation, or bag volume.

## Important distinction

Shorter activation windows at the end of the programmed score do **not guarantee** actual deflation to zero, because the system contains no direct pressure or volume feedback. Visual or experimental claims require separate measurements.
