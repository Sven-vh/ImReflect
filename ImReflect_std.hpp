#pragma once
#include <imgui.h>
#include <imgui_stdlib.h>

#include "ImReflect_helper.hpp"

#include <extern/magic_enum/magic_enum.hpp>
#include <extern/svh/scope.hpp>

#include <type_traits>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <array>
#include <deque>
#include <tuple>
#include <set>
#include <unordered_set>
#include <forward_list>
#include <variant>

/* Helpers */
namespace ImReflect::Detail {
	template<typename T> /* All numbers except bool */
	constexpr bool is_string_type_v = std::is_same_v<std::remove_cv_t<T>, std::string>;

	template<typename T>
	using enable_if_string_t = std::enable_if_t<is_string_type_v<T>, void>;

	/* Is Smart pointers */
	template<typename T>
	struct is_smart_pointer_impl : std::false_type {};
	template<typename T>
	struct is_smart_pointer_impl<std::shared_ptr<T>> : std::true_type {};
	template<typename T>
	struct is_smart_pointer_impl<std::unique_ptr<T>> : std::true_type {};
	template<typename T>
	struct is_smart_pointer_impl<std::weak_ptr<T>> : std::true_type {};

	template<typename T>
	constexpr bool is_smart_pointer_v = Detail::is_smart_pointer_impl<std::remove_cv_t<T>>::value;

	template<typename T>
	using enable_if_smart_pointer_t = std::enable_if_t<is_smart_pointer_v<T>, void>;
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
		type_settings<T>& as_multiline() { _multi_line = true; RETURN_THIS; }
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

	/// <summary>
	/// Whether or not to put a newline between label and input field
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<typename T>
	struct same_line_mixin {
	private:
		bool _same_line = false;
	public:
		type_settings<T>& same_line(const bool v = true) { _same_line = v; RETURN_THIS; }
		const bool& on_same_line() const { return _same_line; };
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
		bool _pop_up_on_insert = true;
	public:
		type_settings<T>& insertable(const bool v = true) { _insertable = v; RETURN_THIS; }
		const bool& is_insertable() const { return _insertable; };
		type_settings<T>& pop_up_on_insert(const bool v = true) { _pop_up_on_insert = v; RETURN_THIS; }
		const bool& is_pop_up_on_insert() const { return _pop_up_on_insert; };
	};

	template<typename T>
	struct removable_mixin {
	private:
		bool _removable = true;
	public:
		type_settings<T>& removable(const bool v = true) { _removable = v; RETURN_THIS; }
		const bool& is_removable() const { return _removable; };
	};

	template<typename T>
	struct resettable_mixin {
	private:
		bool _resettable = false;
	public:
		type_settings<T>& resettable(const bool v = true) { _resettable = v; RETURN_THIS; }
		const bool& is_resettable() const { return _resettable; };
	};
}

/* Input fields for std types */
namespace ImReflect {
	/* ========================= std::string ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_string_t<T>> : ImRequired<T>,
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
					lines = std::max<std::size_t>(lines, 1); /*  at least one line */
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

	/* ========================= Smart pointers ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_smart_pointer_t<T>> : ImRequired<T> {
	};

	template<typename T>
	std::enable_if_t<Detail::is_smart_pointer_v<T>>
		tag_invoke(Detail::ImInputLib_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
		auto& ptr_settings = settings.get<T>();
		auto& ptr_response = response.get<T>();

		//constexpr bool is_const = std::is_const_v<T>;

		if constexpr (std::is_same_v<std::remove_cv_t<T>, std::weak_ptr<typename T::element_type>>) {
			if (value.expired() == false) {
				auto shared_value = value.lock();
				if (shared_value) {
					ImReflect::Input(label, *shared_value, ptr_settings, ptr_response);
				} else {
					ImGui::TextDisabled("%s: ", label);
					ImGui::SameLine();
					ImGui::TextDisabled("expired");
					ImReflect::Detail::imgui_tooltip("Value is expired");
				}
			} else {
				ImGui::TextDisabled("%s: ", label);
				ImGui::SameLine();
				ImGui::TextDisabled("nullptr");
				ImReflect::Detail::imgui_tooltip("Value is nullptr");
			}
		} else {
			if (value) {
				ImReflect::Input(label, *value, ptr_settings, ptr_response);
			} else {
				ImGui::TextDisabled("%s: ", label);
				ImGui::SameLine();
				ImGui::TextDisabled("nullptr");
				ImReflect::Detail::imgui_tooltip("Value is nullptr");
			}
		}

		ImReflect::Detail::check_input_states(ptr_response);
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

		/*  fold expression */
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

	/*  Const overload, exactly the same as non-const version */
	template<typename Tag, bool is_const, typename... Ts>
	void draw_tuple_element_label(const char* label, const std::tuple<Ts...>& original_value, ImSettings& settings, ImResponse& response) {
		auto& tuple_settings = settings.get<Tag>();
		auto& tuple_response = response.get<Tag>();

		/* Get non-const reference if possible */
		auto& value = get_tuple_value<Tag, is_const, Ts...>(original_value);

		const float label_width = ImGui::CalcTextSize(label, NULL, true).x;
		const bool same_line = tuple_settings.on_same_line() || label_width == 0.0f;

		Detail::text_label(label);
		if (same_line) ImGui::SameLine();

		const auto id = Detail::scope_id("tuple");

		/* Settings to have things in a grid */
		const bool is_grid = tuple_settings.is_grid();
		const bool use_min_width = tuple_settings.has_min_width();

		ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings /*| ImGuiTableFlags_Borders*/;
		if (use_min_width) flags |= ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoKeepColumnsVisible;

		constexpr auto tuple_size = sizeof...(Ts);
		const int columns = tuple_settings.get_columns();

		int tuple_columns = tuple_size;
		if (is_grid) {
			tuple_columns = std::min<int>(columns, tuple_size);
		}

		/*  Table rendering */
		auto item_spacing = ImGui::GetStyle().ItemSpacing;
		item_spacing.x = Detail::TUPLE_ITEM_SPACING.x;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, item_spacing);
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
	struct type_settings<std_tuple> : ImRequired<std_tuple>,
		ImReflect::Detail::same_line_mixin<std_tuple>,
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
	struct type_settings<std_pair> : ImRequired<std_pair>,
		ImReflect::Detail::same_line_mixin<std_pair>,
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

