#!/usr/bin/env python3
"""
Extracts all files from The Legend of Zelda: The Wind Waker GameCube ISO
directly into the WiiWaker homebrew folder structure.
"""

import os
import sys
import time

# Use the wwlib GCM parser from betterww
sys.path.append("/Users/cesarriojas/Games/betterww")
from wwlib.gcm import GCM

def extract_iso(iso_path, output_dir):
    print(f"Opening GameCube ISO: {iso_path}")
    if not os.path.isfile(iso_path):
        print(f"Error: ISO not found: {iso_path}")
        sys.exit(1)

    start_time = time.time()
    gcm = GCM(iso_path)
    gcm.read_entire_disc()

    total_files = len(gcm.files_by_path)
    print(f"Discovered {total_files} files across {len(gcm.dirs_by_path)} directories.")
    os.makedirs(output_dir, exist_ok=True)

    with open(iso_path, "rb") as f_iso:
        count = 0
        for rel_path, file_entry in gcm.files_by_path.items():
            # If path starts with 'files/', strip it for relative path inside output_dir
            clean_path = rel_path
            if clean_path.startswith("files/"):
                clean_path = clean_path[len("files/"):]
            
            dest_path = os.path.join(output_dir, clean_path)
            os.makedirs(os.path.dirname(dest_path), exist_ok=True)

            f_iso.seek(file_entry.file_data_offset)
            # Read in chunks for large files
            remaining = file_entry.file_size
            with open(dest_path, "wb") as f_out:
                while remaining > 0:
                    chunk_sz = min(remaining, 1024 * 1024)
                    chunk = f_iso.read(chunk_sz)
                    if not chunk:
                        break
                    f_out.write(chunk)
                    remaining -= len(chunk)

            count += 1
            if count % 200 == 0 or count == total_files:
                pct = (count / total_files) * 100
                print(f"[{count:4d}/{total_files}] ({pct:5.1f}%) Extracted {clean_path}")

    elapsed = time.time() - start_time
    print(f"\nSuccessfully extracted {total_files} files in {elapsed:.2f} seconds to:")
    print(f"  {output_dir}")

if __name__ == "__main__":
    iso = "/Users/cesarriojas/Games/Game Cube/Legend of Zelda, The - The Wind Waker (USA).iso"
    out = "/Users/cesarriojas/Games/WiiWaker/dist/apps/WiiWaker/files"
    extract_iso(iso, out)
