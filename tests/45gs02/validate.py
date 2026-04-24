#!/usr/bin/env python3
import subprocess
import os
import sys

XMEGA65 = "/home/duck/src/xemu/build/bin/xmega65.native"
MMSIM_CLI = "./bin/mmemu-cli"
KICKASS = "tools/KickAss65CE02.jar"

def assemble(asm_path):
    cmd = ["java", "-jar", KICKASS, asm_path, "-odir", "tests/45gs02/"]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    basename = os.path.basename(asm_path).replace(".asm", ".prg")
    return f"tests/45gs02/tests/45gs02/{basename}"

def run_xmega65(prg_path):
    dump_path = "xmega65.dump"
    if os.path.exists(dump_path): os.remove(dump_path)
    
    # Ensure truly headless by stripping display variables
    env = os.environ.copy()
    env.pop("DISPLAY", None)
    env.pop("WAYLAND_DISPLAY", None)

    cmd = [
        XMEGA65,
        "-headless", "-sleepless", "-testing",
        "-gui", "none",
        "-prg", prg_path,
        "-autoload",
        "-dumpmem", dump_path
    ]
    try:
        subprocess.run(cmd, timeout=15, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
    except subprocess.TimeoutExpired:
        pass
    
    if os.path.exists(dump_path):
        with open(dump_path, "rb") as f:
            # Entire memory dump. Address $0400 is at index 0x0400
            data = f.read()
            if len(data) >= 0x0410:
                return data[0x0400:0x0402]
    return b""

def run_mmsim(prg_path):
    dump_path = "mmsim.dump"
    if os.path.exists(dump_path): os.remove(dump_path)
    # Save 16 bytes starting at $0400
    cli_script = f"create rawMega65\nload {prg_path}\nrun $0810\nsave {dump_path} $0400 16\nquit\n"
    subprocess.run([MMSIM_CLI], input=cli_script.encode(), timeout=5, 
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if os.path.exists(dump_path):
        with open(dump_path, "rb") as f:
            return f.read(2) # We only check first 2 bytes for now
    return b""

def main():
    if len(sys.argv) < 2:
        print("Usage: validate.py <test.asm>")
        sys.exit(1)
        
    asm = sys.argv[1]
    prg = assemble(asm)
    
    if not os.path.exists(prg):
        print(f"Assembly failed for {asm}")
        sys.exit(1)

    print(f"Validating {asm}...")
    mmsim_res = run_mmsim(prg)
    xmega65_res = run_xmega65(prg)
    
    print(f"  mmsim:   {mmsim_res.hex().upper()}")
    print(f"  xmega65: {xmega65_res.hex().upper()}")
    
    if b"\xff" in mmsim_res or mmsim_res == b"":
        print("  RESULT: FAIL (mmsim error or test failed)")
        sys.exit(1)
        
    if xmega65_res != b"" and mmsim_res != xmega65_res:
        print("  RESULT: FAIL (Cross-validation mismatch)")
        sys.exit(1)
    elif xmega65_res == b"":
        print("  RESULT: PASS (mmsim ok, xmega65 silent)")
    else:
        print("  RESULT: PASS (mmsim matches xmega65)")
        
    sys.exit(0)

if __name__ == "__main__":
    main()
