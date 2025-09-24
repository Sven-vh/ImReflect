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
	ImGui::InputTextMultiline("##"#x, &input, ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AllowTabInput); \
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
	ImGui::SeparatorText(info.name());
	ImGui::PushID(info.name());
	ImGui::Indent();

	const std::string type_name = info.name();
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
// Boolean Test
// ========================================
static void boolean_test() {
	ImGui::SeparatorText("bool");
	ImGui::PushID("bool");
	ImGui::Indent();

	static bool my_bool;

	ImGui::Text("Default - as_checkbox()");
	HelpMarker("Default settings, no extra settings given");
	{
		ImReflect::Input("my_bool", my_bool);
	}

	ImGui::NewLine();

	ImGui::Text("Radio - as_radio()");
	HelpMarker("Radio button style");
	{
		auto config = ImSettings();
		config.push<bool>()
			.as_radio()
			.pop();
		ImReflect::Input("my_bool##radio", my_bool, config);
	}

	ImGui::Text("Radio - as_radio() - custom text");
	HelpMarker("Radio button style with custom text for true/false states");
	{
		auto config = ImSettings();
		config.push<bool>()
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
		config.push<bool>()
			.as_button()
			.pop();
		ImReflect::Input("my_bool##button", my_bool, config);
	}

	ImGui::Text("Button - as_button() - custom text");
	HelpMarker("Button style with custom text for true/false states");
	{
		auto config = ImSettings();
		config.push<bool>()
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
		config.push<bool>()
			.as_dropdown()
			.pop();
		ImReflect::Input("my_bool##dropdown", my_bool, config);
	}

	ImGui::Text("Button - as_dropdown() - custom text");
	HelpMarker("Dropdown style with custom text for true/false states");
	{
		auto config = ImSettings();
		config.push<bool>()
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

static void enum_test() {
	ImGui::SeparatorText("enum");
	ImGui::PushID("enum");
	ImGui::Indent();

	static MyEnum my_enum;

	ImGui::Text("Default - as_dropdown()");
	HelpMarker("Default settings, no extra settings given");
	{
		ImReflect::Input("my_enum", my_enum);
	}

	ImGui::NewLine();

	ImGui::Text("Radio - as_radio()");
	HelpMarker("Radio button style");
	{
		auto config = ImSettings();
		config.push<MyEnum>()
			.as_radio()
			.pop();
		ImReflect::Input("my_enum##radio", my_enum, config);
	}

	ImGui::NewLine();

	ImGui::Text("Slider - as_slider()");
	HelpMarker("Slider style");
	{
		auto config = ImSettings();
		config.push<MyEnum>()
			.as_slider()
			.pop();
		ImReflect::Input("my_enum##slider", my_enum, config);
	}

	ImGui::NewLine();

	ImGui::Text("Drag - as_drag()");
	HelpMarker("Drag style");
	{
		auto config = ImSettings();
		config.push<MyEnum>()
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
	int a;
	float b;
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
.pop();)";

		IMGUI_SAMPLE_MULTI_CODE(code);
		ImGui::Text("int_one is now a slider between 0 and 10\nint_two is now an input between -50 and 50\nOutput:");
		ImReflect::Input("my_struct", my_struct, config);

	}

	ImGui::Unindent();
	ImGui::PopID();
}

// ========================================
// Settings Test, disable, slider settings, ...
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
			primitive_test<uint8_t>();
			primitive_test<int16_t>();
			primitive_test<uint16_t>();
			primitive_test<int32_t>();
			primitive_test<uint32_t>();
			primitive_test<int64_t>();
			primitive_test<uint64_t>();

			// Floating-point types
			primitive_test<float>();
			primitive_test<double>();
			primitive_test<long double>();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Booleans")) {

			// bool
			boolean_test();

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Enums")) {

			// Enum
			enum_test();

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

		ImGui::EndTabBar();
	}
}