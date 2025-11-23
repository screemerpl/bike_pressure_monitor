#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Fix LVGL includes in SquareLine Studio generated files
This script changes 'include "lvgl/lvgl.h"' to 'include "lvgl.h"'
"""

import re
import sys
from pathlib import Path

def fix_lvgl_includes(file_path):
    """
    Fix LVGL includes in UI generated files.
    Returns tuple (True/False, number_of_changes)
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        
        # Replace incorrect LVGL includes
        # Pattern: #include "lvgl/lvgl.h" -> #include "lvgl.h"
        content = re.sub(r'#include\s+"lvgl/lvgl\.h"', '#include "lvgl.h"', content)
        
        # Also handle single quotes if they appear
        content = re.sub(r"#include\s+'lvgl/lvgl\.h'", '#include "lvgl.h"', content)
        
        if content != original_content:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)
            return True, 1
        
        return False, 0
        
    except Exception as e:
        print(f"ERROR processing {file_path}: {e}", file=sys.stderr)
        return False, 0

def main():
    """Main function - fixes all UI generated files"""
    
    # Get the main directory (where ui.h is located)
    script_dir = Path(__file__).parent
    ui_dir = script_dir / "UI"
    
    if not ui_dir.exists():
        print(f"WARNING: UI directory not found at {ui_dir}", file=sys.stderr)
        return 1
    
    # Files to check for LVGL include issues
    files_to_check = [
        ui_dir / "ui.h",
        ui_dir / "ui_helpers.h",
        ui_dir / "ui_events.h",
    ]
    
    # Also check all generated files in subdirectories
    files_to_check.extend(ui_dir.glob("**/*.h"))
    files_to_check.extend(ui_dir.glob("**/*.c"))
    
    total_fixed = 0
    fixed_files = []
    
    for file_path in files_to_check:
        if file_path.exists():
            was_fixed, changes = fix_lvgl_includes(file_path)
            if was_fixed:
                total_fixed += changes
                fixed_files.append(file_path.name)
    
    if fixed_files:
        print(f"Fixed LVGL includes in {len(fixed_files)} file(s)", file=sys.stderr)
        for fname in fixed_files:
            print(f"  - {fname}", file=sys.stderr)
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