	/* ========================= Container helper ========================= */
	namespace Detail {
		struct container_response {
		private:
			static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

			size_t _inserted_index = INVALID_INDEX;
			size_t _erased_index = INVALID_INDEX;

			struct move_info {
				size_t from = INVALID_INDEX;
				size_t to = INVALID_INDEX;
			};

			move_info _moved_info;

		public:
			bool has_inserted() const { return _inserted_index != INVALID_INDEX; }
			bool has_erased() const { return _erased_index != INVALID_INDEX; }
			bool has_moved() const { return _moved_info.from != INVALID_INDEX && _moved_info.to != INVALID_INDEX; }

			size_t get_inserted_index() const { return _inserted_index; }
			size_t get_erased_index() const { return _erased_index; }
			move_info get_moved_info() const { return _moved_info; }

			void inserted_index(const size_t index) { _inserted_index = index; }
			void erased_index(const size_t index) { _erased_index = index; }
			void moved_index(const size_t from, const size_t to) {
				_moved_info.from = from;
				_moved_info.to = to;
			}
		};

		constexpr const char* VECTOR_TREE_LABEL = "##vector_tree";

		template<typename T, bool is_const>
		auto& get_vector_value(const std::vector<T>& original_value) {
			if constexpr (is_const) {
				return original_value;
			} else {
				return const_cast<std::vector<T>&>(original_value);
			}
		}

		template<typename T, bool is_const>
		auto& get_container_value(const T& original_value) {
			if constexpr (is_const) {
				return original_value;
			} else {
				return const_cast<T&>(original_value);
			}
		}

		/*  Container traits to detect capabilities */
		template<typename Container>
		struct container_traits {
			using value_type = typename Container::value_type;
			using iterator = typename Container::iterator;

			/*  Detect if container has random access */
			static constexpr bool has_random_access = std::is_same_v<
				typename std::iterator_traits<iterator>::iterator_category,
				std::random_access_iterator_tag
			>;

			/*  Detect if container has push_back */
			template<typename C>
			static auto test_push_back(int) -> decltype(std::declval<C>().push_back(std::declval<value_type>()), std::true_type{});
			template<typename>
			static std::false_type test_push_back(...);
			static constexpr bool has_push_back = decltype(test_push_back<Container>(0))::value;

			/*  Detect if container has insert */
			template<typename C>
			static auto test_insert(int) -> decltype(std::declval<C>().insert(std::declval<iterator>(), std::declval<value_type>()), std::true_type{});
			template<typename>
			static std::false_type test_insert(...);
			static constexpr bool has_insert = decltype(test_insert<Container>(0))::value;

			/*  Detect if container has erase */
			template<typename C>
			static auto test_erase(int) -> decltype(std::declval<C>().erase(std::declval<iterator>()), std::true_type{});
			template<typename>
			static std::false_type test_erase(...);
			static constexpr bool has_erase = decltype(test_erase<Container>(0))::value;

			/*  Detect if container has pop front */
			template<typename C>
			static auto test_pop_front(int) -> decltype(std::declval<C>().pop_front(), std::true_type{});
			template<typename>
			static std::false_type test_pop_front(...);
			static constexpr bool has_pop_front = decltype(test_pop_front<Container>(0))::value;

			/*  Detect if container has push_front */
			template<typename C>
			static auto test_push_front(int) -> decltype(std::declval<C>().push_front(std::declval<value_type>()), std::true_type{});
			template<typename>
			static std::false_type test_push_front(...);
			static constexpr bool push_front = decltype(test_push_front<Container>(0))::value;

			/*  Check if it's std::array (fixed size) */
			static constexpr bool is_fixed_size = !has_insert && !has_erase && !has_push_back && !push_front;

			template<typename C>
			static auto test_key_type(int) -> decltype(typename C::key_type{}, std::true_type{});
			template<typename>
			static std::false_type test_key_type(...);
			static constexpr bool is_associative = decltype(test_key_type<Container>(0))::value;

			/* Check if container allows duplicates */
			template<typename C>
			static auto test_multiset(int) -> decltype(
				std::declval<C>().insert(std::declval<value_type>()),
				typename C::key_compare{},
				std::is_same<C, std::multiset<typename C::value_type>>{} ||
				std::is_same<C, std::multimap<typename C::key_type, typename C::mapped_type>>{},
				std::true_type{}
				);

			template<typename>
			static std::false_type test_multiset(...);
			static constexpr bool allows_duplicates = is_associative &&
				(std::is_same_v<Container, std::multiset<value_type>> ||
					std::is_same_v<Container, std::unordered_multiset<value_type>>);

			/* Check if container has size */
			template<typename C>
			static auto test_size(int) -> decltype(std::declval<C>().size(), std::true_type{});
			template<typename>
			static std::false_type test_size(...);
			static constexpr bool has_size = decltype(test_size<Container>(0))::value;
		};

		/*  Helper to get element by index for both random access and non-random access containers */
		template<typename Container>
		auto get_element_at(Container& container, size_t index) -> decltype(auto) {
			if constexpr (container_traits<Container>::has_random_access) {
				return container[index];
			} else {
				auto it = container.begin();
				std::advance(it, index);
				return *it;
			}
		}

