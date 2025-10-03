#!/usr/bin/env python3
"""
Script to combine multiple C++ headers into a single header file.
This script combines all ImReflect headers and external dependencies into one file.
"""

import os
import re
from pathlib import Path
from typing import Set, List

class HeaderCombiner:
    def __init__(self, repo_root: Path):
        self.repo_root = repo_root
        self.processed_files: Set[str] = set()
        self.output_lines: List[str] = []
        
    def should_skip_include(self, include_line: str) -> bool:
        """Check if an include line should be skipped (imgui includes and stdlib)."""
        # Skip imgui includes (they need to be kept separate)
        if '<imgui' in include_line or '"imgui' in include_line:
            return True
        # Skip standard library includes (they stay as-is)
        if include_line.strip().startswith('#include <') and '<extern/' not in include_line:
            return True
        return False
    
    def resolve_include_path(self, include_line: str, current_file: Path) -> Path | None:
        """Resolve the path of an included file."""
        # Extract the file path from the include directive
        match = re.search(r'#include\s+[<"](.+?)[>"]', include_line)
        if not match:
            return None
        
        include_path = match.group(1)
        
        # If it starts with extern/, resolve relative to repo root
        if include_path.startswith('extern/'):
            full_path = self.repo_root / include_path
        # Otherwise, resolve relative to the current file's directory
        else:
            full_path = current_file.parent / include_path
        
        if full_path.exists():
            return full_path.resolve()
        return None
    
    def process_file(self, file_path: Path, is_root: bool = False) -> None:
        """Process a single header file recursively."""
        file_path = file_path.resolve()
        file_str = str(file_path)
        
        # Avoid processing the same file twice
        if file_str in self.processed_files:
            return
        self.processed_files.add(file_str)
        
        if not is_root:
            self.output_lines.append(f"\n// ============================================================================")
            self.output_lines.append(f"// File: {file_path.relative_to(self.repo_root)}")
            self.output_lines.append(f"// ============================================================================\n")
        
        with open(file_path, 'r', encoding='utf-8-sig') as f:  # utf-8-sig handles BOM
            for line in f:
                # Remove BOM if present
                if line.startswith('\ufeff'):
                    line = line[1:]
                
                stripped = line.strip()
                
                # Skip #pragma once since we're combining into one file
                if stripped == '#pragma once' or stripped == '\ufeff#pragma once':
                    continue
                
                # Handle includes
                if stripped.startswith('#include'):
                    if self.should_skip_include(stripped):
                        # Keep imgui and stdlib includes
                        if '<imgui' in stripped or (stripped.startswith('#include <') and '<extern/' not in stripped):
                            if is_root or line not in [l + '\n' for l in self.output_lines]:
                                self.output_lines.append(line.rstrip())
                    else:
                        # Try to resolve and process the included file
                        included_file = self.resolve_include_path(stripped, file_path)
                        if included_file:
                            self.process_file(included_file)
                        else:
                            # If we can't resolve it, keep the include
                            self.output_lines.append(line.rstrip())
                else:
                    # Regular code line
                    self.output_lines.append(line.rstrip())
    
    def generate_combined_header(self, output_path: Path) -> None:
        """Generate the final combined header file."""
        # Header comment
        header = [
            "// ============================================================================",
            "// ImReflect - Single Header File",
            "// Auto-generated - DO NOT EDIT MANUALLY",
            "//",
            "// This file combines all ImReflect headers and external dependencies.",
            "// Include this file instead of ImReflect.hpp for single-header usage.",
            "//",
            "// Note: You still need to include imgui.h, imgui_internal.h, and imgui_stdlib.h",
            "//       separately as they are not part of this single header.",
            "// ============================================================================",
            "",
            "#pragma once",
            "",
            "// Required ImGui includes (you must have ImGui in your project)",
            "#include <imgui.h>",
            "#include <imgui_internal.h>",
            "#include <imgui_stdlib.h>",
            "",
            "// Standard library includes",
            "#include <string>",
            "#include <vector>",
            "#include <array>",
            "#include <map>",
            "#include <optional>",
            "#include <variant>",
            "#include <functional>",
            "#include <type_traits>",
            "#include <utility>",
            "#include <algorithm>",
            "#include <cstddef>",
            "#include <cstdint>",
            "",
        ]
        
        # Write the file
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'w', encoding='utf-8') as f:
            # Write header
            for line in header:
                f.write(line + '\n')
            
            # Write combined content
            for line in self.output_lines:
                f.write(line + '\n')
        
        print(f"Generated single header at: {output_path}")
        print(f"Processed {len(self.processed_files)} files")


def main():
    # Get repository root
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent
    
    print(f"Repository root: {repo_root}")
    
    # Initialize combiner
    combiner = HeaderCombiner(repo_root)
    
    # Define files to process in order
    files_to_process = [
        # External dependencies first (in dependency order)
        repo_root / "extern/svh/tag_invoke.hpp",
        repo_root / "extern/svh/scope.hpp",
        repo_root / "extern/magic_enum/magic_enum.hpp",
        repo_root / "extern/visit_struct/visit_struct.hpp",
        # ImReflect headers (in dependency order)
        repo_root / "ImReflect_entry.hpp",
        repo_root / "ImReflect_helper.hpp",
        repo_root / "ImReflect_primitives.hpp",
        repo_root / "ImReflect_std.hpp",
    ]
    
    # Process each file
    for file_path in files_to_process:
        if file_path.exists():
            print(f"Processing: {file_path.relative_to(repo_root)}")
            combiner.process_file(file_path)
        else:
            print(f"Warning: File not found: {file_path}")
    
    # Generate output
    output_dir = repo_root / "generated"
    output_file = output_dir / "ImReflect_generated.hpp"
    combiner.generate_combined_header(output_file)
    
    print(f"\nSuccess! Single header generated at: {output_file.relative_to(repo_root)}")


if __name__ == "__main__":
    main()
