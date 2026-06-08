# PS2 Hardware Testing

## Prerequisites

- PS2 with FreeMCBoot installed
- Network adapter (for party/stream/ping tests) or loopback peer
- USB keyboard (x86 CD path) or DualShock (PS2 native path)
- Linux host with `ps2client` / `ps2link` for automated tests

## Automated pipeline

```bash
make all
make test-integration
make fmcb-package
PS2_IP=10.0.0.2 bash scripts/install_fmcb.sh
PS2_IP=10.0.0.2 bash tests/hardware/hw_test_ps2.sh
```

## Manual checklist

1. Boot FMCB → launch `BOOT/ASMOS.ELF`
2. Run `help`, `sysinfo`, `meminfo`, `dashboard`
3. Run `network`, `ping 127.0.0.1`
4. Run `game 1` (Snake)
5. Run `party create test`, `saves backup 0 TEST.SAV`
6. Run `stream start 10.0.0.1` (with PC receiver)

## QEMU (x86 dev path)

```bash
make ps2-test
make run
```