		/*  Generic container input function */
		template<typename Tag, typename Container, bool is_const, bool allow_insert, bool allow_remove, bool allow_reorder, bool allow_copy>
		void container_input(const char* label, const Container& original_value, ImReflect::ImSettings& settings, ImReflect::ImResponse& response) {
			using traits = container_traits<Container>;
			using T = typename traits::value_type;

			auto& vec_settings = settings.get<Tag>();
			auto& vec_response = response.get<Tag>();

			/* Get non-const reference if possible */
			auto& value = get_container_value<Container, is_const>(original_value);

			constexpr bool default_constructible = std::is_default_constructible_v<T>;
			constexpr bool copy_constructible = std::is_copy_constructible_v<T>;
			constexpr bool move_constructible = std::is_move_constructible_v<T>;

			/*  Container capabilities */
			constexpr bool container_allows_insert = traits::has_insert || traits::has_push_back || traits::push_front;
			constexpr bool container_allows_remove = traits::has_erase || traits::has_pop_front;
			constexpr bool is_fixed_size_container = traits::is_fixed_size;
			constexpr bool has_size = traits::has_size;

			constexpr bool is_associative = traits::is_associative;
			constexpr bool supports_reorder = !is_associative; /*  Can't reorder sets */
			constexpr bool supports_duplicate = !is_associative || traits::allows_duplicates;

			/*  Final capabilities */
			constexpr bool can_insert = default_constructible && !is_const && allow_insert && container_allows_insert && !is_fixed_size_container;
			constexpr bool can_remove = !is_const && allow_remove && container_allows_remove && !is_fixed_size_container;
			constexpr bool can_reorder = move_constructible && !is_const && allow_reorder && supports_reorder;
			constexpr bool can_copy = copy_constructible && !is_const && allow_copy && container_allows_insert && supports_duplicate;

			const bool is_dropdown = vec_settings.is_dropdown();
			const bool use_min_width = vec_settings.has_min_width();
			const float min_width = vec_settings.get_min_width();

			float column_width = 0;
			if (use_min_width) {
				column_width = min_width;
			}

			const auto id = Detail::scope_id("container");
			const auto pop_up_id = ImGui::GetID("add_item_popup");

			static auto insert_item = value.end();

			ImReflect::Detail::text_label(label);
			ImGui::SameLine();
			size_t item_count = 0;
			if constexpr (has_size) {
				item_count = value.size();
			} else {
			}

			const auto disabled_plus_button = []() {
				ImGui::BeginDisabled();
				ImGui::Button("+");
				ImGui::EndDisabled();
				if constexpr (is_fixed_size_container) {
					Detail::imgui_tooltip("Container has fixed size, cannot add items");
				} else {
					Detail::imgui_tooltip("Type is not default constructible or container is const, cannot add new item");
				}
				};

			/*  Add button */
			if constexpr (can_insert) {
				if (vec_settings.is_insertable()) {
					if (ImGui::Button("+")) {
						if (vec_settings.is_pop_up_on_insert()) {
							insert_item = value.end();
							ImGui::OpenPopup(pop_up_id);
						} else {
							if constexpr (traits::has_push_back) {
								value.push_back(T{});
								vec_response.changed();
								vec_response.inserted_index(value.size() - 1);
								item_count = value.size();
							} else if constexpr (traits::has_insert) {
								value.insert(value.end(), T{});
								vec_response.changed();
								vec_response.inserted_index(value.size() - 1);
								item_count = value.size();
							} else if constexpr (traits::push_front) {
								value.push_front(T{});
								vec_response.inserted_index(0);
								vec_response.changed();
							}
						}
					}
				} else {
					disabled_plus_button();
				}
			} else {
				disabled_plus_button();
			}

			ImGui::SameLine();

			const auto disabled_minus_button = []() {
				ImGui::BeginDisabled();
				ImGui::Button("-");
				ImGui::EndDisabled();
				if constexpr (is_fixed_size_container) {
					Detail::imgui_tooltip("Container has fixed size, cannot remove items");
				} else {
					Detail::imgui_tooltip("Type is not copy/move constructible or container is const, cannot remove item");
				}
				};

			/*  Remove button */
			if constexpr (can_remove) {
				if (vec_settings.is_removable()) {
					if (item_count > 0 || traits::has_pop_front) {
						if (ImGui::Button("-")) {
							if constexpr (traits::has_erase) {
								auto it = value.end();
								--it;
								value.erase(it);
								vec_response.changed();
								vec_response.erased_index(value.size());
								item_count = value.size();
							} else {
								value.pop_front();
								vec_response.erased_index(0);
								vec_response.changed();
							}
						}
					}
				} else {
					disabled_minus_button();
				}
			} else {
				disabled_minus_button();
			}

			bool is_open = true;
			if (is_dropdown) {
				is_open = ImGui::TreeNodeEx(VECTOR_TREE_LABEL, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);
			}

			if (is_open) {
				//if (!is_dropdown) ImGui::Indent();

				/*  Drop zone at beginning (for reordering) */
				if constexpr (can_reorder) {
					if (vec_settings.is_reorderable()) {
						ImGui::BeginChild("##drop_zone_0", ImVec2(0, ImGui::GetStyle().ItemSpacing.y * 0.5f), false);
						ImGui::EndChild();

						if (ImGui::BeginDragDropTarget()) {
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTAINER_ITEM")) {
								int source_idx = *(const int*)payload->Data;
								int target_idx = 0;

								if (source_idx != target_idx) {
									auto src_it = value.begin();
									std::advance(src_it, source_idx);
									std::rotate(value.begin(), src_it, std::next(src_it));
									vec_response.changed();
									vec_response.moved_index(source_idx, target_idx);
								}
							}
							ImGui::EndDragDropTarget();
						}
					}
				}

				/*  Iterate through items */
				int i = 0;
				for (auto it = value.begin(); it != value.end(); ++it, ++i) {
					const auto item_id = Detail::scope_id(i);
					const auto indent = Detail::scope_indent();

					bool right_clicked = false;

					/*  Drag handle for reordering */
					if constexpr (can_reorder) {
						if (vec_settings.is_reorderable()) {
							ImGui::Text("==");
							if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
								ImGui::SetDragDropPayload("CONTAINER_ITEM", &i, sizeof(int));
								ImGui::PushItemWidth(column_width);
								ImReflect::Input("##container_item", *it, vec_settings, vec_response);
								ImGui::PopItemWidth();
								ImGui::EndDragDropSource();
							}
							if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
								right_clicked = true;
							}
							ImGui::SameLine();
						}
					}

					/*  Element input */
					ImGui::PushItemWidth(column_width);
					ImReflect::Input("##container_item", *it, vec_settings, vec_response);
					ImGui::PopItemWidth();

