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
	/* Settings for types */
	template<class T, class = void>
	struct type_settings : svh::scope<type_settings> {};

	using ImSettings = svh::scope<type_settings>;

	/* responses for types */
	struct response_base;

	template<class T>
	struct type_response;

	using ImResponse = svh::scope<type_response>;

	/* Tags for the tag_invoke input functions */
	struct ImInput_t : global_tag{ /* Public Tag */ };
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
			ImGui::PushID(label);
			ImGui::SeparatorText(label);
			visit_struct::context<ImContext>::for_each(value,
				[&](const char* name, auto& field) {
					ImGui::PushID(name);
					auto& member_settings = settings.get_member(value, field);
					auto& member_response = response.get_member(value, field);
					InputImpl(name, field, member_settings, member_response); // recurse
					ImGui::PopID();
				});
			ImGui::PopID();
		}

		template<typename T>
		void InputImpl(const char* label, T& value, ImSettings& settings, ImResponse& response) {
			auto& type_settings = settings.get<T>();
			auto& type_response = response.get<T>();
			/* Try tag_invoke for user defined implementations */
			if constexpr (svh::is_tag_invocable_v<ImInput_t, const char*, T&, ImSettings&, ImResponse&>) {
				tag_invoke(input, label, value, type_settings, type_response);
			}
			/* Try tag_invoke with default library implementations */
			else if constexpr (svh::is_tag_invocable_v<ImInputLib_t, const char*, T&, ImSettings&, ImResponse&>) {
				tag_invoke(input_lib, label, value, type_settings, type_response);
			}
			/* If type is reflected */
			else if constexpr (visit_struct::traits::is_visitable<T, ImContext>::value) {
				imgui_input_visit_field(label, value, type_settings, type_response);
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

	template<typename T>
	ImResponse Input(const char* label, T& value) {
		ImSettings settings;
		ImResponse response;
		Detail::InputImpl(label, value, settings, response);
		return response;
	}
}

namespace ImReflect {
	namespace Internal {
		constexpr int mouse_button_count = 3; // (0=left, 1=right, 2=middle)
	}
	struct response_base {
		virtual ~response_base() = default;
		virtual void changed() = 0;
		virtual void hovered() = 0;
		virtual void active() = 0;
		virtual void activated() = 0;
		virtual void deactivated() = 0;
		virtual void deactivated_after_edit() = 0;
		virtual void clicked(ImGuiMouseButton button) = 0;
		virtual void double_clicked(ImGuiMouseButton button) = 0;
		virtual void focused() = 0;
	};

	template<class T>
	struct type_response : response_base, svh::scope<type_response> {
	private:
		bool _is_changed = false;
		bool _is_hovered = false;
		bool _is_active = false;
		bool _is_activated = false;
		bool _is_deactivated = false;
		bool _is_deactivated_after_edit = false;
		bool _is_clicked[Internal::mouse_button_count] = { false };
		bool _is_double_clicked[Internal::mouse_button_count] = { false };
		bool _is_focused = false;

		/* Helper to chain calls to parent */
		template<typename Method, typename... Args>
		void chain_to_parent(Method method, Args... args) {  // Note: no perfect forwarding
			if (this->has_parent()) {
				auto* p = dynamic_cast<response_base*>(this->parent);
				if (p) {
					std::invoke(method, p, args...);
				}
			}
		}

	public:
		/* Setters */
		void changed() override {
			_is_changed = true;
			chain_to_parent(&response_base::changed);
		}
		void hovered() override {
			_is_hovered = true;
			chain_to_parent(&response_base::hovered);
		}
		void active() override {
			_is_active = true;
			chain_to_parent(&response_base::active);
		}
		void activated() override {
			_is_activated = true;
			chain_to_parent(&response_base::activated);
		}
		void deactivated() override {
			_is_deactivated = true;
			chain_to_parent(&response_base::deactivated);
		}
		void deactivated_after_edit() override {
			_is_deactivated_after_edit = true;
			chain_to_parent(&response_base::deactivated_after_edit);
		}
		void clicked(ImGuiMouseButton button) override {
			if (button >= 0 && button < Internal::mouse_button_count) {
				_is_clicked[button] = true;
			}
			chain_to_parent(&response_base::clicked, button);
		}
		void double_clicked(ImGuiMouseButton button) override {
			if (button >= 0 && button < Internal::mouse_button_count) {
				_is_double_clicked[button] = true;
			}
			chain_to_parent(&response_base::double_clicked, button);
		}
		void focused() override {
			_is_focused = true;
			chain_to_parent(&response_base::focused);
		}

		/* Getters */
		bool is_changed() const { return _is_changed; }
		bool is_hovered() const { return _is_hovered; }
		bool is_active() const { return _is_active; }
		bool is_activated() const { return _is_activated; }
		bool is_deactivated() const { return _is_deactivated; }
		bool is_deactivated_after_edit() const { return _is_deactivated_after_edit; }
		bool is_clicked(ImGuiMouseButton button) const {
			if (button >= 0 && button < Internal::mouse_button_count) {
				return _is_clicked[button];
			}
			return false;
		}
		bool is_double_clicked(ImGuiMouseButton button) const {
			if (button >= 0 && button < Internal::mouse_button_count) {
				return _is_double_clicked[button];
			}
			return false;
		}
		bool is_focused() const { return _is_focused; }
	};
}

using ImSettings = ImReflect::ImSettings;

using ImResponse = ImReflect::ImResponse;