#pragma once
#include <imgui.h>
#include <imgui_stdlib.h>
#include "ImReflect_helper.hpp"
#include <extern/magic_enum/magic_enum.hpp>

/* Helpers */
namespace ImReflect::Detail {
	template<typename T> /* All numbers except bool */
	constexpr bool is_string_type_v = std::is_same_v<std::remove_cv_t<T>, std::string>;
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
	struct dropdown {
	private:
		bool _dropdown = false;
	public:
		type_settings<T>& as_dropdown(const bool v = true) { _dropdown = v; RETURN_THIS; }
		const bool& is_dropdown() const { return _dropdown; };
	};

	enum class TupleRenderMode {
		Line,
		Grid
	};

	struct render_mode_base {
		render_mode_base(const TupleRenderMode& mode) : mode(mode) {}
		TupleRenderMode mode;

		TupleRenderMode get_render_mode() const { return mode; }
	};

	template<typename T>
	struct line_mixin : public virtual render_mode_base {
		line_mixin() : render_mode_base(TupleRenderMode::Line) {}

		type_settings<T>& as_line() { mode = TupleRenderMode::Line; RETURN_THIS; }
		bool is_line() const { return mode == TupleRenderMode::Line; }
	};

	template<typename T>
	struct grid_mixin : public virtual render_mode_base {
	private:
		int _columns = 3; /* Default to 3 columns */

	public:
		grid_mixin() : render_mode_base(TupleRenderMode::Grid) {}

		type_settings<T>& as_grid() { mode = TupleRenderMode::Grid; RETURN_THIS; }
		bool is_grid() const { return mode == TupleRenderMode::Grid; }

		type_settings<T>& columns(int count) { _columns = count; RETURN_THIS; }
		int get_columns() const { return _columns; }
	};

	template<typename T>
	struct reorderable_mixin {
	private:
		bool _reorderable = true;
	public:
		type_settings<T>& reorderable(const bool v = true) { _reorderable = v; RETURN_THIS; }
		const bool& is_reorderable() const { return _reorderable; };
	};

	template<typename T>
	struct insertable_mixin {
	private:
		bool _insertable = true;
	public:
		type_settings<T>& insertable(const bool v = true) { _insertable = v; RETURN_THIS; }
		const bool& is_insertable() const { return _insertable; };
	};

	template<typename T>
	struct removable_mixin {
	private:
		bool _removable = true;
	public:
		type_settings<T>& removable(const bool v = true) { _removable = v; RETURN_THIS; }
		const bool& is_removable() const { return _removable; };
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
		auto& string_settings = settings.get<T>();
		auto& string_response = response.get<T>();

		constexpr bool is_const = std::is_const_v<T>;

		const bool multi_line = string_settings.is_multiline();
		const auto& flags = string_settings.get_input_flags();

		bool changed = false;
		if constexpr (is_const == false) {
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
		} else {
			/* Const value, just display */
			ImReflect::Detail::text_label(label);
			ImReflect::Detail::imgui_tooltip("Value is const");

			ImGui::BeginDisabled();
			ImGui::SameLine();

			ImGui::TextWrapped("%s", value.c_str());

			ImGui::EndDisabled();
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
		constexpr ImVec2 TUPLE_CELL_GRID_PADDING{ 5.0f, 2.5f };
		constexpr ImVec2 TUPLE_ITEM_SPACING{ 4.0f, 0.0f };
	}

