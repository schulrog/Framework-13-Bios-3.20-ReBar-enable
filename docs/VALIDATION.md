# Validation and scope

## Physical laptop evidence (owner-reported)

- Framework Laptop 13, 12th Gen Intel, BIOS 3.20; RTX 3060 Ti Thunderbolt eGPU.
- GPU-Z initially reported ReBAR disabled with the BIOS-enabled requirement failing; other listed requirements passed.
- An ordinary shell variable write had returned WRITE_PROTECTED.
- Shell HII export contained 62 package lists, without the Advanced setup forms.
- Exiting the shell restarted the computer, removing a temporary observation driver.
- The stock-application launcher enabled a setup-time capture: 72 package lists with normal and hidden Advanced.
- RebarPage opened PCI Express Configuration. The owner photographed the ReBAR control at Auto, changed it to Enabled, saved through stock setup, and reported GPU-Z enabled afterward.
- No successful physical combined-menu test was reported before this repository was packaged.

These statements preserve the evidence level: the final GPU-Z state was reported by the owner, not independently recaptured by an automated test. No firmware flashing was performed.

## Local regression checks

`test.ps1` uses synthetic IFR packages, not extracted firmware. It exercises diagnostic export/cleanup, overwrite refusal, partial writes, flush failure, reentrancy, malformed/truncated input, bounded polling, firmware-file path construction, normal-vs-hidden matching, unique handle selection, wrapper restoration, original argument preservation, menu choices, combined handle ordering, label restoration, and rollback after a label failure.

Synthetic fixtures validate code behavior, not arbitrary vendor-form compatibility. Current sources are rebuilt for the repository; a new build is not the exact historical binary the owner tested.

## Development emulator tests

QEMU with EDK2 firmware was used during development. A diagnostic driver captured newly registered forms to a FAT16 disk image; the saved export was validated after shutdown. A virtual-folder filesystem test produced truncated output and was rejected; the final diagnostic explicitly flushes its file.

Firmware-file application loading was tested with the emulator's installed shell. Browser-dispatch tests imported copies of the locally captured form packages, removing only original-machine device-path packages (and changing a package-list GUID where needed to import both). Real HII database/string services plus a test browser sink verified handle selection, page IDs, merged handle ordering, temporary labels, and restoration.

Those emulator tests did **not** render or save through Framework's H2O browser. The firmware-derived fixtures, full import harness inputs, emulator binaries, and machine dumps are intentionally not distributed. The native regression tests are self-contained and rerunnable.

## Restrictions

The hardcoded identifiers were investigated for this BIOS only. There is no support claim for other Framework models, generations, firmware releases, or languages. A browser may filter the additional tab or deny saving. The launchers don't remove access rules, hardware checks, BIOS Lock, FPRR, or Boot Guard.

The combined mode changes two live English menu labels and restores them on return. It does not merge question identifiers or flatten all settings into a single form. A restart clears session changes, while any settings the user saves in stock setup remain real configuration changes.
