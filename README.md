<img width="1251" height="526" alt="ImReflect - Reflection Based ImGui Wrapper" src="https://github.com/user-attachments/assets/dc9a7044-f095-495d-813c-ec605e0b92a2" />

---

# ImReflect

**Reflection-based ImGui wrapper.**

Automatically generate ImGui widgets for your structs and types.

```cpp
struct GameSettings {
    int volume = 50;
    float sensitivity = 1.0f;
    bool fullscreen = false;
};
IMGUI_REFLECT(GameSettings, volume, sensitivity, fullscreen)

// Single Call:
ImReflect::Input("Settings", settings);
```

<img width="604" height="238" alt="image" src="https://github.com/user-attachments/assets/b9d7d7fd-c13f-49c2-9e6b-48c6a443d5f6" />

---

## Features

- 🎯 **Less boilerplate** - One macro, automatic UI
- 🔧 **Configurable** - Fluent builder API for customizing widgets
- 📦 **Batteries included** - Works with primitives, enums, STL containers, smart pointers
- 🎨 **Extensible** - Add your own types without modifying the library
- 📄 **Single header** - Drop in and go

**[📖 All Types](https://github.com/Sven-vh/ImReflect/wiki/Type-Settings)** | **[📦 Download Latest Release](https://github.com/Sven-vh/ImReflect/releases)**

---

## Quick Start

### 1. Installation

Download the single header from [releases](https://github.com/Sven-vh/ImReflect/releases):

```cpp
#include "ImReflect.hpp"
```

### 2. Reflect Your Struct

```cpp
struct GameSettings {
    int volume = 50;
    float sensitivity = 1.0f;
    bool fullscreen = false;
};
IMGUI_REFLECT(GameSettings, volume, sensitivity, fullscreen)
```

### 3. Render UI

```cpp
GameSettings settings;
ImReflect::Input("Settings", settings);
```

Done! ImReflect automatically generates appropriate widgets for each member.

---

## Supported Types

**Most standard types work out of the box.** See [Wiki](https://github.com/Sven-vh/ImReflect/wiki/Type-Settings) for complete reference.

```cpp
// Primitives
static bool my_bool = true;
ImReflect::Input("my bool", my_bool);

static int my_int = 42;
ImReflect::Input("my int", my_int);

static float my_float = 3.14f;
ImReflect::Input("my float", my_float);

static std::string my_string = "Hello ImReflect!";
ImReflect::Input("my string", my_string);

static MyEnum my_enum = MyEnum::Option1;
ImReflect::Input("my enum", my_enum);
```

<img width="385" height="181" alt="image" src="https://github.com/user-attachments/assets/b8f52294-9125-481d-b0df-b3f164eafd42" />

---

**Most of STL is also included.**

```cpp
std::tuple<int, float, std::string> my_tuple = { 1, 2.0f, "three" };
ImReflect::Input("my tuple", my_tuple);

std::map<std::string, float> my_map = { {"one", 1.0f}, {"two", 2.0f} };
ImReflect::Input("my map", my_map);

std::vector<int> my_vec = { 1, 2, 3 };
ImReflect::Input("my vec", my_vec);
```

<img width="540" height="358" alt="image" src="https://github.com/user-attachments/assets/1b067972-9ae5-45ab-9c43-96f3e5b83d47" />

---

## Configuration

Customize widgets using the fluent builder pattern.

### Global Type Settings

Apply settings to all instances of a type:

```cpp
auto config = ImSettings();
config.push<int>()
    .as_slider()
    .min(0)
    .max(100)
.pop();

GameSettings settings;
ImReflect::Input("Settings", settings, config);
```

<img width="602" height="236" alt="image" src="https://github.com/user-attachments/assets/5ac0e439-8021-49f9-876e-5ef7be06e214" />

### Per-Member Settings

Configure specific struct members:

```cpp
auto config = ImSettings();
config.push_member<&GameSettings::volume>()
    .as_slider()
    .min(0)
    .max(100)
.pop()
.push_member<&GameSettings::sensitivity>()
    .as_drag()
    .min(0.1f)
    .max(2.0f)
.pop();

ImReflect::Input("Settings", settings, config);
```

### Enum Widgets

Choose how enums are displayed:

```cpp
enum class GraphicsQuality { Low, Medium, High, Ultra };

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
    .as_slider()    // Slider control
.pop()
.push_member<&Graphics::textures>()
    .as_radio()     // Radio buttons
.pop();

static Graphics gfx;
ImReflect::Input("Graphics", gfx, config);
```

<img width="604" height="238" alt="image" src="https://github.com/user-attachments/assets/1f0e5b6e-1e03-4e44-9fb2-bfcddd08ed48" />

---

## Response Handling

Track user interactions with return values:

```cpp
struct GameSettings {
    int volume = 50;
    float sensitivity = 1.0f;
    bool fullscreen = false;
};

GameSettings settings;
ImResponse response = ImReflect::Input("Settings", settings);

// Check if entire struct changed
if (response.get<GameSettings>().is_changed()) {
    // settings has changed
}

// Check if any int member changed
if (response.get<int>().is_changed()) {
    // any int has changed
}

// Check specific member
if (response.get_member<&GameSettings::volume>().is_changed()) {
    // volume specifically has changed
}
```

---

## Advanced Usage

### Custom Types

Add support for your own types using tag invoke:

```cpp
struct vec3 {
	float x = 0, y = 0, z = 0;
};

void tag_invoke(ImReflect::ImInput_t, const char* label, vec3& value, ImSettings& settings, ImResponse& response) {
	auto& vec3_response = response.get<vec3>();

	bool changed = ImGui::InputFloat3(label, &value.x);
	if (changed) vec3_response.changed();

	/* Check hovered, activated, etc*/
	ImReflect::Detail::check_input_states(vec3_response);
}
```

### Custom Settings

Add configurable settings for your custom types:

```cpp
template<>
struct ImReflect::type_settings<vec3> : ImRequired<vec3> {
private:
	bool _as_color = false;
public:
	// If vec3 needs to be rendered as a color picker
	type_settings<vec3>& as_color(const bool v = true) { _as_color = v; RETURN_THIS_T(vec3); }
	bool is_color() { return _as_color; }
};

void tag_invoke(ImReflect::ImInput_t, const char* label, vec3& value, ImSettings& settings, ImResponse& response) {
	/* Get the vec3 settings and response */
	auto& vec3_settings = settings.get<vec3>();
	auto& vec3_response = response.get<vec3>();

	const bool render_as_color = vec3_settings.is_color();

	bool changed = false;
	if (render_as_color) {
		changed = ImGui::ColorEdit3(label, &value.x);
	} else {
		changed = ImGui::InputFloat3(label, &value.x);
	}
	if (changed) vec3_response.changed();

	/* Check hovered, activated, etc*/
	ImReflect::Detail::check_input_states(vec3_response);
}

// Usage
ImSettings config;
config.push<vec3>()
		.as_color()
	.pop();

static vec3 my_vec3;
ImReflect::Input("my vec3", my_vec3, config);
```

**Requirements for custom settings:**
- Use `template<>` specialization
- Define as `ImReflect::type_settings<YOUR_TYPE>`
- Inherit from `ImRequired`
- Return `type_settings<YOUR_TYPE>&` from setters for chaining

### Overriding Built-in Types

Don't like the default implementation? Override it:

```cpp
void tag_invoke(ImReflect::ImInput_t, const char* label, int& value, ImSettings& settings, ImResponse& response) {
    // Your custom implementation for int
}
```

---

## Resolution Order

When rendering a type, ImReflect searches in this order at compile time:

1. **User implementations** - Your `tag_invoke` functions
2. **Library implementations** - Built-in ImReflect types
3. **Reflection** - Types with `IMGUI_REFLECT` macro

This means you can always override library behavior with your own implementations.

**If no implementation is found:**
```
error C2338: No suitable Input implementation found for type T
```

Check the console for the missing type. If you think it should be supported, [open an issue](https://github.com/Sven-vh/ImReflect/issues)!

---

## Building

### Option 1: Single Header (Recommended)

Download `ImReflect.hpp` from [releases](https://github.com/Sven-vh/ImReflect/releases):

```cpp
#include "ImReflect.hpp"
```

Includes all dependencies except ImGui.

### Option 2: Multiple Headers

Clone the repository and include the main header:

```cpp
#include 
```

This includes: `ImReflect_entry.hpp`, `ImReflect_helper.hpp`, `ImReflect_primitives.hpp`, `ImReflect_std.hpp`, and external dependencies.

### Dependencies

- **[ImGui](https://github.com/ocornut/imgui)** - You need to include this separately
- **[magic_enum](https://github.com/Neargye/magic_enum)** - Included in single header
- **[visit_struct](https://github.com/cbeck88/visit_struct)** - Included in single header

## Documentation

- **[Type Settings Reference](https://github.com/Sven-vh/ImReflect/wiki/Type-Settings)** - Complete API documentation
- **[Fluent Builder Pattern](https://github.com/Sven-vh/fluent-builder-pattern)** - Details on the configuration system

## Contributing

Feedback, issues, and pull requests are welcome! This project is part of my university work, so everything is a learning experience.

## License

**MIT License** - See [LICENSE](https://github.com/Sven-vh/ImReflect/blob/main/LICENSE)

As a student, credit or a quick note about what you're using it for is greatly appreciated! It motivates me to keep contributing to open source!
