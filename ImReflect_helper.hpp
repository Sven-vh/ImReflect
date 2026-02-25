#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include "ImReflect_entry.hpp"

/* Usefull helper functions */
namespace ImReflect::Detail {
	namespace Internal {
		constexpr int mouse_button_count = 3; // (0=left, 1=right, 2=middle)
	}

	/* Helper macro to return *this as derived type */
#define RETURN_THIS return static_cast<type_settings<T>&>(*this)
#define RETURN_THIS_T(...) return static_cast<type_settings<__VA_ARGS__>&>(*this)

	/* Simple RAII */
	struct scope_id {
		scope_id(const char* id) { ImGui::PushID(id); }
		scope_id(const int id) { ImGui::PushID(id); }
		~scope_id() { ImGui::PopID(); }
	};

	struct scope_indent {
		const float width = 0.0f;
		scope_indent(float _width = 0.0f) : width(_width) {
			ImGui::Indent(width);
		}
		~scope_indent() { ImGui::Unindent(width); }
	};

	struct scope_disabled {
        const bool disabled = false;
		scope_disabled(bool _disabled = true) : disabled(_disabled) {
			if (disabled) ImGui::BeginDisabled();
        }
		~scope_disabled() {
            if (disabled) ImGui::EndDisabled();
		}
    };

	struct scope_style {
        const ImGuiStyleVar_ var;
        const float value;
        scope_style(ImGuiStyleVar_ _var, float _value) : var(_var), value(_value) { ImGui::PushStyleVar(var, value); }
        ~scope_style() { ImGui::PopStyleVar(); }
	};

	inline void text_label(const std::string& text) {
		size_t pos = text.find("##");
		if (pos != std::string::npos) {
			ImGui::TextUnformatted(text.c_str(), text.c_str() + pos);
		} else {
			ImGui::TextUnformatted(text.c_str());
		}
	}

	inline float multiline_text_height(std::size_t line_height) {
		const auto& ctx = *ImGui::GetCurrentContext();
		return ctx.FontSize * line_height + ctx.Style.FramePadding.y * 2.0f;
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

	inline void imgui_tooltip(const char* tooltip) {
		if (tooltip && tooltip[0] != '\0' && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("%s", tooltip);
		}
	}
}

/* Shared Generic settings for types between primitives and std types */
namespace ImReflect::Detail {

	/* Setting to enable/disable certain inputs */
	template<typename T>
	struct disabled {
	private:
		bool _disabled = false;
	public:
		type_settings<T>& disable(const bool v = true) { _disabled = v; RETURN_THIS; }
		const bool& is_disabled() const { return _disabled; }
	};

	template<typename T>
	struct min_width_mixin {
	private:
		float _min_width = 0.0f; /* 0 = imgui default */
	public:
		type_settings<T>& min_width(float width) { _min_width = width; RETURN_THIS; }
		const float& get_min_width() const { return _min_width; };
		bool has_min_width() const { return _min_width > 0.0f; }
	};

	template<typename T>
	struct same_line_mixin {
	private:
		bool _same_line = false;
	public:
		type_settings<T>& same_line(bool v = true) { _same_line = v; RETURN_THIS; }
		bool on_same_line() const { return _same_line; }
	};

	template<typename T>
	struct separator_mixin {
	private:
		bool _separator = false;
	public:
		type_settings<T>& separator(bool v = true) { _separator = v; RETURN_THIS; }
		bool has_separator() const { return _separator; }
	};

	/* Only works when pusing members */
	template<typename T>
	struct label_mixin {
	private:
		std::string _label;
	public:
		/* Only works when pusing members */
		type_settings<T>& label(const std::string& label) { _label = label; RETURN_THIS; }
		const std::string& get_label() const { return _label; }
		bool has_label() const { return !_label.empty(); }
	};

	/* Required marker */
	template<typename T>
	struct required : disabled<T>, min_width_mixin<T>, same_line_mixin<T>, separator_mixin<T>, label_mixin<T> {

	};

	/* Input flag settings*/
	template<typename T>
	struct input_flags {
	private:
		ImGuiInputTextFlags _flags = ImGuiInputTextFlags_None;
		inline void set_flag(const ImGuiInputTextFlags flag, const bool enabled) {
			if (enabled) _flags = static_cast<ImGuiInputTextFlags>(_flags | flag);
			else _flags = static_cast<ImGuiInputTextFlags>(_flags & ~flag);
		}

