#pragma once
#include <extern/svh/scope.hpp>
#include <extern/svh/tag_invoke.hpp>
#include <extern/visit_struct/visit_struct.hpp>

// Forward declare
using ImSettings = svh::scope;

/* Define this for you class or struct */
#define IMGUI_REFLECT(T, ...) \
VISITABLE_STRUCT_IN_CONTEXT(ImGui::Reflect::Detail::ImContext, T, __VA_ARGS__);

namespace ImGui::Reflect {
	struct ImResponse {
		bool Changed = false;
	};

	namespace Detail {

		/* Tags for the tag_invoke input functions */
		struct ImInput_t { /* Public Tag */ };
		inline constexpr ImInput_t input{};

		struct ImInputLib_t { /* Library Tag */ };
		inline constexpr ImInputLib_t input_lib{};

		/* Context for visit_struct */
		struct ImContext {};

		/* Forward declare */
		template<typename T>
		void InputImpl(const char* label, T& value, ImSettings& settings, ImResponse& response);

		template<typename T>
		void imgui_input_visit_field(const char* label, T& value, ImSettings& settings, ImResponse& response) {
			ImGui::Text("%s", label);
			visit_struct::context<ImContext>::for_each(value,
				[&](const char* name, auto& field) {
					auto& member_settings = settings.get_member(value, field);
					InputImpl(name, field, member_settings, response); // recurse
				});
		}

		template<typename T>
		void InputImpl(const char* label, T& value, ImSettings& settings, ImResponse& response) {
			auto& type_settings = settings.get<T>();
			/* Try tag_invoke for user defined implementations */
			if constexpr (svh::is_tag_invocable_v<ImInput_t, const char*, T&, ImSettings&, ImResponse&>) {
				tag_invoke(input, label, value, type_settings, response);
			}
			/* Try tag_invoke with default library implementations */
			else if constexpr (svh::is_tag_invocable_v<ImInputLib_t, const char*, T&, ImSettings&, ImResponse&>) {
				tag_invoke(input_lib, label, value, type_settings, response);
			}
			/* If type is reflected */
			else if constexpr (visit_struct::traits::is_visitable<T, ImContext>::value) {
				imgui_input_visit_field(label, value, type_settings, response);
			} else {
				static_assert(svh::always_false<T>::value, "No suitable Input implementation found for type T");
			}
		}
	}

	/* Public entry point */
	template<typename T>
	ImResponse Input(const char* label, T& value, ImSettings& settings) {
		ImResponse response;
		Detail::InputImpl(label, value, settings, response);
		return response;
	}

}

using ImResponse = ImGui::Reflect::ImResponse;