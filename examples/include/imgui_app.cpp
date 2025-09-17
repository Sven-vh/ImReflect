#include "imgui_app.hpp"
#include <imgui/imgui.h>

#include <imgui_reflect.hpp>

struct OtherStruct {
	char foo;
	uint_least64_t bar;
};
IMGUI_REFLECT(OtherStruct, foo, bar)

struct MyStruct {
	int a;
	float b;
	bool c = false;
	OtherStruct d;
};
IMGUI_REFLECT(MyStruct, a, b, c, d)

template<>
struct ImGui::Reflect::type_settings<MyStruct> : ImSettings {
private:
	bool _a_setting = false;
	int _b_setting = 42;

public:
	type_settings<MyStruct>& a_setting(bool v) { _a_setting = v; RETURN_THIS_T(MyStruct); }
	type_settings<MyStruct>& b_setting(int v) { _b_setting = v; RETURN_THIS_T(MyStruct); }
};

enum class MyEnum {
	Option1,
	Option2,
	Option3
};

struct EnumHolder {
	MyEnum a;
	MyEnum b;
	MyEnum c;
	MyEnum d;
};
IMGUI_REFLECT(EnumHolder, a, b, c, d)

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

		static EnumHolder e;
		ImGui::Reflect::Input("Enum Test", e, root);
	}
}


void func() {
	{



		auto settings = ImSettings();
#if 0
		settings.push_member<&MyStruct::a>()
			____.min(0)
			____.max(255)
			____.as_slider()
			____.clamp()
			____.as_hex()
			.pop()
			.push_member<&MyStruct::b>()
			____.min(-10)
			____.max(10)
			____.clamp()
			____.as_input()
			.pop()
			.push_member<&MyStruct::c>()
			____.min(0.0f)
			____.max(1.0f)
			____.clamp()
			____.as_slider()
			____.as_float(5)
			____.logarithmic()
			.pop()
			.push<MyStruct>()
			____.push<int>()
			________.min(0)
			________.max(100)
			________.as_slider()
			________.clamp()
			____.pop()
			.pop()
			.push_member<&MyStruct::d>()
			____.as_checkbox()
			.pop()
			.push_member<&MyStruct::e>()
			____.as_radio()
			.pop()
			.push_member<&MyStruct::f>()
			____.as_button()
			.pop()
			.push_member<&MyStruct::g>()
			____.as_dropdown()
			____.true_text("Yes")
			____.false_text("No")
			.pop();
#else
		settings.push<int>()
			____.min(0)
			____.max(255)
			____.as_slider()
			____.clamp()
			.pop()
			.push<float>()
			____.min(0.0f)
			____.max(1.0f)
			____.as_float(10)
			____.as_slider()
			____.logarithmic()
			.pop();

#endif

		//printf("\nSettings before: \n");
		//settings.debug_log();

		//printf("\nSettings after: \n");
		//settings.debug_log();
		//printf("\nResponse: \n");
		//response.debug_log();

		//if (response.get<MyStruct>().is_changed()) {
		//	printf("MyStruct changed\n");
		//}
		//if (response.get<MyStruct>().get<int>().is_changed()) {
		//	printf("int changed\n");
		//}
		//if (response.get_member<&MyStruct::a>().is_changed()) printf("a changed\n");
		//if (response.get_member<&MyStruct::a>().is_hovered()) printf("a hovered\n");
		//if (response.get_member<&MyStruct::a>().is_active()) printf("a active\n");
		//if (response.get_member<&MyStruct::a>().is_activated()) printf("a activated\n");
		//if (response.get_member<&MyStruct::a>().is_deactivated()) printf("a deactivated\n");
		//if (response.get_member<&MyStruct::a>().is_deactivated_after_edit()) printf("a deactivated after edit\n");
		//if (response.get_member<&MyStruct::a>().is_clicked(ImGuiMouseButton_Left)) printf("a clicked left\n");
		//if (response.get_member<&MyStruct::a>().is_double_clicked(ImGuiMouseButton_Left)) printf("a double clicked left\n");
		//if (response.get_member<&MyStruct::a>().is_focused()) printf("a focused\n");
		//

	}
}