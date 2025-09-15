#pragma once
#include <imgui/imgui.h>
#include "imgui_input.hpp"

#include <string>

/* Helpers */
namespace ImGui::Reflect::Detail {

	template<typename T> /* All numbers except bool */
	constexpr bool is_numeric_v = (std::is_integral_v<T> || std::is_floating_point_v<T>) && !std::is_same_v<T, bool>;
	template<typename T>
	using enable_if_numeric_t = std::enable_if_t<is_numeric_v<T>, void>;

	/* Helper macro to return *this as derived type */
#define RETURN_THIS return static_cast<type_settings<T>&>(*this)

	template<typename T>
	struct imgui_data_type_trait {
		static_assert(is_numeric_v<T>, "T must be an arithmetic type");

		static constexpr ImGuiDataType_ value = []() {
			if constexpr (std::is_floating_point_v<T>) {
				// For floating point, match by size
				return sizeof(T) == sizeof(float) ? ImGuiDataType_Float : ImGuiDataType_Double;
			} else {
				// For integers, use size and signedness
				constexpr bool is_signed = std::is_signed_v<T>;
				constexpr size_t size = sizeof(T);

				if constexpr (size == 1) {
					return is_signed ? ImGuiDataType_S8 : ImGuiDataType_U8;
				} else if constexpr (size == 2) {
					return is_signed ? ImGuiDataType_S16 : ImGuiDataType_U16;
				} else if constexpr (size <= 4) {
					return is_signed ? ImGuiDataType_S32 : ImGuiDataType_U32;
				} else {
					return is_signed ? ImGuiDataType_S64 : ImGuiDataType_U64;
				}
			}
			}();
	};
}

/* Generic settings for types */
namespace ImGui::Reflect::Detail {

	/* Generic min/max settings for numeric types */
	template<typename T>
	struct min_max {
	private:
		T _min = default_min();
		T _max = default_max();
		bool _clamp = false;
	public:

		type_settings<T>& min(const T v) { _min = v; RETURN_THIS; }
		type_settings<T>& max(const T v) { _max = v; RETURN_THIS; }
		/* Enforce clamping after input */
		type_settings<T>& clamp(const bool v = true) { _clamp = v; RETURN_THIS; }

		const T& get_min() const { return _min; }
		const T& get_max() const { return _max; }
		bool is_clamped() const { return _clamp; }
		ImGuiDataType_ get_data_type() const { return imgui_data_type_trait<T>::value; }

	private:
		/* Half because of ImGui limitations */
		/* https://github.com/ocornut/imgui/blob/8a944222469ab23559bc01a563f4e9b516e4e701/imgui_widgets.cpp#L3232 */
		static constexpr T default_min() {
			if constexpr (std::is_signed_v<T>) {
				return std::numeric_limits<T>::lowest() / 2;
			} else {
				/* Unsigned */
				return T(0);
			}
		}

		/* Half because of ImGui limitations */
		/* https://github.com/ocornut/imgui/blob/8a944222469ab23559bc01a563f4e9b516e4e701/imgui_widgets.cpp#L3232 */
		static constexpr T default_max() {
			return std::numeric_limits<T>::max() / 2;
		}
	};

	/* Generic step settings for ``ImGui::InputScalar`` */
	template<typename T>
	struct input_step {
	private:
		T _step = T(1);
		T _step_fast = T(10);
	public:

		type_settings<T>& step(const T v) { _step = v; RETURN_THIS; }
		type_settings<T>& step_fast(const T v) { _step_fast = v; RETURN_THIS; }

		const T& get_step() const { return _step; }
		const T& get_step_fast() const { return _step_fast; }
	};

	/* format settings for input widgets */
	template<typename T>
	struct format_settings {
	private:
		constexpr std::string get_default_format() const {
			if constexpr (std::is_integral_v<T>) return "%d";
			else if constexpr (std::is_floating_point_v<T>) return "%.3f";
			else return "%s";
		}

		std::string _prefix = "";
		std::string _format = get_default_format();
		std::string _suffix = "";

	public:
		type_settings<T>& prefix(const std::string& val) { _prefix = val; RETURN_THIS; }
		type_settings<T>& format(const std::string& val) { _format = val; RETURN_THIS; }
		type_settings<T>& suffix(const std::string& val) { _suffix = val; RETURN_THIS; }

		type_settings<T>& as_int(const int digits = 0) { _format = "%0" + std::to_string(digits) + "d"; RETURN_THIS; }
		type_settings<T>& as_float(const int precision = 2) { _format = "%." + std::to_string(precision) + "f"; RETURN_THIS; }
		type_settings<T>& as_double(const int precision = 2) { _format = "%." + std::to_string(precision) + "lf"; RETURN_THIS; }
		type_settings<T>& as_sci(const int precision = 2) { _format = "%." + std::to_string(precision) + "e"; RETURN_THIS; }