					/*  Drop zone between items */
					const float item_spacing_y = ImGui::GetStyle().ItemSpacing.y;
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - item_spacing_y);
					ImGui::BeginChild("##spacer", ImVec2(0, item_spacing_y), false);
					ImGui::EndChild();
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - item_spacing_y);

					/*  Drop target for reordering */
					if constexpr (can_reorder) {
						if (vec_settings.is_reorderable()) {
							if (ImGui::BeginDragDropTarget()) {
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTAINER_ITEM")) {
									int source_idx = *(const int*)payload->Data;
									int target_idx = i + 1;

									if (source_idx != target_idx) {
										auto src_it = value.begin();
										std::advance(src_it, source_idx);
										auto tgt_it = value.begin();
										std::advance(tgt_it, target_idx);

										if (source_idx < target_idx) {
											std::rotate(src_it, std::next(src_it), tgt_it);
											vec_response.moved_index(source_idx, target_idx - 1);
										} else {
											std::rotate(tgt_it, src_it, std::next(src_it));
											vec_response.moved_index(source_idx, target_idx);
										}
										vec_response.changed();
									}
								}
								ImGui::EndDragDropTarget();
							}
						}
					}

					/*  Context menu */
					if (right_clicked) {
						ImGui::OpenPopup("item_context_menu");
					}


					if (ImGui::BeginPopup("item_context_menu")) {
						/*  Remove item */
						if constexpr (can_remove) {
							if (vec_settings.is_removable() && ImGui::MenuItem("Remove item")) {
								//value.erase(it);
								if constexpr (traits::has_erase) {
									it = value.erase(it);
								} else {
									value.pop_front();
									it = value.begin();
								}
								vec_response.changed();
								vec_response.erased_index(i);
								ImGui::EndPopup();
								break;
							}
						} else {
							ImGui::BeginDisabled();
							ImGui::MenuItem("Remove item");
							ImGui::EndDisabled();

							if constexpr (is_fixed_size_container) Detail::imgui_tooltip("Container has fixed size, cannot remove item");
							else Detail::imgui_tooltip("Type is not copy/move constructible, cannot remove item");
						}

						/*  Duplicate item */
						if constexpr (can_copy) {
							if (vec_settings.is_insertable() && ImGui::MenuItem("Duplicate item")) {
								if constexpr (traits::push_front) {
									value.push_front(*it);
								} else {
									value.insert(std::next(it), *it);
								}
								vec_response.changed();
								ImGui::EndPopup();
								break;
							}
						} else {
							ImGui::BeginDisabled();
							ImGui::MenuItem("Duplicate item");
							ImGui::EndDisabled();
							if constexpr (is_fixed_size_container) Detail::imgui_tooltip("Container has fixed size, cannot duplicate item");
							else Detail::imgui_tooltip("Type is not copy constructible, cannot duplicate item");
						}

						ImGui::Separator();

						/*  Move operations */
						if constexpr (can_reorder) {
							if (vec_settings.is_reorderable()) {
								if (ImGui::MenuItem("Move up") && i != 0) {
									auto prev_it = std::prev(it);
									std::iter_swap(it, prev_it);
									vec_response.changed();
									vec_response.moved_index(i, i - 1);
								}
								if (ImGui::MenuItem("Move down") && i != static_cast<int>(item_count) - 1) {
									auto next_it = std::next(it);
									std::iter_swap(it, next_it);
									vec_response.changed();
									vec_response.moved_index(i, i + 1);
								}
								ImGui::Separator();

								if (ImGui::MenuItem("Move to top") && i != 0) {
									std::rotate(value.begin(), it, std::next(it));
									vec_response.changed();
									vec_response.moved_index(i, 0);
								}
								if (ImGui::MenuItem("Move to bottom") && i != static_cast<int>(item_count) - 1) {
									std::rotate(it, std::next(it), value.end());
									vec_response.changed();
									vec_response.moved_index(i, item_count - 1);
								}
								ImGui::Separator();
							}
						}

						/*  Insert operations */
						if constexpr (can_insert) {
							if (vec_settings.is_insertable() && ImGui::MenuItem("Insert above")) {
								if constexpr (supports_duplicate) {
									if (vec_settings.is_pop_up_on_insert()) {
										insert_item = it;
										ImGui::OpenPopup(pop_up_id);
									} else {
										/*value.insert(it, T());
										vec_response.changed();*/
										if constexpr (traits::has_push_back) {
											value.push_back(T{});
											vec_response.inserted_index(value.size() - 1);
										} else if constexpr (traits::has_insert) {
											value.insert(it, T{});
											vec_response.inserted_index(std::distance(value.begin(), it));
										} else if constexpr (traits::push_front) {
											value.push_front(T{});
											vec_response.inserted_index(0);
										}
										vec_response.changed();
										ImGui::EndPopup();
										break;
									}
								} else {
									ImGui::OpenPopup(pop_up_id);
								}
							}
							if (vec_settings.is_insertable() && ImGui::MenuItem("Insert below")) {
								if constexpr (supports_duplicate) {
									auto next_it = std::next(it);
									if (vec_settings.is_pop_up_on_insert()) {
										insert_item = next_it;
										ImGui::OpenPopup(pop_up_id);
									} else {
										/*value.insert(next_it, T());
										vec_response.changed();*/
										if constexpr (traits::has_push_back) {
											value.push_back(T{});
											vec_response.inserted_index(value.size() - 1);
										} else if constexpr (traits::has_insert) {
											value.insert(next_it, T{});
											vec_response.inserted_index(std::distance(value.begin(), next_it));
										} else if constexpr (traits::push_front) {
											value.push_front(T{});
											vec_response.inserted_index(0);
										}
										vec_response.changed();
										ImGui::EndPopup();
										break;
									}
								} else {
									ImGui::OpenPopup(pop_up_id);
								}
							}
							ImGui::Separator();
						} else {
							ImGui::BeginDisabled();
							ImGui::MenuItem("Insert above");
							ImGui::EndDisabled();
							if constexpr (is_fixed_size_container) Detail::imgui_tooltip("Container has fixed size, cannot add items");
							else Detail::imgui_tooltip("Type is not default constructible or container is const, cannot add new item");

							ImGui::BeginDisabled();
							ImGui::MenuItem("Insert below");
							ImGui::EndDisabled();
							if constexpr (is_fixed_size_container) Detail::imgui_tooltip("Container has fixed size, cannot add items");
							else Detail::imgui_tooltip("Type is not default constructible or container is const, cannot add new item");
						}


						/*  Clear all */
						if constexpr (can_remove) {
							if (vec_settings.is_removable() && ImGui::MenuItem("Clear all")) {
								value.clear();
								vec_response.changed();
								ImGui::EndPopup();
								break;
							}
						} else {
							ImGui::BeginDisabled();
							ImGui::MenuItem("Clear all");
							ImGui::EndDisabled();
							if constexpr (is_fixed_size_container) Detail::imgui_tooltip("Container has fixed size, cannot remove items");
							else Detail::imgui_tooltip("Type is not copy/move constructible or container is const, cannot remove item");
						}

						ImGui::EndPopup();
					}
				}

				/*  Add item */
				if constexpr (can_insert) {
					if (ImGui::BeginPopupEx(pop_up_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {

						static T temp_value{};

						if (ImGui::MenuItem("Add new item")) {
							if (insert_item != value.end()) {
								if constexpr (traits::has_insert) {
									value.insert(insert_item, temp_value);
									vec_response.inserted_index(value.size() - 1);
								} else if constexpr (traits::has_push_back) {
									value.push_back(temp_value);
									vec_response.inserted_index(value.size() - 1);
								} else if constexpr (traits::push_front) {
									value.push_front(temp_value);
									vec_response.inserted_index(0);
								}
							} else {
								if constexpr (traits::has_insert) {
									value.insert(value.end(), temp_value);
									vec_response.inserted_index(value.size() - 1);
								} else if constexpr (traits::has_push_back) {
									value.push_back(temp_value);
									vec_response.inserted_index(value.size() - 1);
								} else  if constexpr (traits::push_front) {
									value.push_front(temp_value);
									vec_response.inserted_index(0);
								}
							}
							vec_response.changed();
							temp_value = T{};
							insert_item = value.end();
							ImGui::CloseCurrentPopup();
						}

						ImReflect::Input("##new_item_input", temp_value, vec_settings, vec_response);

						ImGui::EndPopup();
					}
				}

				//if (!is_dropdown) ImGui::Unindent();
			}

			if (is_dropdown && is_open) {
				ImGui::TreePop();
			}
		}
	}

	/* ========================= std::vector ========================= */
	struct std_vector {};

	template<>
	struct type_settings<std_vector> : ImRequired<std_vector>,
		ImReflect::Detail::dropdown<std_vector>,
		ImReflect::Detail::reorderable_mixin<std_vector>,
		ImReflect::Detail::insertable_mixin<std_vector>,
		ImReflect::Detail::removable_mixin<std_vector> {
	};

	template<>
	struct type_response<std_vector> :
		ImReflect::Detail::required_response<std_vector>,
		ImReflect::Detail::container_response {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::vector<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		constexpr bool all_reorder = true;
		constexpr bool allow_copy = true;

		Detail::container_input<std_vector, std::vector<T>, is_const, allow_insert, allow_remove, all_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::vector<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool all_reorder = false;
		constexpr bool allow_copy = false;

		Detail::container_input<std_vector, std::vector<T>, is_const, allow_insert, allow_remove, all_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= std::array ========================= */
	struct std_array {};

	template<>
	struct type_settings<std_array> : ImRequired<std_array>,
		ImReflect::Detail::dropdown<std_array>,
		ImReflect::Detail::reorderable_mixin<std_array> {
	};

	template<>
	struct type_response<std_array> :
		ImReflect::Detail::required_response<std_array>,
		ImReflect::Detail::container_response {
	};

	template<typename T, std::size_t N>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::array<T, N>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = true;
		constexpr bool allow_copy = false;

		Detail::container_input<std_array, std::array<T, N>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T, std::size_t N>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::array<T, N>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = false;

		Detail::container_input<std_array, std::array<T, N>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= std::list ========================= */
	struct std_list {};

	template<>
	struct type_settings<std_list> : ImRequired<std_list>,
		ImReflect::Detail::dropdown<std_list>,
		ImReflect::Detail::reorderable_mixin<std_list>,
		ImReflect::Detail::insertable_mixin<std_list>,
		ImReflect::Detail::removable_mixin<std_list> {
	};

	template<>
	struct type_response<std_list> :
		ImReflect::Detail::required_response<std_list>,
		ImReflect::Detail::container_response {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::list<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		constexpr bool allow_reorder = true;
		constexpr bool allow_copy = true;

		Detail::container_input<std_list, std::list<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::list<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = false;

		Detail::container_input<std_list, std::list<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= std::forward_list ========================= */
	struct std_forward_list {};

	template<>
	struct type_settings<std_forward_list> : ImRequired<std_forward_list>,
		ImReflect::Detail::dropdown<std_forward_list>,
		ImReflect::Detail::insertable_mixin<std_forward_list>,
		ImReflect::Detail::removable_mixin<std_forward_list> {
	};

	template<>
	struct type_response<std_forward_list> :
		ImReflect::Detail::required_response<std_forward_list>,
		ImReflect::Detail::container_response {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::forward_list<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = true;
		Detail::container_input<std_forward_list, std::forward_list<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::forward_list<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = false;
		Detail::container_input<std_forward_list, std::forward_list<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= std::deque ========================= */
	struct std_deque {};

	template<>
	struct type_settings<std_deque> : ImRequired<std_deque>,
		ImReflect::Detail::dropdown<std_deque>,
		ImReflect::Detail::reorderable_mixin<std_deque>,
		ImReflect::Detail::insertable_mixin<std_deque>,
		ImReflect::Detail::removable_mixin<std_deque> {
	};

	template<>
	struct type_response<std_deque> :
		ImReflect::Detail::required_response<std_deque>,
		ImReflect::Detail::container_response {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::deque<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		constexpr bool allow_reorder = true;
		constexpr bool allow_copy = true;

		Detail::container_input<std_deque, std::deque<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::deque<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = false;

		Detail::container_input<std_deque, std::deque<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= std::set ========================= */
	struct std_set {};

	template<>
	struct type_settings<std_set> : ImRequired<std_set>,
		ImReflect::Detail::dropdown<std_set>,
		ImReflect::Detail::insertable_mixin<std_set>,
		ImReflect::Detail::removable_mixin<std_set> {
	};

	template<>
	struct type_response<std_set> :
		ImReflect::Detail::required_response<std_set>,
		ImReflect::Detail::container_response {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::set<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = true;

		Detail::container_input<std_set, std::set<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::set<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = false;

		Detail::container_input<std_set, std::set<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= std::unordered_set ========================= */
	struct std_unordered_set {};

	template<>
	struct type_settings<std_unordered_set> : ImRequired<std_unordered_set>,
		ImReflect::Detail::dropdown<std_unordered_set>,
		ImReflect::Detail::insertable_mixin<std_unordered_set>,
		ImReflect::Detail::removable_mixin<std_unordered_set> {
	};

	template<>
	struct type_response<std_unordered_set> :
		ImReflect::Detail::required_response<std_unordered_set>,
		ImReflect::Detail::container_response {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::unordered_set<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = true;
		Detail::container_input<std_unordered_set, std::unordered_set<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::unordered_set<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = false;
		Detail::container_input<std_unordered_set, std::unordered_set<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= std::multiset ========================= */
	struct std_multiset {};

	template<>
	struct type_settings<std_multiset> : ImRequired<std_multiset>,
		ImReflect::Detail::dropdown<std_multiset>,
		ImReflect::Detail::insertable_mixin<std_multiset>,
		ImReflect::Detail::removable_mixin<std_multiset> {
	};

	template<>
	struct type_response<std_multiset> :
		ImReflect::Detail::required_response<std_multiset>,
		ImReflect::Detail::container_response {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::multiset<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = true;

		Detail::container_input<std_multiset, std::multiset<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::multiset<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = false;

		Detail::container_input<std_multiset, std::multiset<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= std::unordered_multiset ========================= */
	struct std_unordered_multiset {};

	template<>
	struct type_settings<std_unordered_multiset> : ImRequired<std_unordered_multiset>,
		ImReflect::Detail::dropdown<std_unordered_multiset>,
		ImReflect::Detail::insertable_mixin<std_unordered_multiset>,
		ImReflect::Detail::removable_mixin<std_unordered_multiset> {
	};

	template<>
	struct type_response<std_unordered_multiset> :
		ImReflect::Detail::required_response<std_unordered_multiset>,
		ImReflect::Detail::container_response {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::unordered_multiset<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = true;
		Detail::container_input<std_unordered_multiset, std::unordered_multiset<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::unordered_multiset<T>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		constexpr bool allow_reorder = false;
		constexpr bool allow_copy = false;
		Detail::container_input<std_unordered_multiset, std::unordered_multiset<T>, is_const, allow_insert, allow_remove, allow_reorder, allow_copy>(label, value, settings, response);
	}

	/* ========================= Key / value container ========================= */
	/* map, multimap, unordered_map, unordered_multimap */

	namespace Detail {
		template<typename Container>
		struct map_traits {
			using key_type = typename Container::key_type;
			using mapped_type = typename Container::mapped_type;
		};

		/* Get map value */
		template<typename Container, bool is_const>
		auto& get_map_value(const Container& original_value) {
			if constexpr (is_const) {
				return original_value;
			} else {
				return const_cast<Container&>(original_value);
			}
		}

		/*  Generic map input function */
		template<typename Tag, typename Container, bool is_const, bool allow_insert, bool allow_remove>
		static void map_input(const char* label, const Container& original_value, ImSettings& settings, ImResponse& response) {
			using traits = map_traits<Container>;
			using K = typename traits::key_type;
			using V = typename traits::mapped_type;

			auto& map_settings = settings.get<Tag>();
			auto& map_response = response.get<Tag>();

			/* Get non-const reference if possible */
			auto& value = get_map_value<Container, is_const>(original_value);

			const auto id = Detail::scope_id("map");
			const auto pop_up_id = ImGui::GetID("add_map_item_popup");

			ImReflect::Detail::text_label(label);

			ImGui::SameLine();

			// + and - buttons
			constexpr bool default_constructible = std::is_default_constructible_v<K> && std::is_default_constructible_v<V>;
			constexpr bool copy_constructible = std::is_copy_constructible_v<K> && std::is_copy_constructible_v<V>;

			constexpr bool can_insert = default_constructible && !is_const && allow_insert;
			constexpr bool can_remove = copy_constructible && !is_const && allow_remove;

			const auto disabled_plus_button = []() {
				ImGui::BeginDisabled();
				ImGui::Button("+");
				ImGui::EndDisabled();
				if constexpr (!default_constructible) {
					Detail::imgui_tooltip("Key or value type is not default constructible or container is const, cannot add new item");
				} else {
					Detail::imgui_tooltip("Container is const or insertion disabled in settings, cannot add new item");
				}
				};

			/*  Add button */
			if constexpr (allow_insert && can_insert) {
				if (ImGui::Button("+")) {
					ImGui::OpenPopup(pop_up_id);
				}
			} else {
				disabled_plus_button();
			}


			ImGui::SameLine();

			const auto disabled_minus_button = []() {
				ImGui::BeginDisabled();
				ImGui::Button("-");
				ImGui::EndDisabled();
				if constexpr (!copy_constructible) {
					Detail::imgui_tooltip("Key or value type is not copy/move constructible or container is const, cannot remove item");
				} else {
					Detail::imgui_tooltip("Container is const or removal disabled in settings, cannot remove item");
				}
				};

			/*  Remove button */
			if constexpr (allow_remove && can_remove) {
				if (!value.empty() && ImGui::Button("-")) {
					auto it = value.end();
					--it;
					value.erase(it);
					map_response.changed();
				}
			} else {
				disabled_minus_button();
			}

			int i = 0;
			for (auto it = value.begin(); it != value.end(); ++it) {
				const auto& key = it->first;
				auto& val = it->second;


				auto pair = std::tie(key, val);

				const std::string item_label = std::string("##map_item_") + std::to_string(i);
				const auto item_id = Detail::scope_id(item_label.c_str());
				ImGui::Indent();

				ImGui::Text("==");

				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup((std::string("map_item_context_") + std::to_string(i)).c_str());
				}

				ImGui::SameLine();
				map_settings.push<std::tuple>().same_line(true);
				ImReflect::Input(item_label.c_str(), pair, map_settings, map_response);

				/*  Context menu */
				if (ImGui::BeginPopup((std::string("map_item_context_") + std::to_string(i)).c_str())) {
					/*  Remove item */
					if constexpr (can_remove) {
						if (map_settings.is_removable() && ImGui::MenuItem("Remove item")) {
							value.erase(it);
							map_response.changed();
							ImGui::EndPopup();
							break;
						}
					} else {
						ImGui::BeginDisabled();
						ImGui::MenuItem("Remove item");
						ImGui::EndDisabled();
						if constexpr (!copy_constructible) Detail::imgui_tooltip("Key or value type is not copy/move constructible or container is const, cannot remove item");
						else Detail::imgui_tooltip("Container is const or removal disabled in settings, cannot remove item");
					}
					/*  Clear all */
					if constexpr (can_remove) {
						if (map_settings.is_removable() && ImGui::MenuItem("Clear all")) {
							value.clear();
							map_response.changed();
							ImGui::EndPopup();
							break;
						}
					} else {
						ImGui::BeginDisabled();
						ImGui::MenuItem("Clear all");
						ImGui::EndDisabled();
						if constexpr (!copy_constructible) Detail::imgui_tooltip("Key or value type is not copy/move constructible or container is const, cannot remove item");
						else Detail::imgui_tooltip("Container is const or removal disabled in settings, cannot remove item");
					}
					ImGui::EndPopup();
				}

				ImGui::Unindent();

				++i;
			}

			/*  Add item popup */
			if constexpr (can_insert) {
				if (ImGui::BeginPopupEx(pop_up_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {
					static K temp_key{};
					static V temp_value{};
					if (ImGui::MenuItem("Add new item")) {
						value.emplace(temp_key, temp_value);
						map_response.changed();
						temp_key = K{};
						temp_value = V{};
						ImGui::CloseCurrentPopup();
					}
					ImReflect::Input("##new_map_key", temp_key, map_settings, map_response);
					ImReflect::Input("##new_map_value", temp_value, map_settings, map_response);
					ImGui::EndPopup();
				}
			}
		}
	}


	/* ========================= std::map ========================= */
	struct std_map {};

	template<>
	struct type_settings<std_map> : ImRequired<std_map>,
		ImReflect::Detail::dropdown<std_map>,
		ImReflect::Detail::insertable_mixin<std_map>,
		ImReflect::Detail::removable_mixin<std_map> {
	};

	template<typename K, typename V>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::map<K, V>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;

		Detail::map_input<std_map, std::map<K, V>, is_const, allow_insert, allow_remove>(label, value, settings, response);
	}

	template<typename K, typename V>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::map<K, V>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;

		Detail::map_input<std_map, std::map<K, V>, is_const, allow_insert, allow_remove>(label, value, settings, response);
	}

	/* ========================= std::unordered_map ========================= */
	struct std_unordered_map {};

	template<>
	struct type_settings<std_unordered_map> : ImRequired<std_unordered_map>,
		ImReflect::Detail::dropdown<std_unordered_map>,
		ImReflect::Detail::insertable_mixin<std_unordered_map>,
		ImReflect::Detail::removable_mixin<std_unordered_map> {
	};

	template<typename K, typename V>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::unordered_map<K, V>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		Detail::map_input<std_unordered_map, std::unordered_map<K, V>, is_const, allow_insert, allow_remove>(label, value, settings, response);
	}

	template<typename K, typename V>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::unordered_map<K, V>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		Detail::map_input<std_unordered_map, std::unordered_map<K, V>, is_const, allow_insert, allow_remove>(label, value, settings, response);
	}

	/* ========================= std::multimap ========================= */
	struct std_multimap {};

	template<>
	struct type_settings<std_multimap> : ImRequired<std_multimap>,
		ImReflect::Detail::dropdown<std_multimap>,
		ImReflect::Detail::insertable_mixin<std_multimap>,
		ImReflect::Detail::removable_mixin<std_multimap> {
	};

	template<typename K, typename V>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::multimap<K, V>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		Detail::map_input<std_multimap, std::multimap<K, V>, is_const, allow_insert, allow_remove>(label, value, settings, response);
	}

	template<typename K, typename V>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::multimap<K, V>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		Detail::map_input<std_multimap, std::multimap<K, V>, is_const, allow_insert, allow_remove>(label, value, settings, response);
	}

	/* ========================= std::unordered_multimap ========================= */
	struct std_unordered_multimap {};

	template<>
	struct type_settings<std_unordered_multimap> : ImRequired<std_unordered_multimap>,
		ImReflect::Detail::dropdown<std_unordered_multimap>,
		ImReflect::Detail::insertable_mixin<std_unordered_multimap>,
		ImReflect::Detail::removable_mixin<std_unordered_multimap> {
	};

	template<typename K, typename V>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::unordered_multimap<K, V>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = false;
		constexpr bool allow_insert = true;
		constexpr bool allow_remove = true;
		Detail::map_input<std_unordered_multimap, std::unordered_multimap<K, V>, is_const, allow_insert, allow_remove>(label, value, settings, response);
	}

	template<typename K, typename V>
	void tag_invoke(Detail::ImInputLib_t, const char* label, const std::unordered_multimap<K, V>& value, ImSettings& settings, ImResponse& response) {
		constexpr bool is_const = true;
		constexpr bool allow_insert = false;
		constexpr bool allow_remove = false;
		Detail::map_input<std_unordered_multimap, std::unordered_multimap<K, V>, is_const, allow_insert, allow_remove>(label, value, settings, response);
	}

	/* ========================= std::optional ========================= */
	struct std_optional {};

	template<>
	struct type_settings<std_optional> : ImRequired<std_optional>,
		ImReflect::Detail::resettable_mixin<std_optional> {
	};

	template<typename T>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::optional<T>& value, ImSettings& settings, ImResponse& response) {
		auto& opt_settings = settings.get<std_optional>();
		auto& opt_response = response.get<std_optional>();

		ImReflect::Detail::text_label(label);
		ImGui::SameLine();

		const bool was_engaged = value.has_value();

		bool engaged = was_engaged;
		ImGui::Checkbox("##optional_engaged", &engaged);
		Detail::imgui_tooltip("Toggle whether the optional has a value");
		ImGui::SameLine();
		if (engaged) {
			if (!was_engaged) {
				value = T{};
				opt_response.changed();
			}
			ImReflect::Input("##optional_value", *value, opt_settings, opt_response);
		} else {
			if (was_engaged) {
				value.reset();
				opt_response.changed();
			}
			ImGui::TextDisabled("<nullopt>");
		}
	}

	/* ========================= std::variant ========================= */
	struct std_variant {};

	template<>
	struct type_settings<std_variant> : ImRequired<std_variant>,
		ImReflect::Detail::dropdown<std_variant> {
	};

	template<typename... Types>
	void tag_invoke(Detail::ImInputLib_t, const char* label, std::variant<Types...>& value, ImSettings& settings, ImResponse& response) {
		auto& var_settings = settings.get<std_variant>();
		auto& var_response = response.get<std_variant>();
		ImReflect::Detail::text_label(label);
		ImGui::SameLine();

		constexpr size_t type_count = sizeof...(Types);
		const int current_type = static_cast<int>(value.index());

		static const char* type_names[] = { typeid(Types).name()... };

		const std::string combo_label = std::string("##variant_type_");

		if (type_count <= 1) {
			ImGui::TextDisabled(type_names[0]);
			Detail::imgui_tooltip("Variant has only one type, cannot change type");
		} else {
			const char* current_name = current_type >= 0 && current_type < type_count
				? type_names[current_type]
				: "<valueless>";
			ImGui::PushItemWidth(ImGui::CalcTextSize(current_name).x + 30.0f);
			if (ImGui::BeginCombo(combo_label.c_str(), current_name)) {
				int i = 0;
				([&]() {
					const bool is_selected = (current_type == i);
					if (ImGui::Selectable(type_names[i], is_selected)) {
						if (!is_selected) {
							if constexpr (!std::is_default_constructible_v<Types>) {
								Detail::imgui_tooltip("Type is not default constructible, cannot change type");
								return;
							} else {
								value.template emplace<Types>();
							}
							var_response.changed();
						}
					}
					if (is_selected) ImGui::SetItemDefaultFocus();
					++i;
					}(), ...);
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();
		}

		ImGui::SameLine();
		if (current_type >= 0) {
			std::visit([&](auto& val) {
				using T = std::decay_t<decltype(val)>;
				ImReflect::Input("##variant_value", val, var_settings, var_response);
				}, value);
		} else {
			ImGui::TextDisabled("<valueless>");
			Detail::imgui_tooltip("Variant is in a valueless state due to an exception during a type change, cannot edit value");
		}
	}


}

