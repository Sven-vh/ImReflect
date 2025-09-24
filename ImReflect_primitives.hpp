#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include "ImReflect_entry.hpp"
#include "ImReflect_helper.hpp"

#include <string>

/* Helpers */
namespace ImReflect::Detail {

	template<typename T> /* All numbers except bool */
	constexpr bool is_numeric_v = (std::is_integral_v<T> || std::is_floating_point_v<T>) && !std::is_same_v<T, bool>;
	template<typename T>
	using enable_if_numeric_t = std::enable_if_t<is_numeric_v<T>, void>;

	template<typename T> /* Only bool */
	constexpr bool is_bool_v = std::is_same_v<T, bool>;
	template<typename T>
	using enable_if_bool_t = std::enable_if_t<is_bool_v<T>, void>;

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
namespace ImReflect::Detail {

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

	/* Generic speed settings for sliders and draggers */
	template<typename T>
	struct drag_speed {
	private:
		float _speed = 1.0f;
	public:
		/* Optional default speed */
		drag_speed(const float v = 1.0f) : _speed(v) {}
		type_settings<T>& speed(const float v) { _speed = v; RETURN_THIS; }
		float get_speed() const { return _speed; }
	};

	/* format settings for input widgets */
/* Comprehensive format settings for ImGui input widgets */
	template<typename T>
	struct format_settings {
	private:
		std::string get_default_format() const {
			constexpr auto data_type = imgui_data_type_trait<T>::value;
			const ImGuiDataTypeInfo* data_type_info = ImGui::DataTypeGetInfo(data_type);
			if (data_type_info && data_type_info->PrintFmt) {
				return std::string(data_type_info->PrintFmt);
			}

			// Fallbacks based on type
			if constexpr (std::is_integral_v<T>) {
				if constexpr (std::is_signed_v<T>) return "%d";
				else return "%u";
			} else if constexpr (std::is_floating_point_v<T>) {
				return "%.3f";
			}
			return "%d"; // Final fallback
		}

		std::string _prefix = "";
		std::string _format = "";
		std::string _suffix = "";

	public:
		/*
		* NOTE: Prefix does not work if using a input widget. Only works with sliders and draggers.
		* See https://github.com/ocornut/imgui/issues/6829
		*/
		type_settings<T>& prefix(const std::string& val) { _prefix = val; RETURN_THIS; }
		type_settings<T>& format(const std::string& val) { _format = val;  RETURN_THIS; }
		type_settings<T>& suffix(const std::string& val) { _suffix = val; RETURN_THIS; }

		// Integer formats
		type_settings<T>& as_decimal() { _format = "%d";  RETURN_THIS; }
		type_settings<T>& as_unsigned() { _format = "%u";  RETURN_THIS; }
		type_settings<T>& as_hex(bool uppercase = false) { _format = uppercase ? "%X" : "%x"; RETURN_THIS; }
		type_settings<T>& as_octal() { _format = "%o";  RETURN_THIS; }

		// Integer with width/padding
		type_settings<T>& as_int_padded(int width, char pad_char = '0') { _format = "%" + std::string(1, pad_char) + std::to_string(width) + "d"; RETURN_THIS; }

		// Floating point formats
		type_settings<T>& as_float(int precision = 3) { _format = "%." + std::to_string(precision) + "f"; RETURN_THIS; }
		type_settings<T>& as_double(int precision = 6) { _format = "%." + std::to_string(precision) + "lf"; RETURN_THIS; }
		type_settings<T>& as_scientific(int precision = 2, bool uppercase = false) { _format = "%." + std::to_string(precision) + (uppercase ? "E" : "e"); RETURN_THIS; }
		type_settings<T>& as_general(int precision = 6, bool uppercase = false) { _format = "%." + std::to_string(precision) + (uppercase ? "G" : "g"); RETURN_THIS; }

		// Width and alignment
		type_settings<T>& width(int w) {
			if (_format.empty()) _format = get_base_format();
			_format = insert_width(_format, w, false);
			RETURN_THIS;
		}

