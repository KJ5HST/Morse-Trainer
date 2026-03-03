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

# Files to skip entirely (never copied)
SKIP_ALWAYS = {
    "src": {"main.cpp"},
    "include": set(),
    "data": set(),
}

# Files to copy only if the destination doesn't exist yet.
# Once present, the user's local edits are preserved across syncs.
PRESERVE_IF_EXISTS = {
    "include": {"config.h"},
}

# (source_dir, dest_dir)
SYNC_RULES = [
    ("src",     ARDUINO_DIR),
    ("include", ARDUINO_DIR),
    ("data",    os.path.join(ARDUINO_DIR, "data")),
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

    preserved = 0

    for src_subdir, dest_dir in SYNC_RULES:
        src_dir = os.path.join(PROJ_ROOT, src_subdir)
        if not os.path.isdir(src_dir):
            print(f"  WARNING: {src_subdir}/ not found, skipping")
            continue

        # On Windows without Developer Mode, git clones symlinks as small
        # text files containing the target path. Replace these with real
        # directories so we can copy files into them.
        if os.path.exists(dest_dir) and not os.path.isdir(dest_dir):
            os.remove(dest_dir)
            print(f"  Replaced broken symlink: {os.path.relpath(dest_dir, PROJ_ROOT)}")

        os.makedirs(dest_dir, exist_ok=True)

        skip_files = SKIP_ALWAYS.get(src_subdir, set())
        preserve_files = PRESERVE_IF_EXISTS.get(src_subdir, set())

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

            # On Windows, git may have created a small text file instead of
            # a symlink. Detect and remove these so they get replaced with
            # a real copy. A broken symlink text file is typically < 100 bytes
            # and contains a relative path like "../include/config.h".
            if os.path.isfile(dst_path) and os.path.getsize(dst_path) < 100:
                with open(dst_path, "r", errors="ignore") as f:
                    content = f.read().strip()
                if content.startswith("../"):
                    os.remove(dst_path)
                    print(f"  Replaced broken symlink: MorseTrainer/{os.path.relpath(dst_path, ARDUINO_DIR)}")

            # Preserve user-edited files (e.g. config.h with board selection)
            if filename in preserve_files and os.path.exists(dst_path):
                preserved += 1
                continue

            if needs_copy(src_path, dst_path):
                shutil.copy2(src_path, dst_path)
                print(f"  Copied: {src_subdir}/{filename} -> MorseTrainer/{os.path.relpath(dst_path, ARDUINO_DIR)}")
                copied += 1
            else:
                up_to_date += 1

    return copied, skipped_symlink, up_to_date, preserved


def main():
    print("Syncing Arduino IDE files (MorseTrainer/)...")
    print()

    if not os.path.isdir(ARDUINO_DIR):
        print(f"ERROR: MorseTrainer/ directory not found at {ARDUINO_DIR}")
        sys.exit(1)

    copied, skipped_symlink, up_to_date, preserved = sync()

    print()
    if copied == 0 and skipped_symlink > 0:
        print(f"All {skipped_symlink} files are symlinks — nothing to copy.")
    elif copied == 0:
        print("All files up to date.")
    else:
        print(f"Done: {copied} file(s) copied, {up_to_date} already up to date.")

    if preserved > 0:
        print(f"  ({preserved} user-edited file(s) preserved — delete to re-sync from source)")


if __name__ == "__main__":
    main()
