#!/usr/bin/env python3
"""
Sync source files into MorseTrainer/ for Arduino IDE compatibility.

On macOS/Linux, MorseTrainer/ uses symlinks back to src/, include/, and data/.
On Windows, git clones these symlinks as plain text files unless Developer Mode
is enabled. This script copies the real files into MorseTrainer/ so Arduino IDE
works regardless of OS or git symlink support.

Files that are already valid symlinks are skipped (no work needed).
Files are only overwritten if the source is newer than the destination.

Usage: python sync_arduino.py
"""

import os
import shutil
import sys

PROJ_ROOT = os.path.dirname(os.path.abspath(__file__))
ARDUINO_DIR = os.path.join(PROJ_ROOT, "MorseTrainer")

# (source_dir, glob pattern, dest_dir, files to skip)
SYNC_RULES = [
    ("src",     ARDUINO_DIR,            {"main.cpp"}),
    ("include", ARDUINO_DIR,            set()),
    ("data",    os.path.join(ARDUINO_DIR, "data"), set()),
]


def needs_copy(src, dst):
    """Return True if dst is missing or older than src."""
    if not os.path.exists(dst):
        return True
    return os.path.getmtime(src) > os.path.getmtime(dst)


def sync():
    copied = 0
    skipped_symlink = 0
    up_to_date = 0

    for src_subdir, dest_dir, skip_files in SYNC_RULES:
        src_dir = os.path.join(PROJ_ROOT, src_subdir)
        if not os.path.isdir(src_dir):
            print(f"  WARNING: {src_subdir}/ not found, skipping")
            continue

        os.makedirs(dest_dir, exist_ok=True)

        for filename in sorted(os.listdir(src_dir)):
            if filename in skip_files:
                continue

            src_path = os.path.join(src_dir, filename)
            if not os.path.isfile(src_path):
                continue

            dst_path = os.path.join(dest_dir, filename)

            # Skip if destination is already a working symlink
            if os.path.islink(dst_path) and os.path.exists(dst_path):
                skipped_symlink += 1
                continue

            if needs_copy(src_path, dst_path):
                shutil.copy2(src_path, dst_path)
                print(f"  Copied: {src_subdir}/{filename} -> MorseTrainer/{os.path.relpath(dst_path, ARDUINO_DIR)}")
                copied += 1
            else:
                up_to_date += 1

    # Also handle the data/ symlink itself — on Windows it may be a plain file
    data_link = os.path.join(ARDUINO_DIR, "data")
    if os.path.islink(data_link) and os.path.isdir(data_link):
        # data/ is a valid symlink directory, we don't need the copied files
        pass

    return copied, skipped_symlink, up_to_date


def main():
    print("Syncing Arduino IDE files (MorseTrainer/)...")
    print()

    if not os.path.isdir(ARDUINO_DIR):
        print(f"ERROR: MorseTrainer/ directory not found at {ARDUINO_DIR}")
        sys.exit(1)

    copied, skipped_symlink, up_to_date = sync()

    print()
    if copied == 0 and skipped_symlink > 0:
        print(f"All {skipped_symlink} files are symlinks — nothing to copy.")
    elif copied == 0:
        print("All files up to date.")
    else:
        print(f"Done: {copied} file(s) copied, {up_to_date} already up to date.")


if __name__ == "__main__":
    main()
