#include "imgui_app.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>

#include <ImReflect.hpp>

#define IMGUI_SAMPLE_CODE(x) \
ImGui::Text(#x); \
x

#define IMGUI_SAMPLE_MULTI_CODE(x) \
{ \
	std::string input = x; \
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled)); \
	ImGui::InputTextMultiline("##"#x, &input, ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AllowTabInput); \
	ImGui::PopStyleColor(1); \
}

/* From imgui_demo.cpp */
static void HelpMarker(const char* desc) {
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip()) {
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// ========================================
// Generic primitive Test
// ========================================
template<typename T>
static void primitive_test() {
	std::type_index info = typeid(T);
	const bool is_const = std::is_const_v<T>;
	const std::string name = is_const ? std::string("const_") + info.name() : info.name();
	ImGui::SeparatorText(name.c_str());
	ImGui::PushID(name.c_str());
	ImGui::Indent();

	const std::string type_name = name.c_str();
	const std::string type_label = "my_" + type_name;
	static T my_var = T(0);

	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		const std::string label = type_label + "##default";
		ImReflect::Input(label.c_str(), my_var);
	}

	ImGui::Text("Input - step");
	HelpMarker("Input with step and fast step settings, hold ctrl+click to use step fast");
	{
		const std::string label = type_label + "##input step";
		auto config = ImSettings();
		config.push<T>()
			.step(5)
			.step_fast(50)
			.pop();
		ImReflect::Input(label.c_str(), my_var, config);
	}

	ImGui::Text("Slider - no limit");
	HelpMarker("Slider ranging from numeric limits of the type.\nIf signed it goes from lowest()/2 to max()/2 because of ImGui limitations.\nUnsigned goes from 0 to max().");
	{
		const std::string label = type_label + "##slider";

		auto config = ImSettings();
		config.push<T>()
			.as_slider()
			.pop();

		ImReflect::Input(label.c_str(), my_var, config);
	}

	ImGui::Text("Slider - min/max");
	HelpMarker("Slider with custom min/max range");
	{
		const std::string label = type_label + "##slider min/max";

		auto config = ImSettings();
		config.push<T>()
			.min(0)
			.max(10)
			.as_slider()
			.pop();

		ImReflect::Input(label.c_str(), my_var, config);
	}

	ImGui::Text("Slider - logarithmic");
	HelpMarker("Slider with logarithmic scale");
	{
		const std::string label = type_label + "##slider log";
		auto config = ImSettings();
		config.push<T>()
			.min(0)
			.max(10)
			.as_slider()
			.logarithmic()
			.pop();
		ImReflect::Input(label.c_str(), my_var, config);
	}

	ImGui::Text("Drag - no limit");
	HelpMarker("Drag with no min/max limit");
	{
		const std::string label = type_label + "##drag";
		auto config = ImSettings();
		config.push<T>()
			.as_drag()
			.pop();
		ImReflect::Input(label.c_str(), my_var, config);
	}

	ImGui::Text("Drag - min/max - speed");
	HelpMarker("Drag with min/max limit and custom drag speed");
	{
		const std::string label = type_label + "##drag settings";
		auto config = ImSettings();
		config.push<T>()
			.min(0)
			.max(10)
			.speed(0.05f)
			.as_drag()
			.pop();
		ImReflect::Input(label.c_str(), my_var, config);
	}

	ImGui::Text("Drag - min/max - speed - wrap around");
	HelpMarker("Drag with min/max limit and wrap around");
	{
		const std::string label = type_label + "##drag wrap";
		auto config = ImSettings();
		config.push<T>()
			.min(0)
			.max(10)
			.speed(0.05f)
			.as_drag()
			.wrap_around()
			.pop();
		ImReflect::Input(label.c_str(), my_var, config);
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Format Test
// ========================================
static void format_test() {
	ImGui::SeparatorText("Format");
	ImGui::PushID("format");
	ImGui::Indent();
	static float my_float = 0.0f;
	static double my_double = 0.0;
	ImGui::Text("Float - default format");
	HelpMarker("Default format");
	{
		auto config = ImSettings();
		config.push<float>()
			.pop();
		ImReflect::Input("my_float", my_float, config);
	}
	ImGui::Text("Float - custom format");
	HelpMarker("Custom format with 3 decimal places");
	{
		auto config = ImSettings();
		config.push<float>()
			.format("%.10f")
			.pop();
		ImReflect::Input("my_float##format", my_float, config);
	}

	ImGui::NewLine();
	ImGui::Text("Double - default format");
	HelpMarker("Default format");
	{
		auto config = ImSettings();
		config.push<double>()
			.pop();
		ImReflect::Input("my_double", my_double, config);
	}
	ImGui::Text("uint32_t - as_hex()");
	HelpMarker("Custom format as hexadecimal");
	{
		auto config = ImSettings();
		config.push<uint32_t>()
			.as_hex()
			.pop();
		static uint32_t my_char = 0;
		ImReflect::Input("uint32_t", my_char, config);
	}


	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Boolean Test
// ========================================
template<typename T>
static void boolean_test() {
	const bool is_const = std::is_const_v<T>;
	const std::string name = is_const ? std::string("const_") + typeid(T).name() : typeid(T).name();
	ImGui::SeparatorText(name.c_str());
	ImGui::PushID("bool");
	ImGui::Indent();

	static T my_bool;

	ImGui::Text("Default - as_checkbox()");
	HelpMarker("Will make it a checkbox widget, is default");
	{
		auto config = ImSettings();
		config.push<T>()
			.as_checkbox()
			.pop();
		ImReflect::Input("my_bool", my_bool, config);
	}

	ImGui::NewLine();

	ImGui::Text("Radio - as_radio()");
	HelpMarker("Radio button style");
	{
		auto config = ImSettings();
		config.push<T>()
			.as_radio()
			.pop();
		ImReflect::Input("my_bool##radio", my_bool, config);
	}

	ImGui::Text("Radio - as_radio() - custom text");
	HelpMarker("Radio button style with custom text for true/false states");
	{
		auto config = ImSettings();
		config.push<T>()
			.as_radio()
			.true_text("Yes")
			.false_text("No")
			.pop();
		ImReflect::Input("my_bool##radio text", my_bool, config);
	}

	ImGui::NewLine();

	ImGui::Text("Button - as_button()");
	HelpMarker("Button style");
	{
		auto config = ImSettings();
		config.push<T>()
			.as_button()
			.pop();
		ImReflect::Input("my_bool##button", my_bool, config);
	}

	ImGui::Text("Button - as_button() - custom text");
	HelpMarker("Button style with custom text for true/false states");
	{
		auto config = ImSettings();
		config.push<T>()
			.as_button()
			.true_text("On")
			.false_text("Off")
			.pop();
		ImReflect::Input("my_bool##button text", my_bool, config);
	}

	ImGui::NewLine();

	ImGui::Text("Button - as_dropdown()");
	HelpMarker("Dropdown style");
	{
		auto config = ImSettings();
		config.push<T>()
			.as_dropdown()
			.pop();
		ImReflect::Input("my_bool##dropdown", my_bool, config);
	}

	ImGui::Text("Button - as_dropdown() - custom text");
	HelpMarker("Dropdown style with custom text for true/false states");
	{
		auto config = ImSettings();
		config.push<T>()
			.as_dropdown()
			.true_text("Enabled")
			.false_text("Disabled")
			.pop();
		ImReflect::Input("my_bool##dropdown text", my_bool, config);
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Enum Test
// ========================================
enum class MyEnum {
	Option1,
	Option2,
	Option3
};

template<typename E>
static void enum_test() {
	const bool is_const = std::is_const_v<E>;
	const std::string name = is_const ? std::string("const_") + typeid(E).name() : typeid(E).name();
	ImGui::SeparatorText(name.c_str());
	ImGui::PushID(name.c_str());
	ImGui::Indent();

	static E my_enum;

	ImGui::Text("Default - as_dropdown()");
	HelpMarker("Default settings");
	{
		auto config = ImSettings();
		config.push<E>()
			.as_dropdown()
			.pop();
		ImReflect::Input("my_enum", my_enum, config);
	}

	ImGui::NewLine();

	ImGui::Text("Radio - as_radio()");
	HelpMarker("Radio button style");
	{
		auto config = ImSettings();
		config.push<E>()
			.as_radio()
			.pop();
		ImReflect::Input("my_enum##radio", my_enum, config);
	}

	ImGui::NewLine();

	ImGui::Text("Slider - as_slider()");
	HelpMarker("Slider style");
	{
		auto config = ImSettings();
		config.push<E>()
			.as_slider()
			.pop();
		ImReflect::Input("my_enum##slider", my_enum, config);
	}

	ImGui::NewLine();

	ImGui::Text("Drag - as_drag()");
	HelpMarker("Drag style");
	{
		auto config = ImSettings();
		config.push<E>()
			.as_drag()
			.pop();
		ImReflect::Input("my_enum##drag", my_enum, config);
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Pointer Test
// ========================================
static void pointer_test() {
	const std::string name = "Pointer";

	ImGui::SeparatorText((name + "*").c_str());
	ImGui::PushID((name + "*").c_str());
	ImGui::Indent();

	static int my_value = 42;
	static int* invalid_ptr = nullptr;
	static int* my_ptr = invalid_ptr;

	ImGui::Text("Pointer - nullptr");
	HelpMarker("Pointer is nullptr, nothing to edit");
	{
		ImReflect::Input("my_ptr", invalid_ptr);
	}

	ImGui::NewLine();

	ImGui::Text("Pointer - valid");
	HelpMarker("Pointer is valid, you can edit the value it points to");
	{
		my_ptr = &my_value;
		ImReflect::Input("my_ptr", my_ptr);
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Simple Test
// ========================================

struct MyStruct {
	int a = 0;
	float b = 0.0f;
	bool c = false;
};
IMGUI_REFLECT(MyStruct, a, b, c)

static void simple_reflection_test() {
	ImGui::SeparatorText("Struct / Class Reflection");
	ImGui::PushID("reflection");
	ImGui::Indent();

	static MyStruct my_struct;

	ImGui::Text("Simple Struct");
	HelpMarker("You need to define the ``IMGUI_REFLECT`` macro, this determines what is reflected");
	{
		const std::string code = R"(struct MyStruct {
	int a;
	float b;
	bool c = false;
};
IMGUI_REFLECT(MyStruct, a, b, c))";

		IMGUI_SAMPLE_MULTI_CODE(code);

		ImGui::Text("Output:");
		ImReflect::Input("my_struct", my_struct);
	}

	ImGui::Text("Simple Struct const");
	HelpMarker("You can also reflect const structs");
	{
		static const MyStruct my_const_struct;

		ImGui::Text("Output:");
		ImReflect::Input("my_const_struct", my_const_struct);
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Nested Test
// ========================================
struct Foo {
	int a;
	float b;
	bool c;
};
IMGUI_REFLECT(Foo, a, b, c)

struct Bar {
	int x;
	float y;
	bool z;
	Foo foo;
};
IMGUI_REFLECT(Bar, x, y, z, foo)


static void nested_reflection_test() {
	ImGui::SeparatorText("Nested Struct / Class Reflection");
	ImGui::PushID("nested reflection");
	ImGui::Indent();

	static Bar bar;

	ImGui::Text("Nested Structs");
	HelpMarker("You need to define the ``IMGUI_REFLECT`` macro for each struct, this determines what is reflected");
	{
		const std::string code = R"(struct Foo {
	int a;
	float b;
	bool c;
};
IMGUI_REFLECT(Foo, a, b, c)

struct Bar {
	int x;
	float y;
	bool z;
	Foo foo;
};
IMGUI_REFLECT(Bar, x, y, z, foo)";

		IMGUI_SAMPLE_MULTI_CODE(code);

		ImGui::Text("Output:");
		ImReflect::Input("foo", bar);
	}

	ImGui::NewLine();

	ImGui::Text("Nested Structs - pushing");
	HelpMarker("Push setting inside another struct.");
	{
		auto config = ImSettings();
		config.push<Foo>()
			.push<int>()
			.min(0)
			.max(10)
			.as_slider()
			.pop()
			.pop();

		const std::string code = R"(auto config = ImSettings();
config.push<Bar>()
		.push<Foo>()
			.push<int>()
				.min(0)
				.max(10)
				.as_slider()
			.pop() // int
		.pop() // Foo
	.pop(); // Bar)";
		IMGUI_SAMPLE_MULTI_CODE(code);

		ImGui::Text("Output:");
		ImReflect::Input("bar", bar, config);
	}

	ImGui::Unindent();
	ImGui::PopID();
};

// ========================================
// Type Settings Test
// ========================================

struct MyTypes {
	int int_one;
	int int_two;

	float float_one;
	float float_two;

	bool bool_one;
	bool bool_two;
};
IMGUI_REFLECT(MyTypes, int_one, int_two, float_one, float_two, bool_one, bool_two)

static void type_settings_test() {
	ImGui::SeparatorText("Type Settings");
	ImGui::PushID("type_settings");

	ImGui::TextWrapped(
		"Sometimes you have multiple of the same type in a struct but might want different settings for each one."
		" For example, a struct like this:");
	const std::string type_struct_code = R"(struct MyTypes {
	int int_one;
	int int_two;

	float float_one;
	float float_two;

	bool bool_one;
	bool bool_two;
};
IMGUI_REFLECT(MyTypes, int_one, int_two, float_one, float_two, bool_one, bool_two))";
	IMGUI_SAMPLE_MULTI_CODE(type_struct_code);

	ImGui::TextWrapped("You can see that we have multiple members of the same type, but we might want different settings for each one. Here come Type settings into play.");

	ImGui::NewLine();
	ImGui::NewLine();

	ImGui::Indent();
	static MyTypes my_types;

	ImGui::Text("Type Settings");
	HelpMarker("You can define settings for all members of a specific type");
	{
		ImGui::PushID("int settings");

		ImSettings config;
		config.push<int>()
			.min(0)
			.max(10)
			.as_slider()
			.pop();

		const std::string code = R"(ImSettings config;
config.push<int>()
	.min(0)
	.max(10)
	.as_slider()
.pop();

ImReflect::Input("my_types", my_types, config);)";

		IMGUI_SAMPLE_MULTI_CODE(code);

		ImGui::Text("All ints are now a slider between 0 and 10\noutput:");

		ImReflect::Input("my_types", my_types, config);

		ImGui::PopID();
	}

	ImGui::Unindent();
	ImGui::PopID();
};

// ========================================
// Member Settings Test
// ========================================
static void member_settings_test() {
	ImGui::SeparatorText("Member Settings");
	ImGui::PushID("member_settings");
	ImGui::Indent();

	static MyTypes my_struct;
	ImGui::Text("Member Settings");
	HelpMarker("You can define settings for specific members, by specifying the member name");
	{
		ImSettings config;
		config.push_member<&MyTypes::int_one>()
			.min(0)
			.max(10)
			.as_slider()
			.pop()
			.push_member<&MyTypes::int_two>()
			.min(-50)
			.max(50)
			.as_input()
			.clamp()
			.pop();

		const std::string code = R"(ImSettings config;
config.push_member<&MyTypes::int_one>()
	.min(0)
	.max(10)
	.as_slider()
.pop()
.push_member<&MyTypes::int_two>()
	.min(-50)
	.max(50)
	.as_input()
	.clamp()
.pop();)";

		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("int_one is now a slider between 0 and 10\nint_two is now an input between -50 and 50\nOutput:");
		ImReflect::Input("my_struct", my_struct, config);

	}

	ImGui::NewLine();

	ImGui::Text("Custom Labels");
	HelpMarker("You can define custom labels for types and member variables");
	{
		ImSettings config;
		config.push_member<&MyTypes::int_one>()
			.label("My Int One")
			.pop()
			.push_member<&MyTypes::int_two>()
			.label("My Int Two")
			.pop()
			.push<float>()
			.label("A Float value")
			.pop();

		const std::string code = R"(ImSettings config;
config.push_member<&MyTypes::int_one>()
	.label("My Int One")
.pop()
.push_member<&MyTypes::int_two>()
	.label("My Int Two")
.pop()
.push<float>()
	.label("A Float value")
.pop();)";
		IMGUI_SAMPLE_MULTI_CODE(code);

		ImGui::Text("Custom labels for int_one, int_two and all floats\nOutput:");
		ImReflect::Input("my_struct", my_struct, config);
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Custom implementation Test
// Writing a tag_invoke implementation for a custom type that IMReflect will find.
// ========================================

struct vec3 {
	float x = 0, y = 0, z = 0;
};

/* Type Settings are optional if you want to add more customization */
template<>
struct ImReflect::type_settings<vec3> : ImRequired<vec3> {
private:
	bool _as_color = false;
public:
	// If vec3 needs to be rendered as a color picker
	type_settings<vec3>& as_color(const bool v = true) { _as_color = v; RETURN_THIS_T(vec3); }
	bool is_color() { return _as_color; }
};

/* ImRefelct will find this function at compile time when it encounters an vec3 */
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

static void implementation_test() {
	ImGui::SeparatorText("Custom Implementation");
	ImGui::PushID("custom_implementation");
	ImGui::Indent();


	ImGui::Text("Custom vec3 type - default");
	HelpMarker("You can write a custom tag_invoke implementation for your types, ImReflect will find it at compile time.");
	{
		ImGui::PushID("vec3 default");
		const std::string code = R"(struct vec3 {
	float x = 0, y = 0, z = 0;
};

// Type Settings
template<>
struct ImReflect::type_settings<vec3> : ImRequired<vec3> {
private:
	bool _as_color = false;
public:
	// If vec3 needs to be rendered as a color picker
	type_settings<vec3>& as_color(const bool v = true) { _as_color = v; RETURN_THIS_T(vec3); }
	bool is_color() { return _as_color; }
};

// Custom tag_invoke implementation
void tag_invoke(ImReflect::ImInput_t, const char* label, vec3& value, ImSettings& settings, ImResponse& response) {
	// implementation...
}
)";
		static vec3 my_vec3;
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("Output:");
		ImReflect::Input("my_vec3", my_vec3);

		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Custom vec3 type - as color");
	HelpMarker("You can use type settings in your custom implementation to modify behavior.");
	{
		ImGui::PushID("vec3 color");

		const std::string code = R"(ImSettings config;
config.push<vec3>()
	.as_color()
.pop();

ImReflect::Input("my_vec3", my_vec3, config);)";

		ImSettings config;
		config.push<vec3>()
			.as_color()
			.pop();

		static vec3 my_vec3;
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("Output:");
		ImReflect::Input("my vec3", my_vec3, config);

		ImGui::PopID();
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Settings Test, disable, min_width
// ========================================
static void settings_test() {
	ImGui::SeparatorText("Settings Test");
	ImGui::PushID("settings_test");
	ImGui::Indent();

	static MyTypes my_struct;
	ImGui::Text("Disable member");
	HelpMarker("Disable a specific member variable");
	{
		ImGui::PushID("disable member");

		ImSettings config;
		config.push_member<&MyTypes::int_one>()
			.disable()
			.pop();

		const std::string code = R"(ImSettings config;
config.push_member<&MyTypes::int_one>()
	.disable()
.pop();)";

		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("int_one is now disabled\nOutput:");
		ImReflect::Input("my_struct", my_struct, config);

		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Disable type");
	HelpMarker("Disable all members of a specific type");
	{
		ImGui::PushID("disable type");

		ImSettings config;
		config.push<float>()
			.disable()
			.pop();
		const std::string code = R"(ImSettings config;
config.push<float>()
	.disable()
.pop();)";
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("All floats are now disabled\nOutput:");
		ImReflect::Input("my_struct", my_struct, config);

		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Min Width");
	HelpMarker("Set a minimum width for all members of a specific type");
	{
		ImGui::PushID("min width");
		ImSettings config;
		config.push<float>()
			.min_width(200.0f)
			.pop();
		const std::string code = R"(ImSettings config;
config.push<float>()
	.min_width(200.0f)
.pop();)";
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("All floats now have a minimum width of 200 pixels\nOutput:");
		ImReflect::Input("my_struct", my_struct, config);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Min Width Pair");
	HelpMarker("Set a minimum width for all members of a specific type");
	{
		ImGui::PushID("min width pair");
		static std::pair<int, float> my_pair = { 42, 3.14f };
		ImSettings config;
		config.push<std::pair>()
			.min_width(200.0f)
			.pop();
		const std::string code = R"(static std::pair<int, float> my_pair = { 42, 3.14f };
ImSettings config;
config.push<std::pair>()
	.min_width(200.0f)
.pop();)";
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("The pair items now has a minimum width of 200 pixels\nOutput:");
		ImReflect::Input("my_pair", my_pair, config);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Same line");
	HelpMarker("Put members on the same line");
	{
		ImGui::PushID("same line");

		ImSettings config;
		config.push_member<&MyTypes::int_one>()
			.same_line()
			.pop()
			.push_member<&MyTypes::float_one>()
			.same_line()
			.pop()
			.push_member<&MyTypes::bool_one>()
			.same_line()
			.pop();

		const std::string code = R"(ImSettings config;
config.push_member<&MyTypes::int_one>()
	.same_line()
.pop()
.push_member<&MyTypes::float_one>()
	.same_line()
.pop()
.push_member<&MyTypes::bool_one>()
	.same_line()
.pop();)";
		IMGUI_SAMPLE_MULTI_CODE(code);

		ImReflect::Input("same_line", my_struct, config);

		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Separator");
	HelpMarker("Add a separator between members");
	{
		ImGui::PushID("separator");

		ImSettings config;
		config.push_member<&MyTypes::int_two>()
			.separator()
			.pop()
			.push_member<&MyTypes::float_two>()
			.separator()
			.pop()
			.push_member<&MyTypes::bool_two>()
			.separator()
			.pop();


		const std::string code = R"(ImSettings config;
config.push_member<&MyTypes::int_two>()
	.separator()
.pop()
.push_member<&MyTypes::float_two>()
	.separator()
.pop()
.push_member<&MyTypes::bool_two>()
	.separator()
.pop();)";
		IMGUI_SAMPLE_MULTI_CODE(code);

		ImReflect::Input("separator", my_struct, config);

		ImGui::PopID();
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Result Test
// ========================================
static void result_test() {
	ImGui::SeparatorText("Result Test");
	ImGui::PushID("result_test");
	ImGui::Indent();

	ImGui::Text("ImResponse");
	HelpMarker("You can capture the ImResponse from the Input function to see if the value was changed");
	{
		ImGui::PushID("imresponse int");
		const std::string code = R"(static int my_int = 0;
ImResponse response = ImReflect::Input("my_int", my_int);
if (response.get<int>().is_changed()) {
	// value was changed
})";
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("Output:");
		static int my_int = 0;
		ImResponse response = ImReflect::Input("my_int", my_int);

		const auto& int_response = response.get<int>();
		ImGui::Text("is_changed: %s", int_response.is_changed() ? "true" : "false");
		ImGui::Text("is_hovered: %s", int_response.is_hovered() ? "true" : "false");
		ImGui::Text("is_active: %s", int_response.is_active() ? "true" : "false");
		ImGui::Text("is_focused: %s", int_response.is_focused() ? "true" : "false");
		ImGui::Text("is_clicked left: %s", int_response.is_clicked(0) ? "true" : "false");
		ImGui::Text("is_clicked right: %s", int_response.is_clicked(1) ? "true" : "false");
		ImGui::Text("is_clicked middle: %s", int_response.is_clicked(2) ? "true" : "false");
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("ImResponse - struct");
	HelpMarker("You can also capture the ImResponse for a struct, and check if specific members were changed");
	{
		ImGui::PushID("imresponse struct");
		const std::string code = R"(static MyStruct my_struct;
ImResponse response = ImReflect::Input("my_struct", my_struct);
const auto& struct_response = response.get<MyStruct>();
if (struct_response.get_member<&MyStruct::a>().is_changed()) {
	// member a was changed
})";
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("Output:");
		static MyStruct my_struct;
		ImResponse response = ImReflect::Input("my_struct", my_struct);
		ImGui::Text("a is_changed: %s", response.get_member<&MyStruct::a>().is_changed() ? "true" : "false");
		ImGui::Text("b is_changed: %s", response.get_member<&MyStruct::b>().is_changed() ? "true" : "false");
		ImGui::Text("c is_changed: %s", response.get_member<&MyStruct::c>().is_changed() ? "true" : "false");
		ImGui::PopID();
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::string
// ========================================
template<typename T>
static void string_test() {
	ImGui::SeparatorText("String Test");
	ImGui::PushID("string_test");
	ImGui::Indent();

	static T my_string = "Hello World";

	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_string", my_string);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Multiline default");
	HelpMarker("Multiline input, default settings");
	{
		ImGui::PushID("multiline default");

		ImSettings config;
		config.push<T>()
			.as_multiline()
			.pop();

		const std::string code = R"(ImSettings config;
config.push<std::string>()
	.as_multiline()
.pop();)";

		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("Output:");
		ImReflect::Input("my_string##multiline", my_string, config);

		ImGui::PopID();
	}

	ImGui::NewLine();
	ImGui::Text("Multiline specify height");
	HelpMarker("Multiline input, specify height in lines");
	{
		ImGui::PushID("multiline height");

		ImSettings config;
		config.push<T>()
			.as_multiline()
			.line_count(5) // 5 lines height
			.pop();

		const std::string code = R"(ImSettings config;
config.push<std::string>()
	.as_multiline()
	.line_count(5) // 5 lines height
.pop();)";

		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("Output:");
		ImReflect::Input("my_string##multiline height", my_string, config);

		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Multiline auto resize");
	HelpMarker("Multiline input, auto resize");
	{
		ImSettings config;
		config.push<T>()
			.as_multiline()
			.auto_resize()
			.pop();

		const std::string code = R"(ImSettings config;
config.push<std::string>()
	.as_multiline()
	.auto_resize()
.pop();)";

		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("Output:");
		ImReflect::Input("my_string##multiline auto resize", my_string, config);
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Smart pointers
// ========================================

static void smart_pointer_test() {
	// Shared, unique, weak
	ImGui::SeparatorText("Smart Pointer Test");
	ImGui::Indent();
	ImGui::PushID("smart_pointer_test");

	static std::shared_ptr<int> shared_ptr = std::make_shared<int>(42);

	ImGui::Text("std::shared_ptr");
	HelpMarker("Shared pointer, you can edit the value it points to");
	{
		ImGui::PushID("shared_ptr");
		ImReflect::Input("shared_ptr", shared_ptr);
		ImGui::PopID();
	}

	ImGui::NewLine();


	ImGui::Text("std::unique_ptr");
	HelpMarker("Unique pointer, you can edit the value it points to");
	{
		ImGui::PushID("unique_ptr");
		static std::unique_ptr<float> unique_ptr = std::make_unique<float>(3.14f);
		ImReflect::Input("unique_ptr", unique_ptr);
		ImGui::PopID();
	}

	ImGui::NewLine();


	ImGui::Text("std::weak_ptr");
	HelpMarker("Weak pointer, you can edit the value it points to if it is not expired");
	{
		ImGui::PushID("weak_ptr");
		static std::weak_ptr<int> weak_ptr = shared_ptr;
		ImReflect::Input("weak_ptr", weak_ptr);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("std::shared_ptr nullptr");
	HelpMarker("Shared pointer is nullptr, nothing to edit");
	{
		ImGui::PushID("shared_ptr nullptr");
		static std::shared_ptr<int> null_shared_ptr = nullptr;
		ImReflect::Input("null_shared_ptr", null_shared_ptr);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("std::unique_ptr nullptr");
	HelpMarker("Unique pointer is nullptr, nothing to edit");
	{
		ImGui::PushID("unique_ptr nullptr");
		static std::unique_ptr<float> null_unique_ptr = nullptr;
		ImReflect::Input("null_unique_ptr", null_unique_ptr);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("std::weak_ptr expired");
	HelpMarker("Weak pointer is expired, nothing to edit");
	{
		ImGui::PushID("weak_ptr expired");
		static std::weak_ptr<int> expired_weak_ptr;
		ImReflect::Input("expired_weak_ptr", expired_weak_ptr);
		ImGui::PopID();
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::pair
// ========================================
static void pair_test() {
	ImGui::SeparatorText("std::pair Test");
	ImGui::PushID("pair_test");
	ImGui::Indent();

	static std::pair<int, std::string> my_pair = { 42, "Hello" };

	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_pair", my_pair);
		ImGui::PopID();
	}

	ImGui::Text("Pair inside of pair");
	HelpMarker("You can also nest pairs");
	{
		ImGui::PushID("nested pair");
		static std::pair<std::pair<std::string, float>, int> nested_pair = { {"Nested", 3.14f}, 7 };

		ImSettings config;
		config.push<std::pair>()
			.as_dropdown(false)
			____.push<float>()
			________.min(0.0f)
			________.max(10.0f)
			________.as_slider()
			____.pop()
			____.push<std::string>()
			________.as_multiline()
			________.auto_resize()
			____.pop()
			.pop();

		ImReflect::Input("nested_pair", nested_pair, config);
		ImGui::PopID();
	}

	ImGui::Text("Pair of pairs");
	HelpMarker("You can also nest pairs");
	{
		ImGui::PushID("pair of pairs");
		static std::pair<std::pair<int, int>, std::pair<std::string, std::string>> pair_of_pairs = { {1, 2}, {"Hello", "World"} };
		ImReflect::Input("pair_of_pairs", pair_of_pairs);
		ImGui::PopID();
	}

	ImGui::Text("5 levels of pairs");
	HelpMarker("You can also nest pairs");
	{
		ImGui::PushID("5 levels of pairs");
		using pair5 = std::pair<int, std::pair<int, std::pair<int, std::pair<std::string, std::pair<int, int>>>>>;
		static pair5 five_levels_of_pairs = { 1, { 2, { 3, { "Deep", { 4, 5 } } } } };

		ImSettings config;
		config.push<std::string>()
			.as_multiline()
			.auto_resize()
			.pop()
			.push<std::pair>()
			.min_width(100.0f)
			.pop();

		ImReflect::Input("five_levels_of_pairs", five_levels_of_pairs, config);
		ImGui::PopID();
	}

	ImGui::Text("5 levels of pairs const");
	HelpMarker("You can also nest pairs");
	{
		ImGui::PushID("5 levels of pairs const");
		using pair5 = std::pair<int, std::pair<int, std::pair<int, std::pair<std::string, std::pair<int, int>>>>>;
		static const pair5 five_levels_of_pairs = { 1, { 2, { 3, { "Deep", { 4, 5 } } } } };
		ImSettings config;
		config.push<std::string>()
			.as_multiline()
			.auto_resize()
			.pop()
			.push<std::pair>()
			.min_width(100.0f)
			.pop();
		ImReflect::Input("five_levels_of_pairs_const", five_levels_of_pairs, config);
		ImGui::PopID();
	}


	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::tuple
// ========================================
static void tuple_test() {
	ImGui::SeparatorText("std::tuple Test");
	ImGui::PushID("tuple_test");
	ImGui::Indent();
	static std::tuple<int, float, std::string> my_tuple = { 42, 3.14f, "Hello" };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");

		ImSettings config;
		config.push<std::tuple>()
			.as_dropdown(true)
			.pop();

		ImReflect::Input("my_tuple", my_tuple, config);
		ImGui::PopID();
	}
	ImGui::Text("Tuple inside of tuple");
	HelpMarker("You can also nest tuples");
	{
		ImGui::PushID("nested tuple");
		static std::tuple<std::tuple<std::string, float>, int> nested_tuple = { {"Nested", 3.14f}, 7 };
		ImSettings config;
		config.push<std::tuple>()
			____.as_dropdown(false)
			____.push<float>()
			________.min(0.0f)
			________.max(10.0f)
			________.as_slider()
			____.pop()
			____.push<std::string>()
			________.as_multiline()
			________.auto_resize()
			____.pop()
			.pop();
		ImReflect::Input("nested_tuple", nested_tuple, config);
		ImGui::PopID();
	}
	ImGui::Text("Tuple of tuples");
	HelpMarker("You can also nest tuples");
	{
		ImGui::PushID("tuple of tuples");
		static std::tuple<std::tuple<int, int>, std::tuple<std::string, std::string>> tuple_of_tuples = { {1, 2}, {"Hello", "World"} };
		ImReflect::Input("tuple_of_tuples", tuple_of_tuples);
		ImGui::PopID();
	}
	ImGui::Text("5 levels of tuples");
	HelpMarker("You can also nest tuples");
	{
		ImGui::PushID("5 levels of tuples");
		using tuple5 = std::tuple<int, std::tuple<int, std::tuple<int, std::tuple<std::string, std::tuple<int, int>>>>>;
		static tuple5 five_levels_of_tuples = { 1, { 2, { 3, { "Deep", { 4, 5 } } } } };
		ImSettings config;
		config.push<std::string>()
			.as_multiline()
			.auto_resize()
			.pop()
			.push<std::tuple>()
			.as_dropdown(true)
			.pop();
		ImReflect::Input("five_levels_of_tuples", five_levels_of_tuples, config);
		ImGui::PopID();
	}
	ImGui::Text("Long tuple Line");
	HelpMarker("You can have tuples with many elements");
	{
		ImGui::PushID("long tuple");

		using LongTupleType = std::tuple<
			int, float, std::string, bool,
			int, float, std::string, bool,
			int, float, std::string, bool
		>;

		static LongTupleType long_tup = {
			1,     2.0f,   "Three",  true,
			5,     6.0f,   "Seven",  false,
			9,     10.0f,  "Eleven", true
		};

		ImSettings config;
		config.push<std::string>()
			.as_multiline()
			.auto_resize()
			.pop();

		ImReflect::Input("long_tuple", long_tup, config);
		ImGui::PopID();
	}
	ImGui::Text("Long tuple Grid");
	HelpMarker("You can have tuples with many elements");
	{
		ImGui::PushID("long tuple");

		using LongTupleType = std::tuple<
			int, float, std::string, bool,
			int, float, std::string, bool,
			int, float, std::string, bool
		>;

		static LongTupleType long_tup = {
			1,     2.0f,   "Three",  true,
			5,     6.0f,   "Seven",  false,
			9,     10.0f,  "Eleven", true
		};

		ImSettings config;
		config.push<std::tuple>()
			.as_grid()
			.columns(4)
			.min_width(100.0f)
			.pop();

		ImReflect::Input("long_tuple", long_tup, config);
		ImGui::PopID();
	}
	ImGui::Text("Long tuple Grid const");
	HelpMarker("You can have tuples with many elements");
	{
		ImGui::PushID("long tuple const");
		using LongTupleType = std::tuple<
			int, float, std::string, bool,
			int, float, std::string, bool,
			int, float, std::string, bool
		>;
		static const LongTupleType long_tup = {
			1,     2.0f,   "Three",  true,
			5,     6.0f,   "Seven",  false,
			9,     10.0f,  "Eleven", true
		};
		ImSettings config;
		config.push<std::tuple>()
			.as_grid()
			.columns(4)
			.min_width(100.0f)
			.pop();
		ImReflect::Input("long_tuple_const", long_tup, config);
		ImGui::PopID();
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::vector
// ========================================
static void vector_test() {
	ImGui::SeparatorText("std::vector Test");
	ImGui::PushID("vector_test");
	ImGui::Indent();
	static std::vector<int> my_vector = { 1, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_vector", my_vector);
		ImGui::PopID();
	}

	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::vector<int> my_vector_const = { 1, 2, 3, 4, 5 };
		ImReflect::Input("my_vector_const", my_vector_const);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Vector dropdown");
	HelpMarker("Vector with dropdown style");
	{
		ImGui::PushID("vector dropdown");
		ImSettings config;
		config.push<std::vector>()
			____.as_dropdown()
			____.push<int>()
			________.min(0)
			________.max(100)
			________.as_slider()
			____.pop()
			.pop();
		const std::string code = R"(ImSettings config;
config.push<std::vector>()
	.as_dropdown()
	.push<int>()
		.min(0)
		.max(100)
		.as_slider()
	.pop()
.pop();)";
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("Output:");
		ImReflect::Input("my_vector##dropdown", my_vector, config);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Vector pairs");
	HelpMarker("Vector of pairs");
	{
		ImGui::PushID("vector pairs");
		static std::vector<std::pair<std::string, float>> vector_of_pairs = {
			{"One", 1.0f},
			{"Two", 2.0f},
			{"Three", 3.0f}
		};
		ImReflect::Input("vector_of_pairs", vector_of_pairs);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Vector structs");
	HelpMarker("Vector of structs");
	{
		ImGui::PushID("vector structs");
		static std::vector<MyTypes> vector_of_structs = {
			{1, 2, 3.0f, 4.0f, true, false},
			{5, 6, 7.0f, 8.0f, false, true},
			{9, 10, 11.0f, 12.0f, true, true}
		};
		ImReflect::Input("vector_of_structs", vector_of_structs);
		ImGui::PopID();
	}

	ImGui::NewLine();

	ImGui::Text("Vector response");
	HelpMarker("You can capture the ImResponse when editing vectors");
	{
		ImGui::PushID("vector response");
		static std::vector<int> vector = { 1, 2, 3, 4, 5 };
		ImResponse response = ImReflect::Input("vector_response", vector);
		const auto& vec_response = response.get<std::vector>();

		ImGui::Text("Check Console!");
		if (vec_response.is_changed()) {
			printf("Vector was changed, new size: %zu\n", vector.size());
		}
		if (vec_response.has_inserted()) {
			printf("Vector has element inserted at index: %zu\n", vec_response.get_inserted_index());
		}
		if (vec_response.has_erased()) {
			printf("Vector has element erased at index: %zu\n", vec_response.get_erased_index());
		}
		if (vec_response.has_moved()) {
			auto move_info = vec_response.get_moved_info();
			printf("Vector has element moved from index %zu to index %zu\n", move_info.from, move_info.to);
		}



		ImGui::PopID();
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::Array
// ========================================
static void array_test() {
	ImGui::SeparatorText("std::array Test");
	ImGui::PushID("array_test");
	ImGui::Indent();
	static std::array<int, 5> my_array = { 1, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_array", my_array);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::array<int, 5> my_array_const = { 1, 2, 3, 4, 5 };
		ImReflect::Input("my_array_const", my_array_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::list
// ========================================
static void list_test() {
	ImGui::SeparatorText("std::list Test");
	ImGui::PushID("list_test");
	ImGui::Indent();
	static std::list<int> my_list = { 1, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_list", my_list);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::list<int> my_list_const = { 1, 2, 3, 4, 5 };
		ImReflect::Input("my_list_const", my_list_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::forward_list
// ========================================
static void forward_list_test() {
	ImGui::SeparatorText("std::forward_list Test");
	ImGui::PushID("forward_list_test");
	ImGui::Indent();
	static std::forward_list<int> my_forward_list = { 1, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_forward_list", my_forward_list);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::forward_list<int> my_forward_list_const = { 1, 2, 3, 4, 5 };
		ImReflect::Input("my_forward_list_const", my_forward_list_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::deque
// ========================================
static void deque_test() {
	ImGui::SeparatorText("std::deque Test");
	ImGui::PushID("deque_test");
	ImGui::Indent();
	static std::deque<int> my_deque = { 1, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_deque", my_deque);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::deque<int> my_deque_const = { 1, 2, 3, 4, 5 };
		ImReflect::Input("my_deque_const", my_deque_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::set
// ========================================
static void set_test() {
	ImGui::SeparatorText("std::set Test");
	ImGui::PushID("set_test");
	ImGui::Indent();
	static std::set<int> my_set = { 1, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_set", my_set);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::set<int> my_set_const = { 1, 2, 3, 4, 5 };
		ImReflect::Input("my_set_const", my_set_const);
		ImGui::PopID();
	}
	/* Empty set */
	ImGui::NewLine();
	ImGui::Text("Empty set");
	HelpMarker("An empty set to start with");
	{
		ImGui::PushID("empty set");
		static std::set<int> empty_set;
		ImReflect::Input("empty_set", empty_set);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::unordered_set
// ========================================
static void unordered_set_test() {
	ImGui::SeparatorText("std::unordered_set Test");
	ImGui::PushID("unordered_set_test");
	ImGui::Indent();
	static std::unordered_set<int> my_unordered_set = { 1, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_unordered_set", my_unordered_set);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::unordered_set<int> my_unordered_set_const = { 1, 2, 3, 4, 5 };
		ImReflect::Input("my_unordered_set_const", my_unordered_set_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::mulit_set
// ========================================
static void multiset_test() {
	ImGui::SeparatorText("std::multiset Test");
	ImGui::PushID("multiset_test");
	ImGui::Indent();
	static std::multiset<int> my_multiset = { 1, 2, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_multiset", my_multiset);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::multiset<int> my_multiset_const = { 1, 2, 2, 3, 4, 5 };
		ImReflect::Input("my_multiset_const", my_multiset_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::unordered_multiset
// ========================================
static void unordered_multiset_test() {
	ImGui::SeparatorText("std::unordered_multiset Test");
	ImGui::PushID("unordered_multiset_test");
	ImGui::Indent();
	static std::unordered_multiset<int> my_unordered_multiset = { 1, 2, 2, 3, 4, 5 };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_unordered_multiset", my_unordered_multiset);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::unordered_multiset<int> my_unordered_multiset_const = { 1, 2, 2, 3, 4, 5 };
		ImReflect::Input("my_unordered_multiset_const", my_unordered_multiset_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::map
// ========================================
static void map_test() {
	ImGui::SeparatorText("std::map Test");
	ImGui::PushID("map_test");
	ImGui::Indent();
	static std::map<int, float> my_map = {
		{1, 1.0f},
		{2, 2.0f},
		{3, 3.0f} };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_map", my_map);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::map<int, float> my_map_const = {
			{1, 1.0f},
			{2, 2.0f},
			{3, 3.0f} };
		ImReflect::Input("my_map_const", my_map_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::unordered_map
// ========================================
static void unordered_map_test() {
	ImGui::SeparatorText("std::unordered_map Test");
	ImGui::PushID("unordered_map_test");
	ImGui::Indent();
	static std::unordered_map<int, float> my_unordered_map = {
		{1, 1.0f},
		{2, 2.0f},
		{3, 3.0f} };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_unordered_map", my_unordered_map);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::unordered_map<int, float> my_unordered_map_const = {
			{1, 1.0f},
			{2, 2.0f},
			{3, 3.0f} };
		ImReflect::Input("my_unordered_map_const", my_unordered_map_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::multimap
// ========================================
static void multimap_test() {
	ImGui::SeparatorText("std::multimap Test");
	ImGui::PushID("multimap_test");
	ImGui::Indent();
	static std::multimap<int, float> my_multimap = {
		{1, 1.0f},
		{2, 2.0f},
		{2, 2.5f},
		{3, 3.0f} };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_multimap", my_multimap);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::multimap<int, float> my_multimap_const = {
			{1, 1.0f},
			{2, 2.0f},
			{2, 2.5f},
			{3, 3.0f} };
		ImReflect::Input("my_multimap_const", my_multimap_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::unordered_multimap
// ========================================
static void unordered_multimap_test() {
	ImGui::SeparatorText("std::unordered_multimap Test");
	ImGui::PushID("unordered_multimap_test");
	ImGui::Indent();
	static std::unordered_multimap<int, float> my_unordered_multimap = {
		{1, 1.0f},
		{2, 2.0f},
		{2, 2.5f},
		{3, 3.0f} };
	ImGui::Text("Default");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default");
		ImReflect::Input("my_unordered_multimap", my_unordered_multimap);
		ImGui::PopID();
	}
	ImGui::NewLine();
	ImGui::Text("Default const");
	HelpMarker("Default settings, no extra settings given");
	{
		ImGui::PushID("default const");
		static const std::unordered_multimap<int, float> my_unordered_multimap_const = {
			{1, 1.0f},
			{2, 2.0f},
			{2, 2.5f},
			{3, 3.0f} };
		ImReflect::Input("my_unordered_multimap_const", my_unordered_multimap_const);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::optional
// ========================================
static void optional_test() {
	ImGui::SeparatorText("std::optional Test");
	ImGui::PushID("optional_test");
	ImGui::Indent();

	static std::optional<int> my_optional = 42;

	ImGui::Text("std::optional<int>");
	HelpMarker("Optional integer, you can toggle if it has a value or not");
	{
		ImGui::PushID("optional int");
		ImReflect::Input("my_optional", my_optional);
		ImGui::PopID();
	}

	ImGui::NewLine();

	static std::optional<std::string> my_optional_string = "Hello World";

	ImGui::Text("std::optional<std::string>");
	HelpMarker("Optional string, you can toggle if it has a value or not");
	{
		ImGui::PushID("optional string");
		ImReflect::Input("my_optional_string", my_optional_string);
		ImGui::PopID();
	}

	ImGui::NewLine();

	static std::optional<MyTypes> my_optional_struct = MyTypes{ 1, 2, 3.0f, 4.0f, true, false };

	ImGui::Text("std::optional<Struct>");
	HelpMarker("Optional struct, you can toggle if it has a value or not");
	{
		ImGui::PushID("optional struct");
		ImReflect::Input("my_optional_struct", my_optional_struct);
		ImGui::PopID();
	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// std::variant
// ========================================
static void variant_test() {
	ImGui::SeparatorText("std::variant Test");
	ImGui::PushID("variant_test");
	ImGui::Indent();
	using MyVariantType = std::variant<int, float, std::string, MyTypes>;
	static MyVariantType my_variant = 42;
	ImGui::Text("std::variant<int, float, std::string, MyTypes>");
	HelpMarker("Variant that can hold int, float, string or MyTypes struct");
	{
		ImGui::PushID("variant");
		ImReflect::Input("my_variant", my_variant);
		ImGui::PopID();
	}
	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Complex Object Test
// ========================================
static void complex_object_test() {
	ImGui::SeparatorText("Complex Test");
	ImGui::PushID("Complex Test");

	ImGui::Text("std::map<std::string, std::deque<std::tuple<std::vector<std::pair<std::string, std::tuple<int, bool, double>>>, std::array<std::list<std::set<std::string>>, 4>, std::map<int, std::vector<std::optional<std::string>>>>>>; ");
	HelpMarker("Complex nested object test");

	using crazy =
		std::map<
		std::string,
		std::deque<
		std::tuple<
		std::vector<std::pair<std::string, std::tuple<int, bool, double>>>,
		std::array<std::list<std::set<std::string>>, 4>,
		std::map<int, std::vector<std::optional<std::string>>>
		>
		>
		>;

	static crazy my_crazy_variable = []() {
		crazy result;

		// ========== Entry 1: "player_data" ==========
		{
			std::deque<std::tuple<
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>>,
				std::array<std::list<std::set<std::string>>, 4>,
				std::map<int, std::vector<std::optional<std::string>>>
				>> player_deque;

			// First tuple in deque
			{
				// Vector of pairs: name -> (score, alive, multiplier)
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>> vec1 = {
					{"Alice", {1500, true, 2.5}},
					{"Bob", {2300, true, 1.8}},
					{"Charlie", {950, false, 0.0}},
					{"Diana", {3100, true, 3.2}}
				};

				// Array of 4 lists of sets
				std::array<std::list<std::set<std::string>>, 4> arr1;
				arr1[0] = { {"sword", "shield", "potion"} };
				arr1[1] = { {"first_kill", "level_10"}, {"speedrun", "pacifist"} };
				arr1[2] = { {"forest", "cave", "town"}, {"mountain", "desert"} };
				arr1[3] = { {"main_quest_1", "side_quest_3"} };

				// Map: player_id -> messages
				std::map<int, std::vector<std::optional<std::string>>> map1;
				map1[1001] = { std::string("Welcome!"), std::string("Good luck!"), std::nullopt };
				map1[1002] = { std::string("Hello there"), std::nullopt, std::string("Goodbye") };
				map1[1003] = { std::nullopt, std::string("Achievement unlocked!") };

				player_deque.push_back(std::make_tuple(vec1, arr1, map1));
			}

			// Second tuple in deque
			{
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>> vec2 = {
					{"Eve", {4200, true, 4.0}},
					{"Frank", {1800, true, 2.1}}
				};

				std::array<std::list<std::set<std::string>>, 4> arr2;
				arr2[0] = { {"axe", "bow"} };
				arr2[1] = { {"hunter"} };
				arr2[2] = { {"plains"} };
				arr2[3] = { {"tutorial_complete"} };

				std::map<int, std::vector<std::optional<std::string>>> map2;
				map2[2001] = { std::string("Level up!"), std::string("Nice work") };
				map2[2002] = { std::nullopt };

				player_deque.push_back(std::make_tuple(vec2, arr2, map2));
			}

			result["player_data"] = player_deque;
		}

		// ========== Entry 2: "game_state" ==========
		{
			std::deque<std::tuple<
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>>,
				std::array<std::list<std::set<std::string>>, 4>,
				std::map<int, std::vector<std::optional<std::string>>>
				>> game_deque;

			{
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>> vec = {
					{"current_level", {5, true, 1.5}},
					{"boss_defeated", {1, true, 10.0}},
					{"secrets_found", {7, false, 2.0}}
				};

				std::array<std::list<std::set<std::string>>, 4> arr;
				arr[0] = { {"checkpoint_1", "checkpoint_2", "checkpoint_3"} };
				arr[1] = { {"unlocked_hard_mode"}, {"found_easter_egg"} };
				arr[2] = { {"dungeon_1", "dungeon_2"} };
				arr[3] = { {"game_completed"} };

				std::map<int, std::vector<std::optional<std::string>>> map;
				map[9001] = { std::string("Game saved"), std::string("Continue?"), std::nullopt };
				map[9002] = { std::string("Boss battle started!") };

				game_deque.push_back(std::make_tuple(vec, arr, map));
			}

			result["game_state"] = game_deque;
		}

		// ========== Entry 3: "inventory_system" ==========
		{
			std::deque<std::tuple<
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>>,
				std::array<std::list<std::set<std::string>>, 4>,
				std::map<int, std::vector<std::optional<std::string>>>
				>> inv_deque;

			// First tuple
			{
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>> vec1 = {
					{"health_potions", {10, true, 1.0}},
					{"mana_potions", {5, true, 1.0}},
					{"gold_coins", {999, true, 1.0}},
					{"rare_gems", {3, false, 5.0}}
				};

				std::array<std::list<std::set<std::string>>, 4> arr1;
				arr1[0] = { {"consumable", "stackable"} };
				arr1[1] = { {"common", "rare"}, {"legendary"} };
				arr1[2] = { {"vendor_1", "vendor_2", "vendor_3"} };
				arr1[3] = { {"tradeable", "sellable", "droppable"} };

				std::map<int, std::vector<std::optional<std::string>>> map1;
				map1[101] = { std::string("Inventory full!"), std::nullopt };
				map1[102] = { std::string("Item acquired"), std::string("Check your bag") };
				map1[103] = { std::nullopt, std::nullopt, std::string("Trade complete") };

				inv_deque.push_back(std::make_tuple(vec1, arr1, map1));
			}

			// Second tuple
			{
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>> vec2 = {
					{"enchanted_sword", {1, true, 100.0}},
					{"magic_staff", {1, true, 85.5}}
				};

				std::array<std::list<std::set<std::string>>, 4> arr2;
				arr2[0] = { {"weapon", "enchanted"} };
				arr2[1] = { {"epic"} };
				arr2[2] = { {"blacksmith"} };
				arr2[3] = { {"upgradeable"} };

				std::map<int, std::vector<std::optional<std::string>>> map2;
				map2[201] = { std::string("Weapon equipped") };
				map2[202] = { std::string("Damage increased!") };

				inv_deque.push_back(std::make_tuple(vec2, arr2, map2));
			}

			result["inventory_system"] = inv_deque;
		}

		// ========== Entry 4: "network_stats" ==========
		{
			std::deque<std::tuple<
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>>,
				std::array<std::list<std::set<std::string>>, 4>,
				std::map<int, std::vector<std::optional<std::string>>>
				>> net_deque;

			{
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>> vec = {
					{"ping_ms", {45, true, 1.0}},
					{"packet_loss", {2, false, 0.5}},
					{"bandwidth_mbps", {100, true, 1.0}},
					{"players_online", {1247, true, 1.0}}
				};

				std::array<std::list<std::set<std::string>>, 4> arr;
				arr[0] = { {"server_1", "server_2", "server_3"} };
				arr[1] = { {"NA", "EU"}, {"ASIA"} };
				arr[2] = { {"tcp", "udp"} };
				arr[3] = { {"connected", "stable"} };

				std::map<int, std::vector<std::optional<std::string>>> map;
				map[5001] = { std::string("Connection stable"), std::nullopt };
				map[5002] = { std::nullopt, std::string("Lag detected") };
				map[5003] = { std::string("Reconnecting..."), std::nullopt, std::string("Success") };

				net_deque.push_back(std::make_tuple(vec, arr, map));
			}

			result["network_stats"] = net_deque;
		}

		// ========== Entry 5: "achievement_tracker" ==========
		{
			std::deque<std::tuple<
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>>,
				std::array<std::list<std::set<std::string>>, 4>,
				std::map<int, std::vector<std::optional<std::string>>>
				>> ach_deque;

			// First tuple
			{
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>> vec1 = {
					{"total_achievements", {50, true, 1.0}},
					{"unlocked", {23, true, 1.0}},
					{"hidden_found", {5, false, 2.0}},
					{"completion_rate", {46, true, 1.0}}
				};

				std::array<std::list<std::set<std::string>>, 4> arr1;
				arr1[0] = { {"bronze", "silver", "gold", "platinum"} };
				arr1[1] = { {"combat", "exploration"}, {"social", "crafting"} };
				arr1[2] = { {"easy", "medium", "hard", "impossible"} };
				arr1[3] = { {"unlocked", "locked", "secret"} };

				std::map<int, std::vector<std::optional<std::string>>> map1;
				map1[7001] = { std::string("Achievement unlocked!"), std::string("Congratulations") };
				map1[7002] = { std::string("Progress: 80%"), std::nullopt };
				map1[7003] = { std::nullopt, std::nullopt, std::string("New achievement available") };

				ach_deque.push_back(std::make_tuple(vec1, arr1, map1));
			}

			// Second tuple
			{
				std::vector<std::pair<std::string, std::tuple<int, bool, double>>> vec2 = {
					{"speedrun_time", {3600, true, 1.5}},
					{"deaths", {127, false, 1.0}}
				};

				std::array<std::list<std::set<std::string>>, 4> arr2;
				arr2[0] = { {"timed", "ranked"} };
				arr2[1] = { {"competitive"} };
				arr2[2] = { {"leaderboard"} };
				arr2[3] = { {"verified"} };

				std::map<int, std::vector<std::optional<std::string>>> map2;
				map2[7101] = { std::string("World record!"), std::string("Amazing!") };
				map2[7102] = { std::string("Keep trying") };

				ach_deque.push_back(std::make_tuple(vec2, arr2, map2));
			}

			result["achievement_tracker"] = ach_deque;
		}

		return result;
		}();

	// Use with ImReflect
	ImReflect::Input("my_crazy_variable", my_crazy_variable);

	ImGui::PopID();
}

// ========================================
// Main
// ========================================
namespace svh {
	void imgui_app::render() {

		ImGui::BeginTabBar("MainTab");
		if (ImGui::BeginTabItem("Read Me")) {

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Primitive Types")) {

			// Integer types
			primitive_test<int8_t>();
			primitive_test<const int8_t>();
			primitive_test<uint8_t>();
			primitive_test<const uint8_t>();
			primitive_test<int16_t>();
			primitive_test<const int16_t>();
			primitive_test<uint16_t>();
			primitive_test<const uint16_t>();
			primitive_test<int32_t>();
			primitive_test<const int32_t>();
			primitive_test<uint32_t>();
			primitive_test<const uint32_t>();
			primitive_test<int64_t>();
			primitive_test<const int64_t>();
			primitive_test<uint64_t>();
			primitive_test<const uint64_t>();

			// Floating-point types
			primitive_test<float>();
			primitive_test<const float>();
			primitive_test<double>();
			primitive_test<const double>();
			primitive_test<long double>();
			primitive_test<const long double>();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Format")) {
			// Format test
			format_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Booleans")) {

			// bool
			boolean_test<bool>();
			boolean_test<const bool>();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Enums")) {

			// Enum
			enum_test<MyEnum>();
			enum_test<const MyEnum>();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Pointers")) {

			pointer_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Reflection")) {

			// Struct / Class reflection
			simple_reflection_test();
			ImGui::NewLine();
			ImGui::NewLine();
			nested_reflection_test();
			ImGui::NewLine();
			ImGui::NewLine();
			type_settings_test();
			ImGui::NewLine();
			ImGui::NewLine();
			member_settings_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Settings")) {

			// Settings test
			settings_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Result Test")) {

			// Result Test
			result_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Custom Implementation")) {

			// Custom Implementation test
			implementation_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("String")) {

			// String test
			string_test<std::string>();
			string_test<const std::string>();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Smart Pointers")) {

			// Smart Pointer test
			smart_pointer_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Pair")) {

			// Pair test
			pair_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Tuple")) {

			// Tuple test
			tuple_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Vector")) {

			// Vector test
			vector_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Array")) {

			// Array test
			array_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("List")) {

			// List test
			list_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Forward List")) {

			// Forward List test
			forward_list_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Deque")) {

			// Deque test
			deque_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Set")) {
			// Set test
			set_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Unordered Set")) {
			// Unordered Set test
			unordered_set_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("MultiSet")) {
			// MultiSet test
			multiset_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Unordered MultiSet")) {
			// Unordered MultiSet test
			unordered_multiset_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Map")) {
			// Map test
			map_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Unordered Map")) {
			// Unordered Map test
			unordered_map_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("MultiMap")) {
			// MultiMap test
			multimap_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Unordered MultiMap")) {
			// Unordered MultiMap test
			unordered_multimap_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Optional")) {
			// Optional test
			optional_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Variant")) {
			// Variant test
			variant_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Complex Object")) {
			// Complex Object test
			complex_object_test();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}