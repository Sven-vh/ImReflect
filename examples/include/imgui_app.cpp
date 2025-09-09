#include "imgui_app.hpp"
#include <imgui/imgui.h>
#include <svh/imgui_reflect.hpp>

struct MyStruct {
	int a;
	float b;
	bool c;
};

template<>
struct type_settings<int> : svh::scope {
	int _min = 0;
	int _max = 100;

	type_settings& min(const int v) { _min = v; return *this; }
	type_settings& max(const int v) { _max = v; return *this; }

	int get_min() const { return _min; }
	int get_max() const { return _max; }
};

namespace svh {
	void imgui_app::render() {

		static MyStruct s{ 1, 2.0f, true };

		auto& settings = svh::scope()
			.push<int>()
			____.min(10)
			____.max(50)
			.pop();

		ImGui::Reflect::Input("Test", s, settings);

	}
}