		type_settings<T>& width_left_aligned(int w) {
			if (_format.empty()) _format = get_base_format();
			_format = insert_width(_format, w, true);
			RETURN_THIS;
		}

		// Sign options
		type_settings<T>& always_show_sign() {
			if (_format.empty()) _format = get_base_format();
			_format = insert_flag(_format, '+');
			RETURN_THIS;
		}

		type_settings<T>& space_for_positive() {
			if (_format.empty()) _format = get_base_format();
			_format = insert_flag(_format, ' ');
			RETURN_THIS;
		}

		// Padding options
		type_settings<T>& zero_pad(int width) { _format = "%0" + std::to_string(width) + get_type_specifier(); RETURN_THIS; }

		// Character and percentage
		type_settings<T>& as_char() { _format = "%c";  RETURN_THIS; }
		type_settings<T>& as_percentage(int precision = 1) { _format = "%." + std::to_string(precision) + "f%%"; RETURN_THIS; }

		// Utility methods
		type_settings<T>& clear_format() { _format = ""; RETURN_THIS; }
		type_settings<T>& reset() {
			_prefix.clear();
			_format.clear();
			_suffix.clear();
			RETURN_THIS;
		}

		// Get final format string
		std::string get_format() const {
			std::string core_format = _format.empty() ? get_default_format() : _format;
			if (core_format.empty()) core_format = get_default_format();
			return _prefix + core_format + _suffix;
		}

	private:
		std::string get_base_format() const {
			if constexpr (std::is_integral_v<T>) {
				return std::is_signed_v<T> ? "%d" : "%u";
			} else if constexpr (std::is_floating_point_v<T>) {
				return "%.3f";
			}
			return "%d";
		}

		std::string get_type_specifier() const {
			if constexpr (std::is_integral_v<T>) {
				return std::is_signed_v<T> ? "d" : "u";
			} else if constexpr (std::is_floating_point_v<T>) {
				return "f";
			}
			return "d";
		}

		std::string insert_width(const std::string& fmt, int width, bool left_align) const {
			size_t percent_pos = fmt.find('%');
			if (percent_pos == std::string::npos) return fmt;

			std::string result = fmt;
			std::string width_str = (left_align ? "-" : "") + std::to_string(width);
			result.insert(percent_pos + 1, width_str);
			return result;
		}

		std::string insert_flag(const std::string& fmt, char flag) const {
			size_t percent_pos = fmt.find('%');
			if (percent_pos == std::string::npos) return fmt;

			std::string result = fmt;
			result.insert(percent_pos + 1, 1, flag);
			return result;
		}
	};

	template<typename T>
	struct true_false_text {
	private:
		std::string _true_text = "True";
		std::string _false_text = "False";

	public:
		type_settings<T>& true_text(const std::string& text) { _true_text = text; RETURN_THIS; }
		type_settings<T>& false_text(const std::string& text) { _false_text = text; RETURN_THIS; }
		const std::string& get_true_text() const { return _true_text; }
		const std::string& get_false_text() const { return _false_text; };
	};

	/* Slider flag settings */
	template<typename T>
	struct slider_flags {
	private:
		ImGuiSliderFlags _flags = ImGuiSliderFlags_None;

		inline void set_flag(const ImGuiSliderFlags flag, const bool enabled) {
			if (enabled) _flags = static_cast<ImGuiSliderFlags>(_flags | flag);
			else _flags = static_cast<ImGuiSliderFlags>(_flags & ~flag);
		}
	public:
		type_settings<T>& logarithmic(const bool v = true) { set_flag(ImGuiSliderFlags_Logarithmic, v); RETURN_THIS; }
		type_settings<T>& no_round_to_format(const bool v = true) { set_flag(ImGuiSliderFlags_NoRoundToFormat, v); RETURN_THIS; }
		type_settings<T>& no_input(const bool v = true) { set_flag(ImGuiSliderFlags_NoInput, v); RETURN_THIS; }
		type_settings<T>& wrap_around(const bool v = true) { set_flag(ImGuiSliderFlags_WrapAround, v); RETURN_THIS; }
		type_settings<T>& clamp_on_input(const bool v = true) { set_flag(ImGuiSliderFlags_ClampOnInput, v); RETURN_THIS; }
		type_settings<T>& clamp_zero_range(const bool v = true) { set_flag(ImGuiSliderFlags_ClampZeroRange, v); RETURN_THIS; }
		type_settings<T>& no_speed_tweaks(const bool v = true) { set_flag(ImGuiSliderFlags_NoSpeedTweaks, v); RETURN_THIS; }
		type_settings<T>& always_clamp(const bool v = true) { set_flag(ImGuiSliderFlags_AlwaysClamp, v); RETURN_THIS; }