		type_settings<T>& as_decimal() { _format = "%d"; RETURN_THIS; }
		type_settings<T>& as_hex() { _format = "%x"; RETURN_THIS; }
		type_settings<T>& as_hex_upper() { _format = "%X"; RETURN_THIS; }
		type_settings<T>& as_octal() { _format = "%o"; RETURN_THIS; }
		type_settings<T>& as_binary() { _format = "%b"; RETURN_THIS; } // Note: %b is not standard in printf, may need custom handling

		type_settings<T>& clear_format() { _format = ""; RETURN_THIS; }
		type_settings<T>& default_format() { _format = get_default_format(); RETURN_THIS; }

		std::string get_format() const {
			if (_format.empty()) {
				return _prefix + get_default_format() + _suffix;
			}
			return _prefix + _format + _suffix;
		}
	};

	/* Settings to specify input type: Input, Drag, Slider */
	template<typename T>
	struct input_type : input_step<T>, format_settings<T> {
	public:
		enum class type {
			Input,
			Drag,
			Slider
		};

	private:
		type _type = type::Input;

	public:
		type_settings<T>& as_input() { _type = type::Input; RETURN_THIS; }
		type_settings<T>& as_drag() { _type = type::Drag; RETURN_THIS; }
		type_settings<T>& as_slider() { _type = type::Slider; RETURN_THIS; }

		const type& get_input_type() const { return _type; }
		bool is_input() const { return _type == type::Input; }
		bool is_drag() const { return _type == type::Drag; }
		bool is_slider() const { return _type == type::Slider; }
	};
}

/* Usefull helper functions */
namespace ImGui::Reflect::Detail {

	template<typename T>
	static void input_widget(const char* label, T& value, ImSettingsT<T>& settings) {

	}

	/* Check and set input states in response */
	template<typename T>
	static void check_input_states(type_response<T>& response) {
		if (ImGui::IsItemHovered()) response.hovered();
		if (ImGui::IsItemActive()) response.active();
		if (ImGui::IsItemActivated()) response.activated();
		if (ImGui::IsItemDeactivated()) response.deactivated();
		if (ImGui::IsItemDeactivatedAfterEdit()) response.deactivated_after_edit();
		for (int i = 0; i < Internal::mouse_button_count; ++i) {
			if (ImGui::IsItemClicked(i)) response.clicked(static_cast<ImGuiMouseButton>(i));
			if (ImGui::IsMouseDoubleClicked(i)) response.double_clicked(static_cast<ImGuiMouseButton>(i));
		}
		if (ImGui::IsItemFocused()) response.focused();
	}
}

/* Input fields for primitive types */
namespace ImGui::Reflect {

	/* ========================= integral types except bool ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_numeric_t<T>> : ImSettings,
		ImGui::Reflect::Detail::min_max<T>,
		ImGui::Reflect::Detail::input_type<T> {
		/* Need to specify ``ImGui::Reflect`` before Detail::min_max otherwise intellisense wont work */
	};

	template<typename T>
	std::enable_if_t<ImGui::Reflect::Detail::is_numeric_v<T>, void>
		tag_invoke(Detail::ImInputLib_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
		type_settings<T>& num_settings = settings.get<T>();
		type_response<T>& num_response = response.get<T>();

		const auto& min = num_settings.get_min();
		const auto& max = num_settings.get_max();
		const auto& fmt = num_settings.get_format();
		constexpr auto data_type = Detail::imgui_data_type_trait<T>::value;

		bool changed = false;
		if (num_settings.is_slider()) {
			changed = ImGui::SliderScalar(label, data_type, &value, &min, &max, fmt.c_str());
		} else if (num_settings.is_drag()) {
			changed = ImGui::DragScalar(label, data_type, &value, T(0.1), &min, &max, fmt.c_str());
		} else if (num_settings.is_input()) {
			const auto& step = num_settings.get_step();
			const auto& step_fast = num_settings.get_step_fast();
			changed = ImGui::InputScalar(label, data_type, &value, &step, &step_fast, fmt.c_str());
		} else {
			throw std::runtime_error("Unknown input type");
		}

		/* Clamp if needed */
		if (num_settings.is_clamped()) {
			if (value < min) value = min;
			if (value > max) value = max;
		}

		if (changed) num_response.changed();
		ImGui::Reflect::Detail::check_input_states(num_response);
	}
}

static_assert(svh::is_tag_invocable_v<ImGui::Reflect::Detail::ImInputLib_t, const char*, int&, ImSettings&, ImResponse&>, "int tag_invoke not found");


