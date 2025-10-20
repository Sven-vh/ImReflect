#pragma once
#include <imgui.h>
#include <extern/svh/scope.hpp>
#include <extern/svh/tag_invoke.hpp>
#include <extern/visit_struct/visit_struct.hpp>

/* Define this for you class or struct */
#define IMGUI_REFLECT(T, ...) \
VISITABLE_STRUCT_IN_CONTEXT(ImReflect::Detail::ImContext, T, __VA_ARGS__);

struct global_tag {};

namespace ImReflect {

	/* Forward declare */
	namespace Detail {
		template<typename T>
		struct required;

		template<typename T>
		struct required_response;
	}

	/* SETTINGS */
	template<class T, class ENABLE = void>
	struct type_settings; /* Forward declare */

	//TODO: consider making ImResponse a class and also inherit from required.
	// This will make every ImSettings paramter typed.
	using ImSettings = svh::scope<type_settings>;

	template<class T, class ENABLE>
	struct type_settings : ImSettings, ImReflect::Detail::required<T> {};

	/* RESPONSES */
	template<class T>
	struct type_response; /* Forward declare */

	using ImResponse = svh::scope<type_response>;

	template<class T>
	struct type_response : /* ImResponse is inherited in required */ ImReflect::Detail::required_response<T> {};

	/* Tags for the tag_invoke input functions */
	struct ImInput_t : global_tag { /* Public Tag */ };
	inline constexpr ImInput_t input{};

	namespace Detail {

		struct ImInputLib_t { /* Library only tag */ };
		inline constexpr ImInputLib_t input_lib{};

		/* Context for visit_struct */
		struct ImContext {};

		/* Forward declare */
		template<typename T>
		void InputImpl(const char* label, T& value, ImSettings& settings, ImResponse& response);

		template<typename T>
		void imgui_input_visit_field(const char* label, T& value, ImSettings& settings, ImResponse& response) {
			constexpr bool is_const = std::is_const_v<T>;
			ImGui::PushID(label);
			ImGui::SeparatorText(label);
			if constexpr (is_const) {
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
					ImGui::SetTooltip("Const object");
				}
			}
			ImGui::Indent();
			visit_struct::context<ImContext>::for_each(value,
				[&](const char* name, auto& field) {
					ImGui::PushID(name);
					auto& member_settings = settings.get_member(value, field);
					auto& member_response = response.get_member(value, field);
					InputImpl(name, field, member_settings, member_response); // recurse
					ImGui::PopID();
				});
			ImGui::Unindent();
			ImGui::PopID();
		}

		template<typename T>
		void InputImpl(const char* label, T& value, ImSettings& settings, ImResponse& response) {
			auto& type_settings = settings.get<T>();
			auto& type_response = response.get<T>();

			/* Validate type settings */
			//TODO: add link to documentation
			static_assert(std::is_base_of_v<ImReflect::Detail::required<svh::simplify_t<T>>, std::remove_reference_t<decltype(type_settings)>>,
				"ImReflect Error: TypeSettings specialization class must inherit from ImReflect::Detail::required<T>.");

			const bool disabled = type_settings.is_disabled();
			if (disabled) {
				ImGui::BeginDisabled();
			}

			const float min_width = type_settings.get_min_width();
			if (min_width > 0.0f) {
				ImGui::PushItemWidth(min_width);
			}

			/* Try tag_invoke for user defined implementations */
			if constexpr (svh::is_tag_invocable_v<ImInput_t, const char*, T&, ImSettings&, ImResponse&>) {
				tag_invoke(input, label, value, type_settings, type_response);
			}
			/* Try tag_invoke with default library implementations */
			else if constexpr (svh::is_tag_invocable_v<ImInputLib_t, const char*, T&, ImSettings&, ImResponse&>) {
				tag_invoke(input_lib, label, value, type_settings, type_response);
			}
			/* If type is reflected */
			else if constexpr (visit_struct::traits::is_visitable<std::remove_cv_t<T>, ImContext>::value) {
				imgui_input_visit_field(label, value, type_settings, type_response);
			} else {
				//TODO: add link to documentation
				static_assert(svh::always_false<T>::value, "ImReflect Error: No suitable Input implementation found for type T");
			}

			if (disabled) {
				ImGui::EndDisabled();
			}
		}
	}

	/* Public entry points */
	template<typename T>
	ImResponse Input(const char* label, T& value) {
		ImSettings settings;
		ImResponse response;
		Detail::InputImpl(label, value, settings, response);
		return response;
	}

	/* With settings */
	template<typename T>
	ImResponse Input(const char* label, T& value, ImSettings& settings) {
		ImResponse response;
		Detail::InputImpl(label, value, settings, response);
		return response;
	}

	/* Genreally not needed by users, mainly gets called by other input implementations */
	template<typename T>
	void Input(const char* label, T& value, ImSettings& settings, ImResponse& response) {
		Detail::InputImpl(label, value, settings, response);
	}


	/* Pointer inputs */
	template<typename T>
	ImResponse Input(const char* label, T* value) {
		if (value) {
			return Input(label, *value);
		} else {
			ImSettings settings;
			ImResponse response;
			ImGui::TextDisabled("%s: ", label);
			ImGui::SameLine();
			ImGui::TextDisabled("nullptr");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
				ImGui::SetTooltip("Value is nullptr");
			}
			return response;
		}
	}

	template<typename T>
	ImResponse Input(const char* label, T* value, ImSettings& settings) {
		if (value) {
			return Input(label, *value, settings);
		} else {
			ImResponse response;
			ImGui::TextDisabled("%s: ", label);
			ImGui::SameLine();
			ImGui::TextDisabled("nullptr");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
				ImGui::SetTooltip("Value is nullptr");
			}
			return response;
		}
	}

	template<typename T>
	ImResponse Input(const char* label, T* value, ImSettings& settings, ImResponse& response) {
		if (value) {
			Detail::InputImpl(label, *value, settings, response);
		} else {
			ImGui::TextDisabled("%s: ", label);
			ImGui::SameLine();
			ImGui::TextDisabled("nullptr");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
				ImGui::SetTooltip("Value is nullptr");
			}
		}
		return response;
	}
}

using ImSettings = ImReflect::ImSettings;

using ImResponse = ImReflect::ImResponse;