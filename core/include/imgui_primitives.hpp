#pragma once
#include <imgui/imgui.h>
#include "imgui_input.hpp"

/* Input fields for primitive types */
namespace ImGui {
	namespace Reflect {

		template<>
		struct type_settings<int> : svh::scope {
			int _min = 0;
			int _max = 100;

			type_settings& min(const int v) { _min = v; return *this; }
			type_settings& max(const int v) { _max = v; return *this; }

			int get_min() const { return _min; }
			int get_max() const { return _max; }
		};


		inline void tag_invoke(ImInputLib_t, const char* label, int& value, ImSettings& settings, ImResponse& response) {
			auto& int_settings = settings.get<int>();
			ImGui::SliderInt(label, &value, int_settings.get_min(), int_settings.get_max());
		}
	}
}

static_assert(svh::is_tag_invocable_v<ImGui::Reflect::ImInputLib_t, const char*, int&, ImSettings&, ImResponse&>, "int tag_invoke not found");