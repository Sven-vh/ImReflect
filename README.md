# ImReflect

A reflection-based wrapper for ImGui that automatically generates ImGui UI.

> [!NOTE]
> This is WIP. It currently only supports primitive types, booleans, and enums.
>
> I'm making this as part of my university project.

# Basic Usage
## Simple Struct Reflection
```cpp
struct GameSettings {
    int volume = 50;
    float sensitivity = 1.0f;
    bool fullscreen = false;
};
// Specify which member variables need to be shown
IMGUI_REFLECT(GameSettings, volume, sensitivity, fullscreen)

GameSettings settings;

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

### Settings configurations

Todo

## Building

Currently, the library is provided in multiple headers, see ``ImReflect.hpp``, ``ImReflect_primitives.hpp``, ``ImReflect_entry.hpp``, and the extern folder. I intend on making it a single header in the future.

### Dependencies
- [ImGui](https://github.com/ocornut/imgui) (duh)
- [magic_enum](https://github.com/Neargye/magic_enum) (for enum reflection)
- [visit_struct](https://github.com/cbeck88/visit_struct) (for struct reflection)
