#include "imgui_app.hpp"
#include <imgui/imgui.h>

#include <imgui_reflect.hpp>

typedef uint32_t my_special_int;

struct MyStruct {
	my_special_int a;
	int b;
};

IMGUI_REFLECT(MyStruct, a, b)

namespace svh {
	void imgui_app::render() {

		static MyStruct s{ 1, 2 };

		auto settings = ImSettings();
		settings.push_member<&MyStruct::a>()
			.min(0)
			.pop()
			.push_member<&MyStruct::b>()
			//.min(-10)
			//.max(10)
			.pop();

		ImResponse response = ImGui::Reflect::Input("Test", s, settings);
	}
}