/* ========================= Category registration ========================= */

/* Unfinished containers to alias type traits */
template<> struct svh::category_template<std::pair> { using type = ImReflect::std_pair; };
template<> struct svh::category_template<std::tuple> { using type = ImReflect::std_tuple; };
template<> struct svh::category_template<std::vector> { using type = ImReflect::std_vector; };
/* Arry doesn't work because of the size template parameter */
template<> struct svh::category_template<std::list> { using type = ImReflect::std_list; };
template<> struct svh::category_template<std::forward_list> { using type = ImReflect::std_forward_list; };
template<> struct svh::category_template<std::deque> { using type = ImReflect::std_deque; };
template<> struct svh::category_template<std::set> { using type = ImReflect::std_set; };
template<> struct svh::category_template<std::unordered_set> { using type = ImReflect::std_unordered_set; };
template<> struct svh::category_template<std::multiset> { using type = ImReflect::std_multiset; };
template<> struct svh::category_template<std::unordered_multiset> { using type = ImReflect::std_unordered_multiset; };
template<> struct svh::category_template<std::map> { using type = ImReflect::std_map; };
template<> struct svh::category_template<std::unordered_map> { using type = ImReflect::std_unordered_map; };
template<> struct svh::category_template<std::multimap> { using type = ImReflect::std_multimap; };
template<> struct svh::category_template<std::unordered_multimap> { using type = ImReflect::std_unordered_multimap; };
template<> struct svh::category_template<std::optional> { using type = ImReflect::std_optional; };
template<> struct svh::category_template<std::variant> { using type = ImReflect::std_variant; };

