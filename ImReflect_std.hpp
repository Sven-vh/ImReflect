#pragma once
#include <imgui.h>
#include <imgui_stdlib.h>
#include "ImReflect_helper.hpp"

/* Helpers */
namespace ImReflect::Detail {
	template<typename T> /* All numbers except bool */
	constexpr bool is_string_type_v = std::is_same_v<T, std::string>;
	template<typename T>
	using enable_if_string_t = std::enable_if_t<is_string_type_v<T>, void>;
}

/* Generic settings for types */
namespace ImReflect::Detail {
	/* Setting to be able to color text field*/
	template<typename T>
	struct text_input {
	private:
		bool _multi_line = false; /* single line or multi line input */
		int _line_count = -1; /* -1 = imgui default, 0 = auto resize, >0 line height*/

	public:
		type_settings<T>& as_multiline() { _multi_line = true; RETURN_THIS; }
		type_settings<T>& auto_resize() { _multi_line = true; _line_count = 0; RETURN_THIS; }
		/* -1 = imgui default, 0 = auto resize, >0 line height*/
		type_settings<T>& line_count(int count) { _multi_line = true; _line_count = count; RETURN_THIS; }

		type_settings<T>& as_singleline() { _multi_line = false; _line_count = -1; RETURN_THIS; }

		bool is_multiline() const { return _multi_line; }
		int get_line_count() const { return _line_count; }
	};

}

/* Input fields for std types */
namespace ImReflect {
	/* ========================= std::string ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_string_t<T>> : ImSettings,
		ImReflect::Detail::required<std::string>,
		ImReflect::Detail::input_flags<std::string>,
		ImReflect::Detail::text_input<std::string> {
	};

	template<typename T>
	std::enable_if_t<Detail::is_string_type_v<T>>
		tag_invoke(Detail::ImInputLib_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
		type_settings<T>& string_settings = settings.get<T>();
		type_response<T>& string_response = response.get<T>();

		const bool multi_line = string_settings.is_multiline();
		const auto& flags = string_settings.get_input_flags();

		bool changed = false;
		if (multi_line) {
			const int line_height = string_settings.get_line_count();
			ImVec2 size;
			if (line_height < 0) {
				/* imgui default */
				size = ImVec2(0, 0);
			} else if (line_height == 0) {
				/* auto resize, calculate height based on number of lines in string */
				int lines = std::count(value.begin(), value.end(), '\n') + 1;
				lines = std::max(1, lines); // at least one line
				size = ImVec2(0, Detail::multiline_text_height(lines));
				printf("size: %f\n", size.y);
			} else {
				size = ImVec2(0, Detail::multiline_text_height(line_height));
			}

			changed = ImGui::InputTextMultiline(label, &value, size, flags);
		} else {
			changed = ImGui::InputText(label, &value, flags);
		}

		if (changed) {
			string_response.changed();
		}
		ImReflect::Detail::check_input_states(string_response);
	}
}