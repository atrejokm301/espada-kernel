# Espada Kernel — credits

Espada Kernel for the Pixel 11 family (spacecraft) is built on the work of others:

- **Sultan Alsawaf** (<sultan@kerneltoast.com>, github.com/kerneltoast) — the Capacity
  Aware Superset Scheduler (CASS), the Simple Low Memory Killer, and the scheduler /
  schedutil performance-efficiency series. Imported from his 16.0.0-sultan tree; the
  commits carrying his code are authored by him in this history.
- **The Android common kernel (ACK) and upstream Linux developers** — the android16-6.12
  base and the cherry-picked upstream fixes, which keep their original authors.
- **Google** — the Pixel 11 GPL kernel source drop (CD1A.260714.001.A9) this tree starts from.

Espada-specific changes (grizzly tuning of CASS, the 6.12/MGLRU port of Simple LMK,
the measured energy model, the util_est fixes, devfreq and build changes) are
maintained by Kevin Trejo.
