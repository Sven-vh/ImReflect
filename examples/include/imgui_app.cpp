#include "imgui_app.hpp"
#include <imgui.h>

#include <ImReflect.hpp>

struct OtherStruct {
	char foo;
	uint_least64_t bar;
};
IMGUI_REFLECT(OtherStruct, foo, bar)

struct MyStruct {
	int a;
	float b;
	bool c = false;
	OtherStruct other_struct;
};
IMGUI_REFLECT(MyStruct, a, b, c, other_struct)

template<>
struct ImReflect::type_settings<MyStruct> : ImSettings {
private:
	bool _a_setting = false;
	int _b_setting = 42;

public:
	type_settings<MyStruct>& a_setting(bool v) { _a_setting = v; RETURN_THIS_T(MyStruct); }
	type_settings<MyStruct>& b_setting(int v) { _b_setting = v; RETURN_THIS_T(MyStruct); }
};

enum class Difficulty {
	Easy,
	Medium,
	Hard
};

struct GameSettings {
	int			volume = 50;
	float		sensitivity = 1.0f;
	bool		fullscreen = false;
	Difficulty	difficulty = Difficulty::Medium;
};
IMGUI_REFLECT(GameSettings, volume, sensitivity, fullscreen, difficulty)

// 20 options
enum class MyEnum {
	Option1,
	Option2,
	Option3,
	Option4,
	Option5,
	Option6,
	Option7,
	Option8,
	Option9,
	Option10,
	Option11,
	Option12,
	Option13,
	Option14,
	Option15,
	Option16,
	Option17,
	Option18,
	Option19,
	Option20
};

struct EnumHolder {
	MyEnum a;
	MyEnum b;
	MyEnum c;
	MyEnum d;
};
IMGUI_REFLECT(EnumHolder, a, b, c, d)

struct vec3 {
	float x, y, z;
};

namespace MyNamespace {
	struct Transform {
		vec3 position;
		vec3 rotation;
		vec3 scale;
		std::string name = "Foo";
	};
}

template<>
struct ImReflect::type_settings<MyNamespace::Transform> : ImSettings {
private:
	bool _change_name = false;

public:
	type_settings<MyNamespace::Transform>& change_name(bool value) {
		_change_name = value;
		RETURN_THIS_T(MyNamespace::Transform);
	}

	bool get_change_name() const {
		return _change_name;
	}
};

void tag_invoke(ImReflect::ImInput_t, const char* label, MyNamespace::Transform& value, ImSettings& settings, ImResponse& response) {
	auto& transform_settings = settings.get<MyNamespace::Transform>();

	ImGui::SeparatorText(label);

	if (transform_settings.get_change_name()) {
		//ImGui::InputText("Name", &value.name);
	} else {
		ImGui::Text(value.name.c_str());
	}

	ImGui::InputFloat3("Position", &value.position.x);
	ImGui::InputFloat3("Rotation", &value.rotation.x);
	ImGui::InputFloat3("Scale", &value.scale.x);
}

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

namespace svh {
	void imgui_app::render() {
		auto root = ImSettings();
		root.push_member<&EnumHolder::a>()
			.as_radio()
			.pop()
			.push_member<&EnumHolder::b>()
			.as_slider()
			.pop()
			.push_member<&EnumHolder::c>()
			.as_drag()
			.pop()
			.push_member<&EnumHolder::d>()
			.as_dropdown()
			.pop();

		static GameSettings settings;
		ImReflect::Input("Game Settings", settings);

		auto config = ImSettings();
		config.push<MyNamespace::Transform>()
				.change_name(true)
			.pop();

		static MyNamespace::Transform t;
		ImReflect::Input("My Transform", t, config);

		static MyStruct my_struct;
		ImReflect::Input("My Struct", my_struct);
	}
}