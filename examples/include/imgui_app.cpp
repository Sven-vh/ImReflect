#include "imgui_app.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>

#include <ImReflect.hpp>



//template<>
//struct ImReflect::type_settings<MyStruct> : ImSettings {
//private:
//	bool _a_setting = false;
//	int _b_setting = 42;
//
//public:
//	type_settings<MyStruct>& a_setting(bool v) { _a_setting = v; RETURN_THIS_T(MyStruct); }
//	type_settings<MyStruct>& b_setting(int v) { _b_setting = v; RETURN_THIS_T(MyStruct); }
//};

//enum class Difficulty {
//	Easy,
//	Medium,
//	Hard
//};
//
//struct GameSettings {
//	int			volume = 50;
//	//float		sensitivity = 1.0f;
//	bool		fullscreen = false;
//	Difficulty	difficulty = Difficulty::Medium;
//};
//IMGUI_REFLECT(GameSettings, volume, /*sensitivity,*/ fullscreen, difficulty)

//struct vec3 {
//	float x, y, z;
//};
//
//namespace MyNamespace {
//	struct Transform {
//		vec3 position;
//		vec3 rotation;
//		vec3 scale;
//		std::string name = "Foo";
//	};
//}
//
//template<>
//struct ImReflect::type_settings<MyNamespace::Transform> : ImSettings {
//private:
//	bool _change_name = false;
//
//public:
//	type_settings<MyNamespace::Transform>& change_name(bool value) {
//		_change_name = value;
//		RETURN_THIS_T(MyNamespace::Transform);
//	}
//
//	bool get_change_name() const {
//		return _change_name;
//	}
//};
//
//void tag_invoke(ImReflect::ImInput_t, const char* label, MyNamespace::Transform& value, ImSettings& settings, ImResponse& response) {
//	auto& transform_settings = settings.get<MyNamespace::Transform>();
//
//	ImGui::SeparatorText(label);
//
//	if (transform_settings.get_change_name()) {
//		//ImGui::InputText("Name", &value.name);
//	} else {
//		ImGui::Text(value.name.c_str());
//	}
//
//	ImGui::InputFloat3("Position", &value.position.x);
//	ImGui::InputFloat3("Rotation", &value.rotation.x);
//	ImGui::InputFloat3("Scale", &value.scale.x);
//}

//template<>
//struct ImReflect::type_settings<int> : ImSettings {
//private:
//	int _multiplier = 1;
//public:
//	type_settings<int>& multiplier(int v) { _multiplier = v; RETURN_THIS_T(int); }
//	int get_multiplier() const { return _multiplier; }
//};
//
//void tag_invoke(ImReflect::ImInput_t, const char* label, int& value, ImSettings& settings, ImResponse& response) {
//	ImGui::Text(label);
//	ImGui::InputInt(label, &value);
//}
//
//static_assert(svh::is_tag_invocable<ImReflect::ImInput_t, const char*, int&, ImReflect::ImSettings&, ImReflect::ImResponse&>::value, "Transform does not have tag_invoke for ImInput_t");

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
			.format("%.3f")
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

struct Bar {
	int x;
	float y;
	bool z;
};
IMGUI_REFLECT(Bar, x, y, z)

struct Foo {
	int a;
	float b;
	bool c;
	Bar bar;
};
IMGUI_REFLECT(Foo, a, b, c, bar)

static void nested_reflection_test() {
	ImGui::SeparatorText("Nested Struct / Class Reflection");
	ImGui::PushID("nested reflection");
	ImGui::Indent();

	static Foo foo;

	ImGui::Text("Nested Structs");
	HelpMarker("You need to define the ``IMGUI_REFLECT`` macro for each struct, this determines what is reflected");
	{
		const std::string code = R"(struct Bar {
	int x;
	float y;
	bool z;
};
IMGUI_REFLECT(Bar, x, y, z)

struct Foo {
	int a;
	float b;
	bool c;
	Bar bar;
};
IMGUI_REFLECT(Foo, a, b, c, bar))";

		IMGUI_SAMPLE_MULTI_CODE(code);

		ImGui::Text("Output:");
		ImReflect::Input("foo", foo);
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
		config.push<ImReflect::std_pair>()
			.min_width(200.0f)
			.pop();
		const std::string code = R"(static std::pair<int, float> my_pair = { 42, 3.14f };
ImSettings config;
config.push<ImReflect::std_pair>()
	.min_width(200.0f)
.pop();)";
		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("The pair items now has a minimum width of 200 pixels\nOutput:");
		ImReflect::Input("my_pair", my_pair, config);
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
		config.push<ImReflect::std_pair>()
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
			.push<ImReflect::std_pair>()
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
			.push<ImReflect::std_pair>()
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
		config.push<ImReflect::std_tuple>()
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
		config.push<ImReflect::std_tuple>()
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
			.push<ImReflect::std_tuple>()
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
		config.push<ImReflect::std_tuple>()
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
		config.push<ImReflect::std_tuple>()
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
		config.push<ImReflect::std_vector>()
			.as_dropdown()
			.pop();
		const std::string code = R"(ImSettings config;
config.push<ImReflect::std_vector>()
	.as_dropdown()
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
// Main
// =======================================
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
		if (ImGui::BeginTabItem("String")) {

			// String test
			string_test<std::string>();
			string_test<const std::string>();

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
		if (ImGui::BeginTabItem("MultiSet")) {
			// MultiSet test
			multiset_test();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Map")) {
			// Map test
			map_test();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}