/* Specializations for concrete types */
template<typename T1, typename T2>
struct svh::category<std::pair<T1, T2>> { using type = ImReflect::std_pair; };
template<typename... Types>
struct svh::category<std::tuple<Types...>> { using type = ImReflect::std_tuple; };
template<typename T, typename Alloc>
struct svh::category<std::vector<T, Alloc>> { using type = ImReflect::std_vector; };
template<typename T, std::size_t N>
struct svh::category<std::array<T, N>> { using type = ImReflect::std_array; };
template<typename T, typename Alloc>
struct svh::category<std::list<T, Alloc>> { using type = ImReflect::std_list; };
template<typename T, typename Alloc>
struct svh::category<std::forward_list<T, Alloc>> { using type = ImReflect::std_forward_list; };
template<typename T, typename Alloc>
struct svh::category<std::deque<T, Alloc>> { using type = ImReflect::std_deque; };
template<typename Key, typename Compare, typename Alloc>
struct svh::category<std::set<Key, Compare, Alloc>> { using type = ImReflect::std_set; };
template<typename Key, typename Hash, typename KeyEqual, typename Alloc>
struct svh::category<std::unordered_set<Key, Hash, KeyEqual, Alloc>> { using type = ImReflect::std_unordered_set; };
template<typename Key, typename Compare, typename Alloc>
struct svh::category<std::multiset<Key, Compare, Alloc>> { using type = ImReflect::std_multiset; };
template<typename Key, typename Hash, typename KeyEqual, typename Alloc>
struct svh::category<std::unordered_multiset<Key, Hash, KeyEqual, Alloc>> { using type = ImReflect::std_unordered_multiset; };
template<typename Key, typename T, typename Compare, typename Alloc>
struct svh::category<std::map<Key, T, Compare, Alloc>> { using type = ImReflect::std_map; };
template<typename Key, typename T, typename Hash, typename KeyEqual, typename Alloc>
struct svh::category<std::unordered_map<Key, T, Hash, KeyEqual, Alloc>> { using type = ImReflect::std_unordered_map; };
template<typename Key, typename T, typename Compare, typename Alloc>
struct svh::category<std::multimap<Key, T, Compare, Alloc>> { using type = ImReflect::std_multimap; };
template<typename Key, typename T, typename Hash, typename KeyEqual, typename Alloc>
struct svh::category<std::unordered_multimap<Key, T, Hash, KeyEqual, Alloc>> { using type = ImReflect::std_unordered_multimap; };
template<typename T>
struct svh::category<std::optional<T>> { using type = ImReflect::std_optional; };
template<typename... Types>
struct svh::category<std::variant<Types...>> { using type = ImReflect::std_variant; };