		const ImGuiSliderFlags& get_slider_flags() const { return _flags; }
	};

	enum class input_type_widget {
		Input,
		Drag,
		Slider,
		Radio,
		Checkbox,
		Dropdown,
		Button
	};

	template<typename T>
	struct input_type {
		input_type(const input_type_widget type) : _type(type) {}
		input_type_widget _type;

		const input_type_widget& get_input_type() const { return _type; }
	};

	template<typename T>
	struct input_widget : virtual input_type<T>, input_step<T> {
		input_widget() : input_type<T>(input_type_widget::Input) {}
		type_settings<T>& as_input() { this->_type = input_type_widget::Input; RETURN_THIS; }
		bool is_input() const { return this->_type == input_type_widget::Input; }
	};

	template<typename T>
	struct drag_widget : virtual input_type<T> {
		drag_widget() : input_type<T>(input_type_widget::Drag) {}
		type_settings<T>& as_drag() { this->_type = input_type_widget::Drag; RETURN_THIS; }
		bool is_drag() const { return this->_type == input_type_widget::Drag; }
	};

	template<typename T>
	struct slider_widget : virtual input_type<T> {
		slider_widget() : input_type<T>(input_type_widget::Slider) {}
		type_settings<T>& as_slider() { this->_type = input_type_widget::Slider; RETURN_THIS; }
		bool is_slider() const { return this->_type == input_type_widget::Slider; }
	};

	template<typename T>
	struct radio_widget : virtual input_type<T> {
		radio_widget() : input_type<T>(input_type_widget::Radio) {}
		type_settings<T>& as_radio() { this->_type = input_type_widget::Radio; RETURN_THIS; }
		bool is_radio() const { return this->_type == input_type_widget::Radio; }
	};

	template<typename T>
	struct checkbox_widget : virtual input_type<T> {
		checkbox_widget() : input_type<T>(input_type_widget::Checkbox) {}
		type_settings<T>& as_checkbox() { this->_type = input_type_widget::Checkbox; RETURN_THIS; }
		bool is_checkbox() const { return this->_type == input_type_widget::Checkbox; }
	};

	template<typename T>
	struct dropdown_widget : virtual input_type<T> {
		dropdown_widget() : input_type<T>(input_type_widget::Dropdown) {}
		type_settings<T>& as_dropdown() { this->_type = input_type_widget::Dropdown; RETURN_THIS; }
		bool is_dropdown() const { return this->_type == input_type_widget::Dropdown; }
	};

	template<typename T>
	struct button_widget : virtual input_type<T> {
		button_widget() : input_type<T>(input_type_widget::Button) {}
		type_settings<T>& as_button() { this->_type = input_type_widget::Button; RETURN_THIS; }
		bool is_button() const { return this->_type == input_type_widget::Button; }
	};
}

/* Input fields for primitive types */
namespace ImReflect {

	/* ========================= all integral types except bool ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_numeric_t<T>> : ImSettings,
		/* Need to specify ``ImReflect`` before Detail::min_max otherwise intellisense wont work */
		ImReflect::Detail::required<T>,
		ImReflect::Detail::min_max<T>,
		ImReflect::Detail::drag_speed<T>,
		ImReflect::Detail::input_widget<T>,
		ImReflect::Detail::drag_widget<T>,
		ImReflect::Detail::slider_widget<T>,
		ImReflect::Detail::input_flags<T>,
		ImReflect::Detail::format_settings<T>,
		ImReflect::Detail::slider_flags<T> {
		/* Default to input */
		type_settings() : ImReflect::Detail::input_type<T>(ImReflect::Detail::input_type_widget::Input) {}
	};

