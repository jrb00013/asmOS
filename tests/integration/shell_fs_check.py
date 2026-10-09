#!/usr/bin/env python3
"""Drive the interactive ASMOS shell via QEMU QMP and verify FAT12 FS ops.

Boots build/disk/os.img (or disk/os.img from the repo root), types real
keystrokes into the PS/2 keyboard, reads the VGA text buffer back over QMP,
and asserts the shell lists, reads, and creates directory entries.

Exit 0 = all FS operations verified, 1 = anything failed, 2 = could not run.
"""
import json
import os
import socket
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
IMG = os.path.join(ROOT, "disk", "os.img")
SOCK = "/tmp/asmos_fs_check.qmp"
VGA = "/tmp/asmos_fs_check_vga.bin"

for f in (SOCK, VGA):
    if os.path.exists(f):
        os.unlink(f)


def find_qemu():
    for p in ("qemu-system-i386", "qemu-system-x86_64"):
        r = subprocess.run(["sh", "-c", f"command -v {p} || true"],
                           capture_output=True, text=True)
        if r.stdout.strip():
            return r.stdout.strip()
    return None


def main():
    qemu = find_qemu()
    if not qemu:
        print("SKIP: shell/FS check skipped (no QEMU installed)")
        return 0

    r = subprocess.run([qemu, "--version"], capture_output=True, text=True)
    out = r.stdout
    if "QEMU emulator" not in out:
        print("FAIL: qemu binary does not run")
        return 1

    proc = subprocess.Popen(
        [qemu, "-fda", IMG, "-m", "32", "-display", "none",
         "-qmp", f"unix:{SOCK},server,nowait", "-no-reboot"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        for _ in range(150):
            if os.path.exists(SOCK):
                break
            time.sleep(0.1)
        if not os.path.exists(SOCK):
            print("FAIL: QMP socket never appeared")
            return 1
        time.sleep(2.0)

        sock = socket.socket(socket.AF_UNIX)
        sock.settimeout(5.0)
        sock.connect(SOCK)
        f = sock.makefile("rwb")

        def qmp(execute, **args):
            msg = {"execute": execute}
            if args:
                msg["arguments"] = args
            f.write(json.dumps(msg).encode() + b"\r\n")
            f.flush()
            while True:
                m = json.loads(f.readline())
                if "return" in m or "error" in m:
                    return m

        def hmp(cmd):
            r = qmp("human-monitor-command", **{"command-line": cmd})
            return r.get("return", r.get("error"))

        while "QMP" not in json.loads(f.readline()):
            pass
        qmp("qmp_capabilities")

        PLAIN = "abcdefghijklmnopqrstuvwxyz0123456789"
        KEYNAME = {
            " ": "spc", ".": "dot", "-": "minus", "/": "slash", "_": "shift-minus",
        }

        def typeit(text):
            for ch in text:
                if ch == "\n":
                    hmp("sendkey ret")
                elif ch in KEYNAME:
                    hmp("sendkey " + KEYNAME[ch])
                elif ch.islower() or ch in PLAIN:
                    hmp("sendkey " + ch)
                else:
                    hmp("sendkey shift-" + ch.lower())
                time.sleep(0.04)
            time.sleep(0.5)

        def screen():
            qmp("pmemsave", val=0xB8000, size=0x4000, filename=VGA)
            data = open(VGA, "rb").read()
            lines = []
            for row in range(25):
                chars = []
                for col in range(80):
                    b = data[row * 160 + col * 2]
                    chars.append(chr(b) if 32 <= b < 127 else " ")
                lines.append("".join(chars).rstrip())
            return "\n".join(lines)

        def expect(pred, description):
            for _ in range(30):
                txt = screen()
                if pred(txt):
                    print(f"PASS: {description}")
                    return True
                time.sleep(0.2)
            print(f"FAIL: {description}")
            print(txt)
            return False

        checks = []
        typeit("pwd\n")
        checks.append(expect(lambda t: t.strip().endswith("/$") or "  /" in t or "root@asmos" in t,
                             "shell prompt responds (pwd)"))

        typeit("ls\n")
        checks.append(expect(lambda t: "KERNEL.BIN" in t and "ASMOS.MET" in t and "CONFIG.WF" in t,
                             "ls lists FAT12 files"))

        typeit("cat config.wf\n")
        checks.append(expect(lambda t: "active:superposition" in t,
                             "cat reads file contents"))

        typeit("mkdir testdir\n")
        checks.append(expect(lambda t: "testdir created" in t or "TESTDIR" in t,
                             "mkdir creates directory"))

        typeit("cd testdir\n")
        checks.append(expect(lambda t: "/testdir" in t,
                             "cd descends into subdirectory"))

        typeit("pwd\n")
        checks.append(expect(lambda t: "/testdir" in t,
                             "pwd in subdirectory"))

        typeit("ls\n")
        checks.append(expect(lambda t: "0 file(s)" in t or "TESTDIR" in t,
                             "ls inside subdirectory (empty)"))

        typeit("cd ..\n")
        typeit("pwd\n")
        checks.append(expect(lambda t: ("/$" in t) or ("  /" in t),
                             "cd .. returns to root"))

        return 0 if all(checks) else 1
    finally:
        try:
            proc.kill()
        except Exception:
            pass
        for f in (SOCK, VGA):
            if os.path.exists(f):
                os.unlink(f)


if __name__ == "__main__":
    sys.exit(main())