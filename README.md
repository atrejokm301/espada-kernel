# Espada Kernel

A kernel for the Pixel 11 family (Google "spacecraft": cubs, grizzly, kodiak and the `_2`
variants). It starts from Google's Pixel 11 kernel source and sits on Google's
android16-6.12-lts branch (Linux 6.12.96 since v3.5). The vendor modules in the zips come
from Google's October 2026 source drop (CD1A.261005.003). Tested on the Pixel 11 Pro (grizzly).

Branch `espada` is the kernel with KernelSU-Next and SuSFS built in. Branch `espada-magisk`
is the same kernel without them. Flashable zips are on the releases page and in the XDA thread.

## What it changes

- Sultan Alsawaf's CASS scheduler, ported to 6.12 and tuned for this SoC
- Simple LMK (Sultan) ported to 6.12 / MGLRU; lmkd falls back to the in-kernel path
- Fix for a util_est leak in the vendor scheduler hooks
- Measured per-cluster energy model instead of the stock power == freq placeholder
- Prime core capped at 3571 MHz (see the XDA post for the measurements) and its DVFS
  headroom clamped so it stops spiking to max on every wakeup
- GPU devfreq polling floored at 40 ms
- android16-6.12-lts 6.12.96 base plus fixes picked from newer 6.12 stable and mainline
- Per-app uname spoof (octobot): Play Integrity, Messages and Wallet see the stock
  kernel version, everything else sees the real one
- USB OTG in recovery (asdfmonster261's tcpm patch)
- AutoFDO + MLGO build flags; no LTO on purpose

## Building

Standard Bazel/Kleaf GKI build for the spacecraft target from source
(`--nouse_prebuilt_kernel`, `--lto=none`). The device is 64-bit only; the kernel is
flashed as a boot + vendor_kernel_boot pair and the two must always match.

## License

GPL-2.0 (see `COPYING` and `LICENSE`). The original ACK README is kept as `README-ACK.md`.

## Credits

See `CREDITS.md`. Sultan's code is committed under his own authorship in this history.
