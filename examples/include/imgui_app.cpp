#include "imgui_app.hpp"
#include <imgui/imgui.h>

#include <imgui_reflect.hpp>

typedef uint32_t my_special_int;

struct MyStruct {
	int a;
	int b;
	float c;
};

IMGUI_REFLECT(MyStruct, a, b, c)

namespace svh {
	void imgui_app::render() {

		static MyStruct s{ 1, 2 };

		
		auto settings = ImSettings();
#if 1
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
			____.as_input()
			//____.password()
			.pop()
			.push_member<&MyStruct::c>()
			____.min(0.0f)
			____.max(1.0f)
			____.clamp()
			____.as_slider()
			____.as_float(5)
			____.logarithmic()
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
		ImResponse response = ImGui::Reflect::Input("Test", s, settings);

		//printf("\nSettings after: \n");
		//settings.debug_log();
		//printf("\nResponse: \n");
		//response.debug_log();

		if (response.get<MyStruct>().is_changed()) {
			printf("MyStruct changed\n");
		}
		if (response.get<MyStruct>().get<int>().is_changed()) {
			printf("int changed\n");
		}
		if (response.get_member<&MyStruct::a>().is_changed()) printf("a changed\n");
		if (response.get_member<&MyStruct::a>().is_hovered()) printf("a hovered\n");
		if (response.get_member<&MyStruct::a>().is_active()) printf("a active\n");
		if (response.get_member<&MyStruct::a>().is_activated()) printf("a activated\n");
		if (response.get_member<&MyStruct::a>().is_deactivated()) printf("a deactivated\n");
		if (response.get_member<&MyStruct::a>().is_deactivated_after_edit()) printf("a deactivated after edit\n");
		if (response.get_member<&MyStruct::a>().is_clicked(ImGuiMouseButton_Left)) printf("a clicked left\n");
		if (response.get_member<&MyStruct::a>().is_double_clicked(ImGuiMouseButton_Left)) printf("a double clicked left\n");
		if (response.get_member<&MyStruct::a>().is_focused()) printf("a focused\n");
		

	}
}