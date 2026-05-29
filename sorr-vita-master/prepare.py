#!/usr/bin/env python3
import os
import shutil
import glob
import subprocess
import sys

# Script is in project root, so data/ is here:
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "data")
TOOLS_DIR = os.path.join(BASE_DIR, "tools")
MOD_DIR = os.path.join(DATA_DIR, "mod")

PYTHON = sys.executable or "python3"


def ensure_directories():
    if not os.path.isdir(DATA_DIR):
        raise SystemExit(f"ERROR: data/ directory not found at: {DATA_DIR}")
    os.makedirs(MOD_DIR, exist_ok=True)


def delete_files():
    """Delete *.dll, *.exe and SorMaker.dat in data/"""
    patterns = ["*.dll", "*.exe", "SorMaker.dat"]

    for pattern in patterns:
        for path in glob.glob(os.path.join(DATA_DIR, pattern)):
            try:
                os.remove(path)
                print(f"Removed: {path}")
            except Exception as e:
                print(f"Could not remove {path}: {e}")


def extract_sorr():
    """Run extract_stub.py SorR.dat inside data/"""
    script_path = os.path.join(TOOLS_DIR, "extract_stub.py")

    if not os.path.isfile(script_path):
        raise SystemExit(f"ERROR: extract_stub.py not found at {script_path}")

    print("Running extractor...")
    subprocess.check_call([PYTHON, script_path, "SorR.dat"], cwd=DATA_DIR)

    stripped_file = os.path.join(DATA_DIR, "SorR_stripped.dcb")
    final_file = os.path.join(DATA_DIR, "SorR.dat")

    if os.path.exists(stripped_file):
        os.replace(stripped_file, final_file)
        print("Renamed SorR_stripped.dcb -> SorR.dat")
    else:
        print("WARNING: SorR_stripped.dcb does not exist!")


def copy_system():
    """Copy system.txt -> data/mod/system.txt"""
    src = os.path.join(BASE_DIR, "system.txt")
    dst = os.path.join(MOD_DIR, "system.txt")

    if not os.path.isfile(src):
        raise SystemExit(f"ERROR: system.txt not found at {src}")

    shutil.copy2(src, dst)
    print(f"Copied: {src} -> {dst}")


def main():
    ensure_directories()
    delete_files()
    extract_sorr()
    copy_system()
    print("\n✔ Done.")


if __name__ == "__main__":
    main()
