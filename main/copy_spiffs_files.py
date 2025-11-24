#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Copy SquareLine Studio SPIFFS files to spiffs_data directory
This script copies all files from UI/drive/* to ../spiffs_data/
"""

import shutil
import sys
from pathlib import Path

def copy_spiffs_files():
    """Copy files from UI/drive to spiffs_data"""
    
    # Get paths
    script_dir = Path(__file__).parent
    source_dir = script_dir / "UI" / "drive"
    target_dir = script_dir.parent / "spiffs_data"
    
    if not source_dir.exists():
        print(f"INFO: Source directory not found: {source_dir}", file=sys.stderr)
        return 0
    
    if not target_dir.exists():
        target_dir.mkdir(parents=True, exist_ok=True)
        print(f"Created target directory: {target_dir}", file=sys.stderr)
    
    # Copy all files recursively from source to target
    copied_count = 0
    for source_file in source_dir.rglob("*"):
        if source_file.is_file():
            # Preserve directory structure or flatten to target_dir
            target_file = target_dir / source_file.relative_to(source_dir)
            target_file.parent.mkdir(parents=True, exist_ok=True)
            try:
                shutil.copy2(source_file, target_file)
                copied_count += 1
                print(f"Copied: {source_file.relative_to(source_dir)}", file=sys.stderr)
            except Exception as e:
                print(f"ERROR copying {source_file.name}: {e}", file=sys.stderr)
                return 1
    
    if copied_count > 0:
        print(f"Successfully copied {copied_count} file(s) to spiffs_data/", file=sys.stderr)
    else:
        print(f"No files to copy from {source_dir}", file=sys.stderr)
    
    return 0

if __name__ == '__main__':
    sys.exit(copy_spiffs_files())