	template<typename T>
	std::enable_if_t<ImReflect::Detail::is_numeric_v<T>, void>
		tag_invoke(Detail::ImInputLib_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
		type_settings<T>& num_settings = settings.get<T>();
		type_response<T>& num_response = response.get<T>();

		const auto& min = num_settings.get_min();
		const auto& max = num_settings.get_max();
		const auto& fmt = num_settings.get_format();
		const auto& speed = num_settings.get_speed();
		constexpr auto data_type = Detail::imgui_data_type_trait<T>::value;

		bool changed = false;
		if (num_settings.is_slider()) {
			const auto id = Detail::scope_id("slider");
			changed = ImGui::SliderScalar(label, data_type, &value, &min, &max, fmt.c_str(), num_settings.get_slider_flags());
		} else if (num_settings.is_drag()) {
			const auto id = Detail::scope_id("drag");
			changed = ImGui::DragScalar(label, data_type, &value, speed, &min, &max, fmt.c_str(), num_settings.get_slider_flags());
		} else if (num_settings.is_input()) {
			const auto id = Detail::scope_id("input");
			const auto& step = num_settings.get_step();
			const auto& step_fast = num_settings.get_step_fast();
			changed = ImGui::InputScalar(label, data_type, &value, &step, &step_fast, fmt.c_str(), num_settings.get_input_flags());
		} else {
			throw std::runtime_error("Unknown input type");
		}

		/* Clamp if needed */
		if (num_settings.is_clamped()) {
			if (value < min) value = min;
			if (value > max) value = max;
		}

		if (changed) {
			num_response.changed();
		}
		ImReflect::Detail::check_input_states(num_response);
	}

	/* ========================= bool ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_bool_t<T>> : ImSettings,
		ImReflect::Detail::required<T>,
		ImReflect::Detail::checkbox_widget<T>,
		ImReflect::Detail::radio_widget<T>,
		ImReflect::Detail::button_widget<T>,
		ImReflect::Detail::dropdown_widget<T>,
		ImReflect::Detail::true_false_text<T> {
		/* Default to checkbox */
		type_settings() : ImReflect::Detail::input_type<T>(ImReflect::Detail::input_type_widget::Checkbox) {}
	};

	template<typename T>
	std::enable_if_t<ImReflect::Detail::is_bool_v<T>, void>
		tag_invoke(Detail::ImInputLib_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
		type_settings<T>& bool_settings = settings.get<T>();
		type_response<T>& bool_response = response.get<T>();
		bool changed = false;
		if (bool_settings.is_checkbox()) {
			const auto id = Detail::scope_id("checkbox");

			changed = ImGui::Checkbox(label, &value);
		} else if (bool_settings.is_radio()) {
			const auto id = Detail::scope_id("radio");

			int int_value = value ? 1 : 0;
			changed = ImGui::RadioButton(bool_settings.get_true_text().c_str(), &int_value, 1);
			ImGui::SameLine();
			changed |= ImGui::RadioButton(bool_settings.get_false_text().c_str(), &int_value, 0);
			value = (int_value != 0);

			ImGui::SameLine();
			ImReflect::Detail::text_label(label);
		} else if (bool_settings.is_dropdown()) {
			const auto id = Detail::scope_id("dropdown");

			const char* items[] = { bool_settings.get_false_text().c_str(), bool_settings.get_true_text().c_str() };
			int int_value = value ? 1 : 0;
			changed = ImGui::Combo(label, &int_value, items, IM_ARRAYSIZE(items));

			value = (int_value != 0);
		} else if (bool_settings.is_button()) {
			const auto id = Detail::scope_id("button");

			const auto& button_label = value ? bool_settings.get_true_text() : bool_settings.get_false_text();
			if (ImGui::Button(button_label.c_str())) {
				value = !value;
				changed = true;
			}

			ImGui::SameLine();
			ImReflect::Detail::text_label(label);
		} else {
			throw std::runtime_error("Unknown input type for bool");
		}
		if (changed) {
			bool_response.changed();
		}
		ImReflect::Detail::check_input_states(bool_response);

	}

