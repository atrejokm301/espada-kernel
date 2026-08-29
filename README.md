# Espada Kernel

A kernel for the Pixel 11 family (Google "spacecraft": cubs, grizzly, kodiak and the `_2`
variants), built from Google's GPL source drop for build CD1A.260714.001.A9 and rebased onto
ACK android16-6.12 LTS. Tested on the Pixel 11 Pro (grizzly).

Branch `espada` is the kernel. Flashable zips are posted in the XDA thread.

## What it changes

- Sultan Alsawaf's CASS scheduler, ported to 6.12 and tuned for this SoC
- Simple LMK (Sultan) ported to 6.12 / MGLRU; lmkd falls back to the in-kernel path
- Fix for a util_est leak in the vendor scheduler hooks
- Measured per-cluster energy model instead of the stock power == freq placeholder
- Prime core capped at 3571 MHz (see the XDA post for the measurements) and its DVFS
  headroom clamped so it stops spiking to max on every wakeup
- GPU devfreq polling floored at 40 ms
- ACK 6.12.92 LTS base plus a few 6.12.y / 7.x scheduler and cpufreq backports
- AutoFDO + MLGO build flags; no LTO on purpose

## Building

Standard Bazel/Kleaf GKI build for the spacecraft target from source
(`--nouse_prebuilt_kernel`, `--lto=none`). The device is 64-bit only; the kernel is
flashed as a boot + vendor_kernel_boot pair and the two must always match.

## License

GPL-2.0 (see `COPYING` and `LICENSE`). The original ACK README is kept as `README-ACK.md`.

## Credits

See `CREDITS.md`. Sultan's code is committed under his own authorship in this history.
