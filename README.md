<img width="1251" height="526" alt="ImReflect - Reflection Based ImGui Wrapper" src="https://github.com/user-attachments/assets/dc9a7044-f095-495d-813c-ec605e0b92a2" />

---

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
ImReflect::Input("Settings", settings);
```

<img width="604" height="238" alt="image" src="https://github.com/user-attachments/assets/b9d7d7fd-c13f-49c2-9e6b-48c6a443d5f6" />

That's basically it!

## Configurable ImGui Settings

The ``ImSettings`` struct provides a way to customize the ImGui widgets. It uses a *Fluent Builder Pattern** design to allow for easy configurations.

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
ImReflect::Input("Settings", settings, config);
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
ImReflect::Input("Settings", settings, config);
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
    .as_slider()     // Slider control
.pop()
.push_member<&Graphics::textures>()
    .as_radio()    // Radio buttons
.pop();

static Graphics gfx;
ImReflect::Input("Graphics", gfx, config);
```

<img width="604" height="238" alt="image" src="https://github.com/user-attachments/assets/1f0e5b6e-1e03-4e44-9fb2-bfcddd08ed48" />


### Response Handling

This is still a WIP, but as of right now, you can check if something has changed:

```cpp
PlayerStats stats;
auto response = ImReflect::Input("Player", stats);

// Check if any field changed
if (response.get<PlayerStats>().is_changed()) {
    printf("Player stats changed!\n");
}

// Check specific field interactions WIP
if (response.get_member<&PlayerStats::health>().is_changed()) {
    printf("Health changed to: %d\n", stats.health);
}
```

### Supported Types

See the [Wiki](https://github.com/Sven-vh/ImReflect/wiki/Type-Settings) for the supported types and functions.

### Custom Functionality

As of right now the library supports only primitive types. I plan on adding most std types too, like ``vector``, ``map``, ``pair``, etc.

However, if you need more customization, the library offers the ability to add your own custom type functions without touching the library. For example:
```cpp
struct Transform {
	vec3 position;
	vec3 rotation;
	vec3 scale;
	std::string name = "Foo";
};

void tag_invoke(ImReflect::ImInput_t, const char* label, Transform& value, ImSettings& settings, ImResponse& response) {
	ImGui::SeparatorText(label);
	ImGui::Text(value.name.c_str());

	ImGui::InputFloat3("Position", &value.position.x);
	ImGui::InputFloat3("Rotation", &value.rotation.x);
	ImGui::InputFloat3("Scale", &value.scale.x);
}
```

ImReflect will find this function using Tag Invoke and gets called when it receives a Transform.

**Important**, to be able to find your function, it needs to be defined exactly like this:

```cpp
void tag_invoke(ImReflect::ImInput_t, const char* label, {YOUR_TYPE}& value, ImSettings& settings, ImResponse& response) { }
```

Additionally, it's also possible to add your custom settings. So when someone calls ``push<YOUR_TYPE>``, they can set custom settings. If we use the ``Transform`` example:
```cpp
template<>
struct ImReflect::type_settings<Transform> : ImSettings {
private:
	bool _change_name = false;

public:
	// Setters
	type_settings<Transform>& change_name(bool value) { 
		_change_name = value; 
		RETURN_THIS_T(Transform); 
	}

	// Getters
	bool get_change_name() const {
		return _change_name;
	}
};

void tag_invoke(ImReflect::ImInput_t, const char* label, Transform& value, ImSettings& settings, ImResponse& response) {
	auto& transform_settings = settings.get<Transform>();
	//...
	if (transform_settings.get_change_name()) {
		ImGui::InputText("Name", &value.name);
	} else {
		ImGui::Text(value.name.c_str());
	}
	//...
}

// Somewhere in your code
auto config = ImSettings();
config.push<Transform>()
		.change_name(true)
	.pop();

static Transform t;
ImReflect::Input("My Transform", t, config);
```

A couple of things are important to note here.
1. It needs a ``template<>`` above it.
2. The struct needs to be defined as ``ImReflect::type_settings<YOUR_TYPE>``.
3. Has to inherit from ``ImSettings``, so you can chain pushes.
4. The setter functions need to return ``type_settings<YOUR_TYPE>&`` to be able to chain fnctions.

### Overwrite

If you don't like the way I'm drawing a type or need more customizations, you can overwrite the default implementation. Simply implement a tag_invoke function of said type, and your implementation will be called instead. For example, overriding ``int``:

```cpp
void tag_invoke(ImReflect::ImInput_t, const char* label, int& value, ImSettings& settings, ImResponse& response) {
    // Your implementation
}
```

You can also overwrite the int settings by implementing:
```cpp
template<>
struct ImReflect::type_settings<int> : ImSettings {
    // Getters and setters
};
```

Note that this **overwrites** the default implementation, so the default settings can not be used anymore. A workaround for this is to go to the default implementation and copy the classes it inherits from.

### Order of Operations

ImReflect has a defined way of checking for specific type implementations.

1. User defined implementations with ``tag_invoke(ImReflect::ImInput_t, ...)``
2. Library implementations, made by me, the author.
3. Reflection, using the ``IMGUI_REFLECT`` macro.

As said before, this allows you to easily overwrite my implementations since it first checks if you, the user, has made one. If not, it checks my implementations, and after that, it checks if it's reflected.

If none of these are implemented, it will call a static_assert in ``ImReflect::Detail::InputImpl()``with the error message

```
'error C2338: static_assert failed: 'No suitable Input implementation found for type T'
```

Check the console to see which type is not implemented. If you think this type should be supported, feel free to open an issue or draft a PR!

## API Reference

### Core Functions

- `ImReflect::Input(label, value)` - Render UI for a reflected struct with default settings
- `ImReflect::Input(label, value, settings)` - Render UI with custom settings

### Settings configurations

See the [Wiki](https://github.com/Sven-vh/ImReflect/wiki/Type-Settings) for the supported types and functions.

## Building

Currently, the library is provided in multiple headers, see ``ImReflect.hpp``, ``ImReflect_primitives.hpp``, ``ImReflect_entry.hpp``, and the extern folder. I intend on making it a single header in the future.

### Dependencies
- [ImGui](https://github.com/ocornut/imgui) (duh)
- [magic_enum](https://github.com/Neargye/magic_enum) (for enum reflection)
- [visit_struct](https://github.com/cbeck88/visit_struct) (for struct reflection)

## Fluent Builder Pattern

For this project, I developed a new way of using the Builder Pattern. For more info on how it works, see:

https://github.com/Sven-vh/fluent-builder-pattern

## Contributing

I'm open to all feedback, issues, and suggestions!

As I said, this project is part of my university project, so everything here is a learning experience for me.

## License

MIT, see [LICENSE](https://github.com/Sven-vh/ImReflect/blob/main/LICENSE). However, as a student, it would be greatly appreciated if you would give credit or let me know what you're using it for. This motivates me to keep working on open-source projects.
