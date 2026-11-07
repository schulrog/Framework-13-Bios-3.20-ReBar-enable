# Framework 13 BIOS 3.20 ReBAR enable

EFI setup launchers and diagnostic tools developed for **Framework Laptop 13, 12th Gen Intel, BIOS 3.20**. They use the installed stock firmware; they do not flash a modified BIOS.

**Reported hardware result:** the owner opened `PCIE Resizable BAR Support` using `RebarPage.efi`, changed **Auto → Enabled**, saved through the stock setup browser, and reported GPU-Z showing ReBAR enabled with an RTX 3060 Ti Thunderbolt eGPU. This is one user-confirmed configuration, not a guarantee for other devices or BIOS versions.

## Tools and status

| Tool | Purpose | Status |
|---|---|---|
| `RebarPage.efi` | Open PCI Express Configuration directly | Earlier build confirmed by owner: page opened and ReBAR saved successfully. Packaged source has subsequently received validation/generalization changes; packaged binaries are rebuilt from that source. |
| `FullSetup.efi` | Normal setup, hidden Advanced, or combined menu request | **Experimental**. Software and virtual-firmware dispatch tests passed; physical rendering of combined tabs has not been confirmed. |
| `OpenSetup.efi` | Start the installed SetupUtilityApp without leaving the shell | Owner successfully reached setup for the live capture. |
| `HiiReadOnly.efi` | Export shell-visible HII packages | Produced a valid live export; setup forms were absent outside Setup. |
| `HiiWatch.efi` | Capture HII after setup forms are registered | Produced a valid setup-time capture including normal and hidden Advanced. |

These are unsigned x64 UEFI executables. **They are not Windows applications.** No bootable EFI shell, updater, BIOS image, or security bypass is included. Use your existing trusted EFI-shell USB. If firmware rejects a tool, stop and report the error.

## ReBAR

1. Copy `bin/RebarPage.efi` onto the EFI-shell USB.
2. Boot the shell and select the USB filesystem mapping containing it.
3. Run `RebarPage.efi` directly. Do **not** use `load`, and do not type `exit` first.
4. Confirm that **PCI Express Configuration → PCIE Resizable BAR Support** is the page shown.
5. To reproduce the reported change, change **Auto → Enabled**, leaving all other options alone, then save through the stock BIOS interface.
6. After Windows starts, verify both the BIOS requirement and overall ReBAR status in GPU-Z. A successful save alone does not prove active ReBAR.

The launcher does not save a value automatically. The save you request in the firmware browser is a persistent BIOS configuration change. Resetting defaults or updating firmware may require rechecking the setting. There is no need to run the launcher on every boot.

## Normal and hidden menus

Run `FullSetup.efi`, then choose:

- **1:** normal BIOS setup, unchanged.
- **2:** full hidden Advanced menu tree.
- **3:** combined request: original normal menus plus an additional **Advanced (Extended)** tree.
- **Esc:** return to the shell.

The combined mode preserves the original menu handles and their order, appends the hidden package, and temporarily renames its English titles. It does not flatten duplicate controls into one form. Framework's browser may still filter tabs or controls. **First hardware run: inspect and exit without saving.** Hidden hardware, memory, and power options can prevent normal boot; visibility does not mean a setting applies to this laptop.

## How it works

`OpenSetup` locates the installed SetupUtilityApp by firmware-file GUID and uses standard `LoadImage`/`StartImage`. The menu launchers temporarily redirect the public FormBrowser2 `SendForm` pointer while stock setup initializes. The wrapper selects an exact live HII package, restores the original function pointer **before entering the browser**, and forwards the page/menu request. Cleanup also restores the pointer if stock setup returns without calling it.

Combined mode updates only two labels in the live hidden HII package and restores them after the browser returns. Restarting clears these in-memory changes. The tools do not install a permanent menu, change boot entries, disable firmware locks, or call flash/SMM write services. The stock Setup application still runs its normal initialization and may perform variable housekeeping or restart.

The ReBAR setting in the inspected firmware/live forms is:

| Field | Value |
|---|---|
| Variable | `SaSetup` |
| Variable GUID | `72C5E28C-7783-43A1-8767-FAD73FCCAFA4` |
| Byte offset / size | `0x42B` / 1 byte |
| Values | `0` Disabled, `1` Enabled, `2` Auto |
| Form | `0x159A` |
| Normal / hidden question IDs | `0x448` / `0x7CB` |

The firmware explicitly requests a lock on `SaSetup`. Ordinary shell writes were rejected, but the owner's save through stock setup succeeded. The tools do not disable that lock.

## Diagnostics

- `HiiReadOnly.efi` writes `HII-export.bin` to the application's drive.
- `load HiiWatch.efi`, followed by `OpenSetup.efi` **in the same boot**, can produce `HII-setup.bin` while Setup is active. Exiting the owner's shell rebooted the laptop, so that route did not preserve the observer.
- HiiWatch is a temporary boot-service driver. Restarting removes it. Both diagnostics refuse to overwrite their output file.
- HII exports describe forms/defaults; they are not proof of the live variable's current value. Treat your exports as private until reviewed.

## Build and test

Requires **Zig 0.13.0**, PowerShell, and Python 3 for the synthetic test-fixture generator. The compiler and Python are not bundled.

```powershell
./build.ps1 -Zig /path/to/zig
./test.ps1 -Zig /path/to/zig -Python /path/to/python
```

See [validation notes](docs/VALIDATION.md) and [tool instructions](docs). Synthetic tests do not require or distribute a BIOS dump. Emulator results described in the notes were obtained during development; the full firmware-import harness and proprietary input packages are not distributed.

## Contents

`src/` contains the five tools; `bin/` contains freshly built EFI binaries and checksums; `tests/` contains native regression checks; `tools/` generates synthetic fixtures. `docs/` contains the original tool instructions and an evidence-based validation summary.

Excluded: BIOS/update images, live HII dumps, disassembly, firmware-derived IFR dumps, screenshots, personal files, downloaded compilers, QEMU binaries, and intermediate build data. This is the complete **shareable source project**, not the original scratch workspace.

Independent community experiment; not an official Framework utility. MIT license applies to the original source in this repository. See [LICENSE](LICENSE).
