#pragma once
#include <imgui.h>
#include <imgui_stdlib.h>
#include "ImReflect_helper.hpp"
#include <extern/magic_enum/magic_enum.hpp>

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
		type_settings<T>& as_multiline(const bool v = true) { _multi_line = v; RETURN_THIS; }
		type_settings<T>& auto_resize(const bool v = true) { _multi_line = v; _line_count = v ? 0 : -1; RETURN_THIS; }
		/* -1 = imgui default, 0 = auto resize, >0 line height*/
		type_settings<T>& line_count(int count) { _multi_line = true; _line_count = count; RETURN_THIS; }

		type_settings<T>& as_singleline() { _multi_line = false; _line_count = -1; RETURN_THIS; }

		bool is_multiline() const { return _multi_line; }
		int get_line_count() const { return _line_count; }
	};

	/* Whether or not tree node is wanted */
	template<typename T>
	struct tree_node {
	private:
		bool _tree_node = false;
	public:
		type_settings<T>& as_tree_node(const bool v = true) { _tree_node = v; RETURN_THIS; }
		const bool& is_tree_node() const { return _tree_node; };
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
				std::size_t lines = std::count(value.begin(), value.end(), '\n') + 1;
				lines = std::max<std::size_t>(lines, 1); // at least one line
				size = ImVec2(0, Detail::multiline_text_height(lines));
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

	/* ========================= std::pair ========================= */
	template<typename T1, typename T2>
	struct type_settings<std::pair<T1, T2>> : ImSettings,
		ImReflect::Detail::required<std::pair<T1, T2>>,
		ImReflect::Detail::tree_node<std::pair<T1, T2>> {
	private:
		int _pair_count = 0; /* 0 = not set, >0 = user specified*/
	public:
		type_settings<std::pair<T1, T2>>& pair_count(int count) { _pair_count = count; RETURN_THIS_T(std::pair<T1,T2>); }
		int get_pair_count() const { return _pair_count; }
	};

	template<typename T>
	void pair_item_input(const char* label, T& value, ImSettings& pair_settings, ImResponse& pair_response) {
		constexpr const char* tree_label = "##pair_tree";

		ImGuiID id = ImGui::GetID(tree_label);
		ImGuiStorage* storage = ImGui::GetStateStorage();
		bool is_open = storage->GetBool(id, true);

		auto tree_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding;
		if (is_open == false) tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;

		// render
		is_open = ImGui::TreeNodeEx(tree_label, tree_flags);
		if (is_open) {
			ImGui::SameLine();

			ImReflect::Input(label, value, pair_settings, pair_response);

			ImGui::TreePop();
		}
	}

	template<typename T1, typename T2>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::pair<T1, T2>& value, ImSettings& settings, ImResponse& response) {
		type_settings<std::pair<T1, T2>>& pair_settings = settings.get<std::pair<T1, T2>>();
		type_response<std::pair<T1, T2>>& pair_response = response.get<std::pair<T1, T2>>();

		const bool as_tree = pair_settings.is_tree_node();
		const int pair_count = pair_settings.get_pair_count();

		Detail::text_label(label);
		const auto id = Detail::scope_id("pair");
		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(5.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));  // Remove item spacing

		const float column_width = (pair_count > 0) ? (ImGui::CalcItemWidth() / pair_count) : -FLT_MIN;

		if (ImGui::BeginTable("table", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {

			//ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, style.CellPadding.y));
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(column_width);

			ImGui::PushID("first");
			if (as_tree) {
				pair_item_input("##pair_first", value.first, pair_settings, pair_response);
			} else {
				ImReflect::Input("##pair_first", value.first, pair_settings, pair_response);
			}
			ImGui::PopID();

			ImGui::TableNextColumn();
			ImGui::PushItemWidth(column_width);

			ImGui::PushID("second");
			if (as_tree) {
				pair_item_input("##pair_second", value.second, pair_settings, pair_response);
			} else {
				ImReflect::Input("##pair_second", value.second, pair_settings, pair_response);
			}
			ImGui::PopID();

			ImGui::EndTable();
		}

		ImGui::PopStyleVar(2);
		/* No need to check if changed, is already handled by their own input functions*/
	}

	/* ========================= std::tuple ========================= */
	template<typename... Ts>
	struct type_settings<std::tuple<Ts...>> : ImSettings,
		ImReflect::Detail::required<std::tuple<Ts...>>,
		ImReflect::Detail::tree_node<std::tuple<Ts...>> {
	};

	template<std::size_t Index = 0, typename... Ts>
	void tuple_item_input(std::tuple<Ts...>& value, ImSettings& tuple_settings, ImResponse& tuple_response) {
		if constexpr (Index < sizeof...(Ts)) {
			using ElementType = std::tuple_element_t<Index, std::tuple<Ts...>>;
			constexpr const char* tree_label = "##tuple_tree";
			ImGuiID id = ImGui::GetID((std::string(tree_label) + std::to_string(Index)).c_str());
			ImGuiStorage* storage = ImGui::GetStateStorage();
			bool is_open = storage->GetBool(id, true);
			auto tree_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding;
			if (is_open == false) tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;
			// render
			is_open = ImGui::TreeNodeEx((std::string(tree_label) + std::to_string(Index)).c_str(), tree_flags);
			if (is_open) {
				ImGui::SameLine();
				ImReflect::Input("##tuple_item", std::get<Index>(value), tuple_settings, tuple_response);
				ImGui::TreePop();
			}
			tuple_item_input<Index + 1, Ts...>(value, tuple_settings, tuple_response); // recurse
		}
	}

	template<typename... Ts>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::tuple<Ts...>& value, ImSettings& settings, ImResponse& response) {
		type_settings<std::tuple<Ts...>>& tuple_settings = settings.get<std::tuple<Ts...>>();
		type_response<std::tuple<Ts...>>& tuple_response = response.get<std::tuple<Ts...>>();

		const bool as_tree = tuple_settings.is_tree_node();

		Detail::text_label(label);
		ImGui::SameLine();
		
		const auto id = Detail::scope_id("tuple");
		
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(5.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));

		if (ImGui::BeginTable("table", sizeof...(Ts), ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {
			std::apply([&](auto&... args) {
				int index = 0;
				((ImGui::TableNextColumn(),
					ImGui::PushItemWidth(-FLT_MIN),
					ImGui::PushID(index),
					[&]() {
						if (as_tree) {
							tuple_item_input(value, tuple_settings, tuple_response);
						} else {
							ImReflect::Input("##tuple_item", args, tuple_settings, tuple_response);
						}
					}(),
						ImGui::PopID(),
						++index), ...);
				}, value);
			ImGui::EndTable();
		}
		ImGui::PopStyleVar(2);
		/* No need to check if changed, is already handled by their own input functions*/
	}

}