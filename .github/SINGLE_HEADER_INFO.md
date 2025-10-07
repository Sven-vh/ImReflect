# Single Header Generation

This document explains the single header generation feature for ImReflect.

## Overview

ImReflect now provides an automatically generated single-header version that combines all library headers and external dependencies into one file: `single_header/ImReflect.hpp`

## What Gets Combined

The single header includes:

### External Dependencies (in order)
1. `extern/svh/tag_invoke.hpp` - Tag invoke pattern implementation
2. `extern/svh/scope.hpp` - Scope management utilities  
3. `extern/magic_enum/magic_enum.hpp` - Enum reflection library
4. `extern/visit_struct/visit_struct.hpp` - Struct visitation library

### ImReflect Headers (in order)
5. `ImReflect_entry.hpp` - Core entry points, types, and response system
6. `ImReflect_helper.hpp` - Helper functions and utilities
7. `ImReflect_primitives.hpp` - Primitive type implementations (int, float, bool, etc.)
8. `ImReflect_std.hpp` - Standard library type implementations (string, vector, etc.)

### What's NOT Included

- **ImGui headers** - You must include these separately:
  - `imgui.h`
  - `imgui_internal.h`
  - `imgui_stdlib.h`

## Automatic Generation

The single header is automatically generated via GitHub Actions whenever:

- A new release is created

The workflow:
1. Runs the Python generation script
2. Uploads the generated file as a release asset
3. The file is attached to the release for easy download

## Manual Generation

To regenerate the single header manually:

```bash
python3 scripts/generate_single_header.py
```

## How It Works

The Python script (`scripts/generate_single_header.py`):

1. Reads each header file in dependency order
2. Removes `#pragma once` directives (since we're combining into one file)
3. Resolves and inlines local `#include` directives
4. Preserves ImGui and standard library `#include` directives
5. Adds file section markers for debugging
6. Handles UTF-8 BOM characters properly
7. Outputs to `single_header/ImReflect.hpp`

## Usage Example

Instead of:
```cpp
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <ImReflect.hpp>
```

You can use:
```cpp
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <single_header/ImReflect.hpp>
```

## Benefits

1. **Simpler integration** - Only one ImReflect header to include
2. **No path configuration** - No need to set up include paths for `extern/` directories
3. **Faster compilation** - Single translation unit for all ImReflect code
4. **Distribution** - Easy to drop into existing projects

## File Structure

```
single_header/
├── ImReflect.hpp   # ~4500 lines, auto-generated
└── README.md                  # Documentation for the generated directory
```

## Maintenance

- The generation script is located at `scripts/generate_single_header.py`
- The GitHub Actions workflow is at `.github/workflows/generate-single-header.yml`
- Both are version controlled and will be maintained alongside the library
