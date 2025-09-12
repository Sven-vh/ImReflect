#pragma once
#include <imgui/imgui.h>
#include "imgui_input.hpp"

/*
// A primary data type
enum ImGuiDataType_
{
	ImGuiDataType_S8,       // signed char / char (with sensible compilers)
	ImGuiDataType_U8,       // unsigned char
	ImGuiDataType_S16,      // short
	ImGuiDataType_U16,      // unsigned short
	ImGuiDataType_S32,      // int
	ImGuiDataType_U32,      // unsigned int
	ImGuiDataType_S64,      // long long / __int64
	ImGuiDataType_U64,      // unsigned long long / unsigned __int64
	ImGuiDataType_Float,    // float
	ImGuiDataType_Double,   // double
	ImGuiDataType_Bool,     // bool (provided for user convenience, not supported by scalar widgets)
	ImGuiDataType_String,   // char* (provided for user convenience, not supported by scalar widgets)
	ImGuiDataType_COUNT
};
*/

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
		static_assert(is_numeric_v<T>, "T must be an arithmetic type and not be a bool");
		T _min = default_min();
		T _max = default_max();

		type_settings<T>& min(const T v) { _min = v; RETURN_THIS; }
		type_settings<T>& max(const T v) { _max = v; RETURN_THIS; }

		T get_min() const { return _min; }
		T get_max() const { return _max; }
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
}

/* Input fields for primitive types */
namespace ImGui::Reflect {

	/* ========================= integral types except bool ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_numeric_t<T>> : svh::scope, Detail::min_max<T> {
	};

	template<typename T>
	std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, void>
		tag_invoke(Detail::ImInputLib_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
		auto& type_settings = settings.get<T>();
		const auto& min = type_settings.get_min();
		const auto& max = type_settings.get_max();

		//int temp = static_cast<int>(value);
		bool changed = ImGui::SliderScalar(label, type_settings.get_data_type(), &value, &min, &max);
		if (changed) {
			//value = static_cast<T>(temp);
			response.Changed = true;
		}
	}
}


static_assert(svh::is_tag_invocable_v<ImGui::Reflect::Detail::ImInputLib_t, const char*, int&, ImSettings&, ImResponse&>, "int tag_invoke not found");