	/* TODO: delete this, only for testing*/
	enum class TestEnum {

	};
	static_assert(std::is_enum_v<TestEnum>, "TestEnum is not an enum");

	/* ========================= enums ========================= */
	template<typename E>
	struct type_settings<E, std::enable_if_t<std::is_enum_v<E>, void>> : ImSettings,
		ImReflect::Detail::required<E>,
		ImReflect::Detail::radio_widget<E>,
		ImReflect::Detail::dropdown_widget<E>,
		ImReflect::Detail::drag_widget<E>,
		ImReflect::Detail::drag_speed<E>,
		ImReflect::Detail::slider_widget<E> {
		type_settings() :
			/* Default settings */
			ImReflect::Detail::input_type<E>(ImReflect::Detail::input_type_widget::Dropdown),
			ImReflect::Detail::drag_speed<E>(0.01f) {
		}
	};

	/* Use magic enum to reflect */
	template<typename E>
	std::enable_if_t<std::is_enum_v<E>, void>
		tag_invoke(Detail::ImInputLib_t, const char* label, E& value, ImSettings& settings, ImResponse& response) {
		type_settings<E>& enum_settings = settings.get<E>();
		type_response<E>& enum_response = response.get<E>();

		const auto enum_values = magic_enum::enum_values<E>();
		const auto enum_names = magic_enum::enum_names<E>();
		const int enum_count = static_cast<int>(enum_values.size());

		bool changed = false;
		if (enum_settings.is_radio()) {
			const auto id = Detail::scope_id("radio_enum");

			int int_value = static_cast<int>(value);

			const float child_width = ImGui::CalcItemWidth();
			const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
			const ImGuiStyle& style = ImGui::GetStyle();
			const float child_height = label_size.y + style.ScrollbarSize + (style.WindowPadding.y) * 2.0f;
			const ImVec2 child_size(child_width, child_height);

			/* Evenly spread radio buttons */
			if (ImGui::BeginChild("##radio_enum", child_size, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
				for (int i = 0; i < enum_count; ++i) {
					changed |= ImGui::RadioButton(enum_names[i].data(), &int_value, static_cast<int>(enum_values[i]));
					if (i < enum_count - 1) ImGui::SameLine();
				}
				value = static_cast<E>(int_value);
			}

			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::Text("%s", label);
		} else if (enum_settings.is_dropdown()) {
			const auto id = Detail::scope_id("dropdown_enum");

			int int_value = static_cast<int>(value);

			std::vector<const char*> item_vec;
			for (const auto& name : enum_names) {
				item_vec.push_back(name.data());
			}

			changed = ImGui::Combo(label, &int_value, item_vec.data(), enum_count);
			value = static_cast<E>(int_value);
		} else if (enum_settings.is_slider()) {
			const auto id = Detail::scope_id("slider_enum");

			int int_value = static_cast<int>(value);
			const int min = static_cast<int>(enum_values.front());
			const int max = static_cast<int>(enum_values.back());

			const char* index_name = magic_enum::enum_name(value).data();

			changed = ImGui::SliderInt(label, &int_value, min, max, index_name);
			value = static_cast<E>(int_value);
		} else if (enum_settings.is_drag()) {
			const auto id = Detail::scope_id("drag_enum");

			int int_value = static_cast<int>(value);
			const int min = static_cast<int>(enum_values.front());
			const int max = static_cast<int>(enum_values.back());

			const auto& speed = enum_settings.get_speed();
			const char* index_name = magic_enum::enum_name(value).data();

			changed = ImGui::DragInt(label, &int_value, speed, min, max, index_name);
			value = static_cast<E>(int_value);
		} else {
			throw std::runtime_error("Unknown input type for enum");
		}

		if (changed) {
			enum_response.changed();
		}
		ImReflect::Detail::check_input_states(enum_response);
	}
}

static_assert(svh::is_tag_invocable_v<ImReflect::Detail::ImInputLib_t, const char*, int&, ImSettings&, ImResponse&>, "int tag_invoke not found");