	template<typename T, typename Tuple, std::size_t... Is>
	void render_tuple_elements_as_table(Tuple& value, ImSettings& settings, ImResponse& response, std::index_sequence<Is...>) {
		auto& tuple_settings = settings.get<T>();
		auto& tuple_response = response.get<T>();

		/* Settings to have things in a grid */
		const bool is_grid = tuple_settings.is_grid();
		const int columns = tuple_settings.get_columns();

		/* Settings to have things on a single line*/
		const bool is_line = tuple_settings.is_line();
		const bool use_min_width = tuple_settings.has_min_width();
		const float min_width = tuple_settings.get_min_width();

		float column_width = -FLT_MIN;
		if (use_min_width) {
			column_width = min_width;
		}

		if ((is_line && use_min_width) || is_grid) {
			const int loop_count = (is_line && use_min_width) ? sizeof...(Is) : columns;
			const bool first_col_fixed = (is_line && use_min_width) || (is_grid && use_min_width);

			for (int i = 0; i < loop_count; ++i) {
				if (i == 0 && first_col_fixed) {
					ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthFixed, min_width);
				} else {
					if (is_grid && use_min_width) {
						ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthFixed, min_width);
					} else {
						ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch);
					}
				}
			}
		}

		const bool is_dropdown = tuple_settings.is_dropdown();
		auto render_element = [&](auto index, auto&& element) {
			if (is_grid) {
				const int col = index % columns;
				if (col == 0 && index != 0) {
					ImGui::TableNextRow();
				}
			}
			ImGui::TableNextColumn();
			ImGui::PushID(static_cast<int>(index));
			if (is_dropdown) {
				const std::string node_id = std::string(Detail::TUPLE_TREE_LABEL) + std::to_string(index);

				const ImGuiID id = ImGui::GetID(node_id.c_str());
				ImGuiStorage* storage = ImGui::GetStateStorage();
				bool is_open = storage->GetBool(id, true);

				auto tree_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding;
				if (is_open == false) tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;

				is_open = ImGui::TreeNodeEx(node_id.c_str(), tree_flags);
				if (is_open) {
					ImGui::SameLine();
					ImGui::SetNextItemWidth(column_width);
					ImReflect::Input("##tuple_item", std::forward<decltype(element)>(element), tuple_settings, tuple_response);
					ImGui::TreePop();
				}
			} else {
				ImGui::SetNextItemWidth(column_width);
				ImReflect::Input("##tuple_item", std::forward<decltype(element)>(element), tuple_settings, tuple_response);
			}
			ImGui::PopID();
			};

		// fold expression
		(render_element(Is, std::get<Is>(value)), ...);
	}

	template<typename Tag, bool is_const, typename... Ts>
	auto& get_tuple_value(const std::tuple<Ts...>& original_value) {
		if constexpr (is_const) {
			return original_value;
		} else {
			return const_cast<std::tuple<Ts...>&>(original_value);
		}
	}

	// Const overload, exactly the same as non-const version
	template<typename Tag, bool is_const, typename... Ts>
	void draw_tuple_element_label(const char* label, const std::tuple<Ts...>& original_value, ImSettings& settings, ImResponse& response) {
		auto& tuple_settings = settings.get<Tag>();
		auto& tuple_response = response.get<Tag>();

		/* Get non-const reference if possible */
		auto& value = get_tuple_value<Tag, is_const, Ts...>(original_value);

		Detail::text_label(label);
		ImGui::SameLine();

		const auto id = Detail::scope_id("tuple");

		/* Settings to have things in a grid */
		const bool is_grid = tuple_settings.is_grid();
		const bool use_min_width = tuple_settings.has_min_width();

		ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
		if (use_min_width) flags |= ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoKeepColumnsVisible;

		constexpr auto tuple_size = sizeof...(Ts);
		const int columns = tuple_settings.get_columns();

		int tuple_columns = tuple_size;
		if (is_grid) {
			tuple_columns = std::min<int>(columns, tuple_size);
		}

		// Table rendering
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Detail::TUPLE_ITEM_SPACING);
		if (is_grid) {
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, Detail::TUPLE_CELL_GRID_PADDING);
		} else {
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, Detail::TUPLE_CELL_PADDING);
		}

		if (ImGui::BeginTable("table", tuple_columns, flags)) {
			render_tuple_elements_as_table<Tag>(value, tuple_settings, tuple_response, std::make_index_sequence<tuple_size>{});
			ImGui::EndTable();
		}

		ImGui::PopStyleVar(2);
	}

	/* ========================= std::tuple ========================= */
	struct std_tuple {};

	template<>
	struct type_settings<std_tuple> : ImSettings,
		ImReflect::Detail::required<std_tuple>,
		ImReflect::Detail::dropdown<std_tuple>,
		ImReflect::Detail::line_mixin<std_tuple>,
		ImReflect::Detail::grid_mixin<std_tuple> {
		type_settings() : ImReflect::Detail::render_mode_base(Detail::TupleRenderMode::Line) {}
	};

	template<typename... Ts>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::tuple<Ts...>& value, ImSettings& settings, ImResponse& response) {
		draw_tuple_element_label<std_tuple, false, Ts...>(label, value, settings, response);
	}

	template<typename... Ts>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::tuple<Ts...>& value, ImSettings& settings, ImResponse& response) {
		draw_tuple_element_label<std_tuple, true, Ts...>(label, value, settings, response);
	}

	/* ========================= std::pair ========================= */
	/* Custom type for all pairs, this is because calling .push<std::pair> is not possible*/
	struct std_pair {};

	template<>
	struct type_settings<std_pair> : ImSettings,
		ImReflect::Detail::required<std_pair>,
		ImReflect::Detail::dropdown<std_pair>,
		ImReflect::Detail::line_mixin<std_pair>,
		ImReflect::Detail::grid_mixin<std_pair> {
		type_settings() : ImReflect::Detail::render_mode_base(Detail::TupleRenderMode::Line) {}
	};

	template<typename T1, typename T2>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::pair<T1, T2>& value, ImSettings& settings, ImResponse& response) {
		auto tuple_value = std::tie(value.first, value.second);
		draw_tuple_element_label<std_pair, false>(label, tuple_value, settings, response);
	}

	template<typename T1, typename T2>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::pair<T1, T2>& value, ImSettings& settings, ImResponse& response) {
		auto tuple_value = std::tie(value.first, value.second);
		draw_tuple_element_label<std_pair, true>(label, tuple_value, settings, response);
	}

	/* ========================= std::vector ========================= */

	struct std_vector {};
	template<>
	struct type_settings<std_vector> : ImSettings,
		ImReflect::Detail::required<std_vector>,
		ImReflect::Detail::dropdown<std_vector>,
		ImReflect::Detail::reorderable_mixin<std_vector>,
		ImReflect::Detail::insertable_mixin<std_vector>,
		ImReflect::Detail::removable_mixin<std_vector> {
	};

	namespace Detail {
		constexpr const char* VECTOR_TREE_LABEL = "##vector_tree";
	}

	template<typename T, bool is_const>
	auto& get_vector_value(const std::vector<T>& original_value) {
		if constexpr (is_const) {
			return original_value;
		} else {
			return const_cast<std::vector<T>&>(original_value);
		}
	}

	template<typename T, bool is_const>
	void vector_input(ImReflect::ImSettings& settings, ImReflect::ImResponse& response, const char* label, const std::vector<T>& original_value) {
		auto& vec_settings = settings.get<std_vector>();
		auto& vec_response = response.get<std_vector>();

		/* Get non-const reference if possible */
		auto& value = get_vector_value<T, is_const>(original_value);

		const bool is_dropdown = vec_settings.is_dropdown();
		const bool use_min_width = vec_settings.has_min_width();
		const float min_width = vec_settings.get_min_width();

		const bool is_reorderable = vec_settings.is_reorderable();
		const bool is_insertable = vec_settings.is_insertable();
		const bool is_removable = vec_settings.is_removable();

		float column_width = 0;
		if (use_min_width) {
			column_width = min_width;
		}

		constexpr bool default_constructible = std::is_default_constructible_v<T> && !is_const;
		constexpr bool copy_constructible = std::is_copy_constructible_v<T> && !is_const;
		constexpr bool move_constructible = std::is_move_constructible_v<T> && !is_const;

		const auto id = Detail::scope_id("vector");

		ImReflect::Detail::text_label(label);
		ImGui::SameLine();
		size_t item_count = value.size();
		if (is_insertable) {
			if constexpr (default_constructible) {
				if (ImGui::Button("+")) {
					value.emplace_back(T());
					vec_response.changed();
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::Button("+");
				ImGui::EndDisabled();
				Detail::imgui_tooltip("Type is not default constructible or container is const, cannot add new item");
			}
		}
		ImGui::SameLine();
		if (is_removable) {
			if constexpr (is_const == false) {
				if (item_count > 0) {
					if (ImGui::Button("-")) {
						value.pop_back();
						vec_response.changed();
						item_count = value.size();
					}
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::Button("-");
				ImGui::EndDisabled();
				Detail::imgui_tooltip("Value is const, cannot remove item");
			}
		}

		bool is_open = true;
		if (is_dropdown) {
			is_open = ImGui::TreeNodeEx(Detail::VECTOR_TREE_LABEL, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);
		}
		if (is_open) {
			if (!is_dropdown) ImGui::Indent();

			if constexpr (move_constructible) {
				if (is_reorderable) {
					ImGui::BeginChild("##drop_zone_0", ImVec2(0, ImGui::GetStyle().ItemSpacing.y * 0.5f), false);
					ImGui::EndChild();

					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VECTOR_ITEM")) {
							int source_idx = *(const int*)payload->Data;
							int target_idx = 0; // Insert at beginning

							if (source_idx != target_idx) {
								// Moving to beginning: rotate [0, source, source+1)
								std::rotate(value.begin(),
									value.begin() + source_idx,
									value.begin() + source_idx + 1);
								vec_response.changed();
							}
						}
						ImGui::EndDragDropTarget();
					}
				}
			}

			for (int i = 0; i < item_count; ++i) {
				ImGui::PushID(i);

				bool right_clicked = false;

				if (is_reorderable) {
					//ImGui::BeginDisabled();
					//ImGui::Button("==");
					//ImGui::EndDisabled();
					ImGui::Text("==");
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						ImGui::SetDragDropPayload("VECTOR_ITEM", &i, sizeof(int));
						//ImGui::Text("Moving item %d", i);
						ImGui::PushItemWidth(column_width);
						ImReflect::Input("##vector_item", value[i], vec_settings, vec_response);
						ImGui::PopItemWidth();
						ImGui::EndDragDropSource();
					}
					if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
						right_clicked = true;
					}
					ImGui::SameLine();
				}

				ImGui::PushItemWidth(column_width);
				ImReflect::Input("##vector_item", value[i], vec_settings, vec_response);
				ImGui::PopItemWidth();

				/* drop zone between items */
				const float item_spacing_y = ImGui::GetStyle().ItemSpacing.y;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - item_spacing_y);
				ImGui::BeginChild("##spacer", ImVec2(0, item_spacing_y), false);
				ImGui::EndChild();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - item_spacing_y);

				if constexpr (move_constructible) {

					if (is_reorderable) {
						if (ImGui::BeginDragDropTarget()) {
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VECTOR_ITEM")) {
								int source_idx = *(const int*)payload->Data;
								int target_idx = i + 1; // Now this correctly inserts AFTER item i

								if (source_idx != target_idx) {
									if (source_idx < target_idx) {
										std::rotate(value.begin() + source_idx,
											value.begin() + source_idx + 1,
											value.begin() + target_idx);
									} else {
										std::rotate(value.begin() + target_idx,
											value.begin() + source_idx,
											value.begin() + source_idx + 1);
									}
									vec_response.changed();
								}
							}
							ImGui::EndDragDropTarget();
						}
					}
				}

				if (right_clicked) {
					ImGui::OpenPopup("item_context_menu");
				}

				if constexpr (is_const == false) {

					if (ImGui::BeginPopup("item_context_menu")) {

						if (is_removable && ImGui::MenuItem("Remove item")) {
							value.erase(value.begin() + i);
							vec_response.changed();
							ImGui::EndPopup();
							ImGui::PopID();
							break; /* Important, as the vector has changed */
						}
						//only allow duplicate/insert if type is default constructible
						if constexpr (copy_constructible) {
							if (is_insertable && ImGui::MenuItem("Duplicate item")) {
								value.insert(value.begin() + i + 1, value[i]);
								vec_response.changed();
							}
						} else {
							ImGui::BeginDisabled();
							if (ImGui::MenuItem("Duplicate item")) {
							}
							ImGui::EndDisabled();
							Detail::imgui_tooltip("Type is not copy constructible or container is const, cannot duplicate item");
						}
						ImGui::Separator();
						//move up/down
						if (is_reorderable) {

							if (ImGui::MenuItem("Move up") && i != 0) {
								std::swap(value[i], value[i - 1]);
								vec_response.changed();
							}
							if (ImGui::MenuItem("Move down") && i != static_cast<int>(item_count) - 1) {
								std::swap(value[i], value[i + 1]);
								vec_response.changed();
							}
							ImGui::Separator();
							if (ImGui::MenuItem("Move to top") && i != 0) {
								std::rotate(value.begin(), value.begin() + i, value.begin() + i + 1);
								vec_response.changed();
							}
							if (ImGui::MenuItem("Move to bottom") && i != static_cast<int>(item_count) - 1) {
								std::rotate(value.begin() + i, value.begin() + i + 1, value.end());
								vec_response.changed();
							}
							ImGui::Separator();
						}
						if constexpr (default_constructible) {
							//only allow insert if type is default constructible
							//insert above/below
							if (is_insertable && ImGui::MenuItem("Insert above")) {
								value.insert(value.begin() + i, T());
								vec_response.changed();
							}
							if (is_insertable && ImGui::MenuItem("Insert below")) {
								value.insert(value.begin() + i + 1, T());
								vec_response.changed();
							}
						} else {
							ImGui::BeginDisabled();
							if (ImGui::MenuItem("Insert above")) {
							}
							if (ImGui::MenuItem("Insert below")) {
							}
							ImGui::EndDisabled();
							Detail::imgui_tooltip("Type is not default constructible or container is const, cannot insert new item");
						}

						ImGui::Separator();
						if (is_removable && ImGui::MenuItem("Clear all")) {
							value.clear();
							vec_response.changed();
							ImGui::EndPopup();
							ImGui::PopID();
							break; /* Important, as the vector has changed */
						}
						ImGui::EndPopup();
					}
				} else {
					if (ImGui::BeginPopup("item_context_menu")) {
						ImGui::TextDisabled("Value is const, cannot modify items");
						ImGui::EndPopup();
					}
				}

				ImGui::PopID();
			}
			if (!is_dropdown) ImGui::Unindent();
		}
		if (is_dropdown && is_open) {
			ImGui::TreePop();
		}
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::vector<T>& value, ImSettings& settings, ImResponse& response) {
		vector_input<T, false>(settings, response, label, value);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::vector<T>& value, ImSettings& settings, ImResponse& response) {
		vector_input<T, true>(settings, response, label, value);
	}
}
