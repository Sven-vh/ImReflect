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

	/* ========================= std::tuple render methods, also used by std::pair ========================= */
	namespace Detail {
		constexpr const char* TUPLE_TREE_LABEL = "##tuple_tree";
		constexpr ImVec2 TUPLE_CELL_PADDING{ 5.0f, 0.0f };
		constexpr ImVec2 TUPLE_ITEM_SPACING{ 4.0f, 0.0f };
	}

	template<std::size_t Index = 0, typename... Ts>
	void render_tuple_element_as_tree(std::tuple<Ts...>& value, ImSettings& settings, ImResponse& response) {
		if constexpr (Index < sizeof...(Ts)) {
			const std::string node_id = std::string(Detail::TUPLE_TREE_LABEL) + std::to_string(Index);

			ImGui::TableNextColumn();

			ImGuiID id = ImGui::GetID(node_id.c_str());
			ImGuiStorage* storage = ImGui::GetStateStorage();
			bool is_open = storage->GetBool(id, true);

			auto tree_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding;
			if (is_open == false) tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;


			is_open = ImGui::TreeNodeEx(node_id.c_str(), tree_flags);
			if (is_open) {
				ImGui::SameLine();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImReflect::Input("##tuple_item", std::get<Index>(value), settings, response);
				ImGui::TreePop();
			}

			// Tail recursion
			render_tuple_element_as_tree<Index + 1>(value, settings, response);
		}
	}

	template<typename Tuple, std::size_t... Is>
	void render_tuple_elements_as_table(Tuple& value, ImSettings& settings, ImResponse& response, std::index_sequence<Is...>) {
		auto render_element = [&](auto index, auto& element) {
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::PushID(static_cast<int>(index));
			ImReflect::Input("##tuple_item", element, settings, response);
			ImGui::PopID();
			};

		// Much cleaner fold expression
		(render_element(Is, std::get<Is>(value)), ...);
	}

	template<typename T, typename... Ts>
	void draw_tuple_element_label(const char* label, std::tuple<Ts...>& value, ImSettings& settings, ImResponse& response) {
		auto& tuple_settings = settings.get<T>();
		auto& tuple_response = response.get<T>();

		Detail::text_label(label);
		ImGui::SameLine();

		const auto id = Detail::scope_id("tuple");

		// Table rendering
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, Detail::TUPLE_CELL_PADDING);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Detail::TUPLE_ITEM_SPACING);

		constexpr auto tuple_size = sizeof...(Ts);
		if (ImGui::BeginTable("table", tuple_size, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings)) {
			if (tuple_settings.is_tree_node()) {
				render_tuple_element_as_tree(value, tuple_settings, tuple_response);
			} else {
				render_tuple_elements_as_table(value, tuple_settings, tuple_response, std::make_index_sequence<tuple_size>{});
			}
			ImGui::EndTable();
		}

		ImGui::PopStyleVar(2);
	}

	/* ========================= std::tuple ========================= */
	struct std_tuple {};

	template<>
	struct type_settings<std_tuple> : ImSettings,
		ImReflect::Detail::required<std_tuple>,
		ImReflect::Detail::tree_node<std_tuple> {
	};

	template<typename... Ts>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::tuple<Ts...>& value, ImSettings& settings, ImResponse& response) {
		draw_tuple_element_label<std_tuple, Ts...>(label, value, settings, response);
	}

	/* ========================= std::pair ========================= */
	/* Custom type for all pairs, this is because calling .push<std::pair> is not possible*/
	struct std_pair {};

	template<>
	struct type_settings<std_pair> : ImSettings,
		ImReflect::Detail::required<std_pair>,
		ImReflect::Detail::tree_node<std_pair> {
	};

	template<typename T1, typename T2>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::pair<T1, T2>& value, ImSettings& settings, ImResponse& response) {
		auto tuple_value = std::tie(value.first, value.second);
		draw_tuple_element_label<std_pair>(label, tuple_value, settings, response);
	}

}