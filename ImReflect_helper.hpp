#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include "ImReflect_entry.hpp"

/* Usefull helper functions */
namespace ImReflect::Detail {
	/* Helper macro to return *this as derived type */
#define RETURN_THIS return static_cast<type_settings<T>&>(*this)
#define RETURN_THIS_T(...) return static_cast<type_settings<__VA_ARGS__>&>(*this)

	/* Simple RAII ID */
	struct scope_id {
		scope_id(const char* id) { ImGui::PushID(id); }
		~scope_id() { ImGui::PopID(); }
	};

	void text_label(const std::string& text) {
		size_t pos = text.find("##");
		if (pos != std::string::npos) {
			ImGui::TextUnformatted(text.c_str(), text.c_str() + pos);
		} else {
			ImGui::TextUnformatted(text.c_str());
		}
	}

	float multiline_text_height(std::size_t line_height) {
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
		const bool& has_min_width() const { return _min_width > 0.0f; }
	};

	/* Required marker */
	template<typename T>
	struct required : disabled<T>, min_width_mixin<T> {

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
