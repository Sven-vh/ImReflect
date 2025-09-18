# imgui-reflect

A reflection-based wrapper for ImGui that automatically generates UI elements from C++ structs and provides type-safe configuration.

## Features

- **Automatic UI Generation**: Use the `IMGUI_REFLECT` macro to automatically create ImGui controls for your structs
- **Type-Safe Settings**: Configure input types, ranges, formats, and behavior with a fluent API
- **Enum Support**: Handle enums with radio buttons, dropdowns, sliders, or drag controls
- **Response Handling**: Get detailed feedback about user interactions (changed, hovered, clicked, etc.)
- **Nested Structures**: Support for complex nested data structures

## Quick Start

```cpp
#include <imgui_reflect.hpp>

struct PlayerStats {
    int health = 100;
    float speed = 5.0f;
    bool isAlive = true;
};
IMGUI_REFLECT(PlayerStats, health, speed, isAlive)

// In your ImGui render loop:
static PlayerStats stats;
ImGui::Reflect::Input("Player", stats);
```

## Basic Usage

### Simple Struct Reflection

```cpp
struct GameSettings {
    int volume = 50;
    float sensitivity = 1.0f;
    bool fullscreen = false;
};
IMGUI_REFLECT(GameSettings, volume, sensitivity, fullscreen)

// Render with default settings
GameSettings settings;
ImGui::Reflect::Input("Settings", settings);
```

### Configuring Input Types

```cpp
// Create settings to customize how fields are displayed
auto config = ImSettings();
config.push_member<&GameSettings::volume>()
    .min(0)
    .max(100)
    .as_slider()
    .clamp()
.pop()
.push_member<&GameSettings::sensitivity>()
    .min(0.1f)
    .max(5.0f)
    .as_drag()
    .speed(0.01f)
.pop();

GameSettings settings;
ImGui::Reflect::Input("Settings", settings, config);
```

### Enum Handling

```cpp
enum class GraphicsQuality {
    Low, Medium, High, Ultra
};

struct Graphics {
    GraphicsQuality quality = GraphicsQuality::Medium;
    GraphicsQuality shadows = GraphicsQuality::High;
    GraphicsQuality textures = GraphicsQuality::High;
};
IMGUI_REFLECT(Graphics, quality, shadows, textures)

// Configure different input types for enums
auto config = ImSettings();
config.push_member<&Graphics::quality>()
    .as_dropdown()  // Dropdown menu
.pop()
.push_member<&Graphics::shadows>()
    .as_radio()     // Radio buttons
.pop()
.push_member<&Graphics::textures>()
    .as_slider()    // Slider control
.pop();

Graphics gfx;
ImGui::Reflect::Input("Graphics", gfx, config);
```

### Response Handling

```cpp
PlayerStats stats;
auto response = ImGui::Reflect::Input("Player", stats);

// Check if any field changed
if (response.get<PlayerStats>().is_changed()) {
    printf("Player stats changed!\n");
}

// Check specific field interactions
if (response.get_member<&PlayerStats::health>().is_changed()) {
    printf("Health changed to: %d\n", stats.health);
}
```

## API Reference

### Core Functions

- `ImGui::Reflect::Input(label, value)` - Render UI for a reflected struct with default settings
- `ImGui::Reflect::Input(label, value, settings)` - Render UI with custom settings

### Settings Configuration

#### Numeric Types
```cpp
settings.push<int>()
    .min(0).max(100)        // Set range
    .clamp()                // Enforce clamping
    .as_slider()           // Use slider widget
    .as_drag()             // Use drag widget  
    .as_input()            // Use input field
    .step(1).step_fast(10) // Set step increments
    .as_hex()              // Display as hexadecimal
    .width(100)            // Set field width
.pop();
```

#### Float Types
```cpp
settings.push<float>()
    .min(0.0f).max(1.0f)
    .as_float(3)           // 3 decimal places
    .as_percentage()       // Display as percentage
    .as_scientific(2)      // Scientific notation
    .logarithmic()         // Logarithmic scale
    .speed(0.01f)          // Drag speed
.pop();
```

#### Enum Types
```cpp
settings.push<MyEnum>()
    .as_dropdown()         // Dropdown menu
    .as_radio()           // Radio buttons
    .as_slider()          // Slider through values
    .as_drag()            // Drag through values
.pop();
```

### Member-Specific Settings
```cpp
// Configure specific struct members
settings.push_member<&MyStruct::fieldName>()
    .min(0).max(100)
    .as_slider()
.pop();
```

## Building

This is a header-only library. Include the headers and link against ImGui:

```cpp
#include <imgui_reflect.hpp>
```

### Dependencies
- ImGui
- magic_enum (for enum reflection)
- visit_struct (for struct reflection)

## License

See LICENSE file for details.