	public:
		type_settings<T>& chars_decimal(const bool v = true) { set_flag(ImGuiInputTextFlags_CharsDecimal, v); RETURN_THIS; }
		type_settings<T>& chars_hexadecimal(const bool v = true) { set_flag(ImGuiInputTextFlags_CharsHexadecimal, v); RETURN_THIS; }
		type_settings<T>& chars_scientific(const bool v = true) { set_flag(ImGuiInputTextFlags_CharsScientific, v); RETURN_THIS; }
		type_settings<T>& chars_uppercase(const bool v = true) { set_flag(ImGuiInputTextFlags_CharsUppercase, v); RETURN_THIS; }
		type_settings<T>& chars_no_blank(const bool v = true) { set_flag(ImGuiInputTextFlags_CharsNoBlank, v); RETURN_THIS; }
		type_settings<T>& allow_tab_input(const bool v = true) { set_flag(ImGuiInputTextFlags_AllowTabInput, v); RETURN_THIS; }
		type_settings<T>& enter_returns_true(const bool v = true) { set_flag(ImGuiInputTextFlags_EnterReturnsTrue, v); RETURN_THIS; }
		type_settings<T>& escape_clears_all(const bool v = true) { set_flag(ImGuiInputTextFlags_EscapeClearsAll, v); RETURN_THIS; }
		type_settings<T>& ctrl_enter_for_new_line(const bool v = true) { set_flag(ImGuiInputTextFlags_CtrlEnterForNewLine, v); RETURN_THIS; }
		type_settings<T>& read_only(const bool v = true) { set_flag(ImGuiInputTextFlags_ReadOnly, v); RETURN_THIS; }
		type_settings<T>& password(const bool v = true) { set_flag(ImGuiInputTextFlags_Password, v); RETURN_THIS; }
		type_settings<T>& always_overwrite(const bool v = true) { set_flag(ImGuiInputTextFlags_AlwaysOverwrite, v); RETURN_THIS; }
		type_settings<T>& auto_select_all(const bool v = true) { set_flag(ImGuiInputTextFlags_AutoSelectAll, v); RETURN_THIS; }
		type_settings<T>& parse_empty_ref_val(const bool v = true) { set_flag(ImGuiInputTextFlags_ParseEmptyRefVal, v); RETURN_THIS; }
		type_settings<T>& display_empty_ref_val(const bool v = true) { set_flag(ImGuiInputTextFlags_DisplayEmptyRefVal, v); RETURN_THIS; }
		type_settings<T>& no_horizontal_scroll(const bool v = true) { set_flag(ImGuiInputTextFlags_NoHorizontalScroll, v); RETURN_THIS; }
		type_settings<T>& no_undo_redo(const bool v = true) { set_flag(ImGuiInputTextFlags_NoUndoRedo, v); RETURN_THIS; }
		type_settings<T>& elide_left(const bool v = true) { set_flag(ImGuiInputTextFlags_ElideLeft, v); RETURN_THIS; }
		type_settings<T>& callback_completion(const bool v = true) { set_flag(ImGuiInputTextFlags_CallbackCompletion, v); RETURN_THIS; }
		type_settings<T>& callback_history(const bool v = true) { set_flag(ImGuiInputTextFlags_CallbackHistory, v); RETURN_THIS; }
		type_settings<T>& callback_always(const bool v = true) { set_flag(ImGuiInputTextFlags_CallbackAlways, v); RETURN_THIS; }
		type_settings<T>& callback_char_filter(const bool v = true) { set_flag(ImGuiInputTextFlags_CallbackCharFilter, v); RETURN_THIS; }
		type_settings<T>& callback_resize(const bool v = true) { set_flag(ImGuiInputTextFlags_CallbackResize, v); RETURN_THIS; }
		type_settings<T>& callback_edit(const bool v = true) { set_flag(ImGuiInputTextFlags_CallbackEdit, v); RETURN_THIS; }

		const ImGuiInputTextFlags& get_input_flags() const { return _flags; }
	};
}

/* Shared Generic responses */
namespace ImReflect::Detail {
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

	template<typename T>
	struct required_response : ImResponse, response_base {
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