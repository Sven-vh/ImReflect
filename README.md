# imgui-reflect

A reflection-based wrapper for ImGui that automatically generates ImGui UI.

## Features

- **Automatic UI Generation**: Use the `IMGUI_REFLECT` macro to automatically create ImGui controls for your structs
- **Type-Safe Settings**: Configure input types, ranges, formats, and behavior with a fluent API
- **Enum Support**: Handle enums with radio buttons, dropdowns, sliders, or drag controls
- **Response Handling**: Get detailed feedback about user interactions (changed, hovered, clicked, etc.)
- **Nested Structures**: Support for complex nested data structures

# Basic Usage
## Simple Struct Reflection
```cpp
struct GameSettings {
    int volume = 50;
    float sensitivity = 1.0f;
    bool fullscreen = false;
};
IMGUI_REFLECT(GameSettings, volume, sensitivity, fullscreen)

static GameSettings settings;
// In your render loop
ImGui::Reflect::Input("Settings", settings);
```

<img width="604" height="238" alt="image" src="https://github.com/user-attachments/assets/b9d7d7fd-c13f-49c2-9e6b-48c6a443d5f6" />


## Configuring ImGui Settings

``ImSettings`` provides a way to customize the ImGui widgets. It uses the Builder Pattern to allow for fluent configurations.

### Types

All ints will now be a slider ranging from 0 to 100.

```cpp
auto config = ImSettings();
config.push<int>()
    .min(0)
    .max(100)
    .as_slider()
.pop();

GameSettings settings;
ImGui::Reflect::Input("Settings", settings, config);
```

<img width="602" height="236" alt="image" src="https://github.com/user-attachments/assets/5ac0e439-8021-49f9-876e-5ef7be06e214" />

### Member variables

If you want settings for a specific member variable, you can call ``push_member``. These settings will be only for the pushed member.

```cpp
auto config = ImSettings();
config.push_member<&GameSettings::volume>()
    .min(0)
    .max(100)
    .as_slider()
.pop()
.push_member<&GameSettings::sensitivity>()
    .min(0.1f)
    .max(5.0f)
    .as_drag()
.pop();

GameSettings settings;
ImGui::Reflect::Input("Settings", settings, config);
```

### Enum Handling

The library uses [``magic_enum.hpp``](https://github.com/Neargye/magic_enum) for automatic enum support.

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

static Graphics gfx;
ImGui::Reflect::Input("Graphics", gfx, config);
```

<img width="604" height="238" alt="image" src="https://github.com/user-attachments/assets/1f0e5b6e-1e03-4e44-9fb2-bfcddd08ed48" />


### Response Handling

This is still a WIP

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

## Building

Currently, the library is in multiple headers, see ``Core/Include``. I intend to make a single header in the future.

### Dependencies
- [ImGui](https://github.com/ocornut/imgui) (duh)
- [magic_enum](https://github.com/Neargye/magic_enum) (for enum reflection)
- [visit_struct](https://github.com/cbeck88/visit_struct) (for struct reflection)
