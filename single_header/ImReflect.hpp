// ============================================================================
// ImReflect - Single Header File
// https://github.com/Sven-vh/ImReflect
//
// Auto-generated - DO NOT EDIT MANUALLY
//
// This file combines all ImReflect headers and external dependencies.
// Include this file instead of ImReflect.hpp for single-header usage.
//
// Note: You still need to include imgui.h, imgui_internal.h, and imgui_stdlib.h
//       separately as they are not part of this single header.
// ============================================================================

#pragma once

// Required ImGui includes (you must have ImGui in your project)
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

// Standard library includes
#include <string>
#include <vector>
#include <array>
#include <map>
#include <optional>
#include <variant>
#include <functional>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <cstddef>
#include <cstdint>


// ============================================================================
// File: extern/svh/tag_invoke.hpp
// ============================================================================

//Source: https://github.com/Sven-vh/tag-invoke
#include <type_traits>
#include <utility>

namespace svh {

	// ADL anchor: no definition on purpose.
	void tag_invoke();

	// result type of a valid tag_invoke
	template<class Tag, class... Args>
	using tag_invoke_result_t = decltype(tag_invoke(std::declval<Tag>(), std::declval<Args>()...));

	template<class, class Tag, class... Args>
	struct is_tag_invocable_impl : std::false_type {};

	template<class Tag, class... Args>
	struct is_tag_invocable_impl<std::void_t<tag_invoke_result_t<Tag, Args...>>, Tag, Args...> : std::true_type {
	};

	template<class Tag, class... Args>
	using is_tag_invocable = is_tag_invocable_impl<void, Tag, Args...>;

	template<class Tag, class... Args>
	inline constexpr bool is_tag_invocable_v = is_tag_invocable<Tag, Args...>::value;

	template<class T> struct always_false : std::false_type {};

} // namespace svh

// ============================================================================
// File: extern/svh/scope.hpp
// ============================================================================

#include <unordered_map>
#include <map>
#include <typeindex>
#include <iostream>
#include <stdexcept>
#include <memory>

/* Whether to insert a default object when calling get at root level if not found in any scope*/
#ifndef SVH_AUTO_INSERT
#define SVH_AUTO_INSERT true
#endif

namespace svh {

	/*
	Allow users to category specific types to a single type
	e.g. std::vector<int> and std::vector<bool> = std_container
	Uses type traits
	*/
	template<typename T>
	struct category {
		using type = T;
	};

	struct template_type {};

	template<template<typename...> typename T>
	struct category_template {
		using type = template_type;
	};

	template<typename T>
	using category_t = typename category<T>::type;

	template<template<typename...> typename T>
	using category_template_t = typename category_template<T>::type;

	/* Simplify a type by removing const + volitile and category them based on user inputs */
	template<typename T>
	struct simplify {
		using type = category_t<std::remove_cv_t<T>>;
	};

	template<template<typename...> typename T>
	struct simplify_template {
		using type = category_template_t<T>;
	};

	template<typename T>
	using simplify_t = typename simplify<T>::type;

	template<template<typename...> typename T>
	using simplify_template_t = typename simplify_template<T>::type;

	/* Base template*/
	template<typename T>
	struct member_pointer_traits;

	/* Specialization for member object pointers */
	template<typename M, typename T>
	struct member_pointer_traits<M T::*> {
		using member_type = simplify_t<M>;
		using class_type = simplify_t<T>;
	};

	template<auto member>
	std::size_t get_member_offset() {
		using traits = member_pointer_traits<decltype(member)>;
		using C = typename traits::class_type;
		return reinterpret_cast<std::size_t>(&(reinterpret_cast<C const volatile*>(0)->*member));
	}
}

namespace svh {

	template<template<class> class BaseTemplate>
	struct scope {
	private:
		struct member_id; // Forward declare
	public:

		virtual ~scope() = default; // Needed for dynamic_cast
		scope() = default;

		/// <summary>
		/// Push a new scope for type T. If one already exists, it is returned.
		/// Else if a parent has one, it is copied.
		/// Else, a new one is created.
		/// </summary>
		/// <typeparam name="T">The type of the scope to push</typeparam>
		/// <returns>Reference to the pushed scope</returns>
		/// <exception cref="std::runtime_error">If an existing child has an unexpected type</exception>
		template<class T>
		BaseTemplate<simplify_t<T>>& push() {
			return _push<simplify_t<T>>();
		}

		template<template<class...> class T>
		BaseTemplate<simplify_template_t<T>>& push() {
			return _push<simplify_template_t<T>>();
		}

		/// <summary>
		/// Push multiple scopes in order.
		/// </summary>
		/// <typeparam name="T">First types</typeparam>
		/// <typeparam name="U">Second type</typeparam>
		/// <typeparam name="...Rest">Chain other Types</typeparam>
		/// <returns>Reference to the last pushed scope</returns>
		template<class T, class U, class... Rest>
		auto& push() {
			auto& next = _push<simplify_t<T>>();
			/* If rest is something, we recurse */
			/* If rest is nothing, we fall back to single T push */
			return next.template push<simplify_t<U>, Rest...>();
		}

		/// <summary>
		/// Push a new scope for type T with default values.
		/// </summary>
		/// <typeparam name="T">The type of the scope to push</typeparam>
		/// <returns>Reference to the pushed scope</returns>
		/// <exception cref="std::runtime_error">If an existing child has an unexpected type</exception>
		template<class T>
		BaseTemplate<simplify_t<T>>& push_default() {
			const std::type_index key = get_type_key<simplify_t<T>>();

			/* reset if present */
			auto it = children.find(key);
			if (it != children.end()) {
				auto* found = dynamic_cast<BaseTemplate<simplify_t<T>>*>(it->second.get());
				if (!found) {
					throw std::runtime_error("Existing child has unexpected type");
				}
				*found = BaseTemplate<simplify_t<T>>{}; // Reset to default
				return *found;
			}

			/* Else create new */
			return emplace_new<simplify_t<T>>();
		}

		/// <summary>
		/// Push member settings. Creates new or returns existing.
		/// </summary>
		/// <typeparam name="member">Auto-deduced member pointer</typeparam>
		/// <returns>Reference to member settings</returns>
		template<auto member>
		auto& push_member() {
			using traits = member_pointer_traits<decltype(member)>;
			using MemberType = typename traits::member_type;
			using ClassType = typename traits::class_type;

			const std::type_index struct_type = get_type_key<ClassType>();
			const std::type_index member_type = get_type_key<MemberType>();
			const std::size_t member_offset = get_member_offset<member>();
			const auto key = member_id{ struct_type, member_type, member_offset };

			// Check if already exists in current scope
			auto it = member_children.find(key);
			if (it != member_children.end()) {
				auto* found = dynamic_cast<BaseTemplate<MemberType>*>(it->second.get());
				if (!found) {
					throw std::runtime_error("Existing member child has unexpected type");
				}
				return *found;
			}

			// Look for existing in parent to copy
			if (has_parent()) {
				auto* found = find_member<member>();
				if (found) {
					auto child = std::make_unique<BaseTemplate<MemberType>>(*found);
					auto& ref = *child;
					child->parent = this;
					child->children.clear(); // Clear inherited children
					child->member_children.clear(); // Clear inherited member children
					child->active_member = key;
					member_children.emplace(key, std::move(child));
					return ref;
				}
			}

			// Create new
			auto child = std::make_unique<BaseTemplate<MemberType>>();
			auto& ref = *child;
			child->parent = this;
			child->children.clear(); // Clear inherited children
			child->member_children.clear(); // Clear inherited member children
			child->active_member = key;
			member_children.emplace(key, std::move(child));
			return ref;
		}

		/// <summary>
		/// Pop to parent scope. Throws if at root.
		/// </summary>
		/// <returns>Reference to the parent scope</returns>
		/// <exception cref="std::runtime_error">If at root</exception>
		scope& pop(int count = 1) const {
			if (!has_parent() && count > 0) {
				throw std::runtime_error("No parent to pop to");
			}
			if (count == 1) {
				return *parent;
			}

			return parent->pop(--count);
		}

		scope& pop_to_root() {
			if (is_root()) {
				return *this;
			}
			return parent->pop_to_root();
		}

		/// <summary>
		/// Get the scope for type T. If not found, recurse to parent.
		/// If not found and at root, optionally insert a default one depending on ``SVH_AUTO_INSERT``.
		/// </summary>
		/// <typeparam name="T">The type of the scope to get</typeparam>
		/// <returns>Reference to the found scope</returns>
		/// <exception cref="std::runtime_error">If not found and at root and ``SVH_AUTO_INSERT`` is false</exception>
		template <class T>
		BaseTemplate<simplify_t<T>>& get() {
			return _get<simplify_t<T>>();
		}

		template<template<class...> class T>
		BaseTemplate<simplify_template_t<T>>& get() {
			return _get<simplify_template_t<T>>();
		}

		template <class T, class U, class... Rest>
		auto& get() {
			auto& next = _get<simplify_t<T>>();
			/* If rest is something, we recurse */
			/* If rest is nothing, we fall back to single T get */
			return next.template get<simplify_t<U>, Rest...>();
		}

		/// <summary>
		/// Get the scope for type T. If not found, recurse to parent.
		/// If not found and at root, throw.
		/// </summary>
		/// <typeparam name="T">The type of the scope to get</typeparam>
		/// <returns>Reference to the found scope</returns>
		/// <exception cref="std::runtime_error">If not found</exception>
		template <class T>
		const BaseTemplate<simplify_t<T>>& get() const {
			return _get<simplify_t<T>>();
		}

		template <class T, class U, class... Rest>
		const auto& get() const {
			auto& next = _get<simplify_t<T>>();
			/* If rest is something, we recurse */
			/* If rest is nothing, we fall back to single T get */
			return next.template get<simplify_t<U>, Rest...>();
		}

		/// Get member settings. Auto-creates if not found at root level.
		/// </summary>
		/// <typeparam name="member">Auto-deduced member pointer</typeparam>
		/// <returns>Reference to member settings</returns>
		template<auto member>
		auto& get_member() {
			auto* found = find_member<member>();
			if (found) {
				return *found;
			}

			if (SVH_AUTO_INSERT) {
				return push_member<member>();
			}

			throw std::runtime_error("Member settings not found");
		}

		/// Get member settings (const version).
		/// </summary>
		/// <typeparam name="member">Auto-deduced member pointer</typeparam>
		/// <returns>Const reference to member settings</returns>
		template<auto member>
		const auto& get_member() const {
			auto* found = find_member<member>();
			if (found) {
				return *found;
			}

			throw std::runtime_error("Member settings not found");
		}


		/// <summary>
		/// Find the scope for type T. If not found, recurse to parent.
		/// If not found and at root, return nullptr.
		/// </summary>
		/// <typeparam name="T">The type of the scope to find</typeparam>
		/// <returns>Pointer to the found scope or nullptr if not found</returns>
		/// <exception cref="std::runtime_error">If an existing child has an unexpected type</exception>
		template <class T>
		BaseTemplate<T>* find(const member_id& child_member_id = {}) const {
			const std::type_index key = get_type_key<T>();

			/* Check member map */
			if (child_member_id.is_valid()) {
				auto mit = member_children.find(child_member_id);
				if (mit != member_children.end()) {
					auto* found = dynamic_cast<BaseTemplate<T>*>(mit->second.get());
					if (!found) {
						throw std::runtime_error("Existing member child has unexpected type");
					}
					return found;
				}
			} else {
				/* Check current map */
				auto it = children.find(key);
				if (it != children.end()) {
					auto* found = dynamic_cast<BaseTemplate<T>*>(it->second.get());
					if (!found) {
						throw std::runtime_error("Existing child has unexpected type");
					}
					return found;
				}
			}

			/* Recurse to parent */
			if (has_parent()) {
				return parent->find<T>(active_member);
			}
			return nullptr; // Not found
		}

		/// <summary>
		/// Find member settings. Returns nullptr if not found.
		/// </summary>
		/// <typeparam name="member">Auto-deduced member pointer</typeparam>
		/// <returns>Pointer to member settings or nullptr if not found</returns>
		template<auto member>
		auto* find_member() const {
			using traits = member_pointer_traits<decltype(member)>;
			using MemberType = typename traits::member_type;
			using ClassType = typename traits::class_type;

			const std::type_index struct_type = get_type_key<ClassType>();
			const std::type_index member_type = get_type_key<MemberType>();
			const std::size_t member_offset = get_member_offset<member>();
			const auto key = member_id{ struct_type, member_type, member_offset };

			/* Check member map */
			auto it = member_children.find(key);
			if (it != member_children.end()) {
				auto* found = dynamic_cast<BaseTemplate<MemberType>*>(it->second.get());
				if (!found) {
					throw std::runtime_error("Existing member child has unexpected type");
				}
				return found;
			}

			/* Check in children of type ClassType */
			auto class_it = children.find(struct_type);
			if (class_it != children.end()) {
				auto* class_scope = dynamic_cast<BaseTemplate<ClassType>*>(class_it->second.get());
				if (!class_scope) {
					throw std::runtime_error("Existing child has unexpected type");
				}
				auto* found = class_scope->template find<MemberType>(key);
				if (found) {
					return found;
				}
			}

			/* Check in children of member type */
			auto member_it = children.find(member_type);
			if (member_it != children.end()) {
				auto* member_scope = dynamic_cast<BaseTemplate<MemberType>*>(member_it->second.get());
				if (!member_scope) {
					throw std::runtime_error("Existing child has unexpected type");
				}
				return member_scope;
			}

			/* Recurse to parent */
			if (has_parent()) {
				return parent->find_member<member>();
			}

			return static_cast<BaseTemplate<MemberType>*>(nullptr);
		}

		/// <summary>
		/// Find member settings using runtime instance and member reference.
		/// </summary>
		/// <typeparam name="T">Parent class type</typeparam>
		/// <typeparam name="M">Member type</typeparam>
		/// <param name="instance">Instance containing the member</param>
		/// <param name="member">Reference to the specific member</param>
		/// <returns>Pointer to member settings or nullptr if not found</returns>
		template<class T, class M>
		BaseTemplate<M>* find_member_runtime(const T& instance, const M& member) const {
			// Calculate offset using pointer arithmetic
			const char* instance_addr = reinterpret_cast<const char*>(&instance);
			const char* member_addr = reinterpret_cast<const char*>(&member);

			// Validate that member is within instance bounds
			if (member_addr < instance_addr || member_addr >= instance_addr + sizeof(T)) {
				throw std::runtime_error("Member is not within instance bounds");
			}

			const std::size_t member_offset = static_cast<std::size_t>(member_addr - instance_addr);
			const std::type_index struct_type = get_type_key<T>();
			const std::type_index member_type = get_type_key<M>();
			const auto key = member_id{ struct_type, member_type, member_offset };

			/* Check member map */
			auto it = member_children.find(key);
			if (it != member_children.end()) {
				auto* found = dynamic_cast<BaseTemplate<M>*>(it->second.get());
				if (!found) {
					throw std::runtime_error("Existing member child has unexpected type");
				}
				return found;
			}

			/* Check in children of type T */
			auto class_it = children.find(struct_type);
			if (class_it != children.end()) {
				auto* class_scope = dynamic_cast<BaseTemplate<T>*>(class_it->second.get());
				if (!class_scope) {
					throw std::runtime_error("Existing child has unexpected type");
				}
				auto* found = class_scope->template find<M>();
				if (found) {
					return found;
				}
			}

			/* Recurse to parent */
			if (has_parent()) {
				return parent->find_member_runtime(instance, member);
			}

			return nullptr;
		}

		/// <summary>
		/// Get member settings using runtime instance and member reference.
		/// </summary>
		/// <typeparam name="T">Parent class type</typeparam>
		/// <typeparam name="M">Member type</typeparam>
		/// <param name="instance">Instance containing the member</param>
		/// <param name="member">Reference to the specific member</param>
		/// <returns>Reference to member settings</returns>
		template<class T, class M>
		BaseTemplate<M>& get_member(const T& instance, const M& member) {
			auto* found = find_member_runtime(instance, member);
			if (found) {
				return *found;
			}

			if (SVH_AUTO_INSERT) {
				// Create new member settings at runtime
				const char* instance_addr = reinterpret_cast<const char*>(&instance);
				const char* member_addr = reinterpret_cast<const char*>(&member);
				const std::size_t member_offset = static_cast<std::size_t>(member_addr - instance_addr);
				const std::type_index struct_type = get_type_key<T>();
				const std::type_index member_type = get_type_key<M>();
				const auto key = member_id{ struct_type, member_type, member_offset };

				auto child = std::make_unique<BaseTemplate<M>>();
				auto& ref = *child;
				child->parent = this;
				member_children.emplace(key, std::move(child));
				return ref;
			}

			throw std::runtime_error("Member settings not found");
		}

		/// <summary>
		/// Get member settings using runtime instance and member reference (const version).
		/// </summary>
		/// <typeparam name="T">Parent class type</typeparam>
		/// <typeparam name="M">Member type</typeparam>
		/// <param name="instance">Instance containing the member</param>
		/// <param name="member">Reference to the specific member</param>
		/// <returns>Const reference to member settings</returns>
		template<class T, class M>
		const BaseTemplate<M>& get_member(const T& instance, const M& member) const {
			auto* found = find_member_runtime(instance, member);
			if (found) {
				return *found;
			}

			throw std::runtime_error("Member settings not found");
		}

		/// <summary>
		/// Debug log the scope tree to console.
		/// </summary>
		/// <param name="indent">Indentation level</param>
		void debug_log(int indent = 0) const {
			std::string prefix(indent * 2, '=');
			for (const auto& pair : children) {
				const auto& key = pair.first;
				const auto& child = pair.second;
				const auto& name = key.name();
				std::cout << prefix << name << "\n";
				child->debug_log(indent + 2);
			}
			for (const auto& item : member_children) {
				const auto& key = item.first;
				const auto& child = item.second;
				const auto& struct_name = key.struct_type.name();
				const auto& member_name = key.member_type.name();
				std::cout << prefix << struct_name << "::(offset " << key.offset << ") -> " << member_name << "\n";
				child->debug_log(indent + 2);
			}
		}
	private:
		struct member_id {
			std::type_index struct_type = typeid(void);
			std::type_index member_type = typeid(void);
			std::size_t offset = std::numeric_limits<std::size_t>::max();

			bool is_valid() const {
				return struct_type != typeid(void) && member_type != typeid(void) && offset != std::numeric_limits<std::size_t>::max();
			}

			bool operator==(const member_id& other) const {
				return struct_type == other.struct_type && member_type == other.member_type && offset == other.offset;
			}
		};
		struct member_key_hash {
			std::size_t operator()(const member_id& k) const {
				return std::hash<std::type_index>()(k.struct_type) ^ std::hash<std::type_index>()(k.member_type) ^ std::hash<std::size_t>()(k.offset);
			}
		};
	protected:
		scope* parent = nullptr; /* Root level */

		/* type -> scope */
		std::unordered_map<std::type_index, std::shared_ptr<scope>> children; /* shared since we need to copy the base*/
		/* (struct type + member type + offset) -> scope */
		std::unordered_map<member_id, std::shared_ptr<scope>, member_key_hash> member_children;

		member_id active_member;

		bool is_root() const { return parent == nullptr; }
		bool has_parent() const { return parent != nullptr; }

		template<class T>
		std::type_index get_type_key() const { return std::type_index{ typeid(std::decay_t<T>) }; }

		template<class T>
		BaseTemplate<T>& emplace_new() {
			const std::type_index key = get_type_key<T>();
			auto child = std::make_unique<BaseTemplate<T>>();
			auto& ref = *child;
			child->parent = this;
			child->children.clear(); /* Clear children, we only create the base settings */
			child->member_children.clear(); /* Clear member children, we only create the base settings */
			children.emplace(key, std::move(child));
			return ref;
		}

		/* Actual implementation to push */
		template<class T>
		BaseTemplate<T>& _push() {
			const std::type_index key = get_type_key<T>();

			/* Reuse if present */
			auto it = children.find(key);
			if (it != children.end()) {
				auto* found = dynamic_cast<BaseTemplate<T>*>(it->second.get());
				if (!found) {
					throw std::runtime_error("Existing child has unexpected type");
				}
				return *found;
			}

			/* copy if found recursive */
			if (has_parent()) {
				auto* found = find<T>();
				if (found) {
					auto child = std::make_unique<BaseTemplate<T>>(*found); /* Copy */
					auto& ref = *child;
					child->parent = this;
					child->children.clear(); /* Clear children, we only copy the base settings */
					child->member_children.clear(); /* Clear member children, we only copy the base settings */
					children.emplace(key, std::move(child));
					return ref;
				}
			}

			/* Else create new */
			return emplace_new<T>();
		}


		template<class T>
		BaseTemplate<T>& _get() {

			auto* found = find<T>();
			if (found) {
				return *found;
			}

			if (SVH_AUTO_INSERT) {
				return emplace_new<T>();
			}

			throw std::runtime_error("Type not found");
		}

		template<class T>
		const BaseTemplate<T>& _get() const {
			auto* found = find<T>();
			if (found) {
				return *found;
			}
			throw std::runtime_error("Type not found");
		}
	};
} // namespace svh

/* Macros for indenting */
#define ____
#define ________
#define ____________
#define ________________
#define ____________________
#define ________________________
#define ____________________________
#define ________________________________

// ============================================================================
// File: extern/magic_enum/magic_enum.hpp
// ============================================================================

//  __  __             _        ______                          _____
// |  \/  |           (_)      |  ____|                        / ____|_     _
// | \  / | __ _  __ _ _  ___  | |__   _ __  _   _ _ __ ___   | |   _| |_ _| |_
// | |\/| |/ _` |/ _` | |/ __| |  __| | '_ \| | | | '_ ` _ \  | |  |_   _|_   _|
// | |  | | (_| | (_| | | (__  | |____| | | | |_| | | | | | | | |____|_|   |_|
// |_|  |_|\__,_|\__, |_|\___| |______|_| |_|\__,_|_| |_| |_|  \_____|
//                __/ | https://github.com/Neargye/magic_enum
//               |___/  version 0.9.7
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2019 - 2024 Daniil Goncharov <neargye@gmail.com>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef NEARGYE_MAGIC_ENUM_HPP
#define NEARGYE_MAGIC_ENUM_HPP

#define MAGIC_ENUM_VERSION_MAJOR 0
#define MAGIC_ENUM_VERSION_MINOR 9
#define MAGIC_ENUM_VERSION_PATCH 7

#ifndef MAGIC_ENUM_USE_STD_MODULE
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#endif

#if defined(MAGIC_ENUM_CONFIG_FILE)
#  include MAGIC_ENUM_CONFIG_FILE
#endif

#ifndef MAGIC_ENUM_USE_STD_MODULE
#if !defined(MAGIC_ENUM_USING_ALIAS_OPTIONAL)
#  include <optional>
#endif
#if !defined(MAGIC_ENUM_USING_ALIAS_STRING)
#  include <string>
#endif
#if !defined(MAGIC_ENUM_USING_ALIAS_STRING_VIEW)
#  include <string_view>
#endif
#endif

#if defined(MAGIC_ENUM_NO_ASSERT)
#  define MAGIC_ENUM_ASSERT(...) static_cast<void>(0)
#elif !defined(MAGIC_ENUM_ASSERT)
#  include <cassert>
#  define MAGIC_ENUM_ASSERT(...) assert((__VA_ARGS__))
#endif

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-warning-option"
#  pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
#  pragma clang diagnostic ignored "-Wuseless-cast" // suppresses 'static_cast<char_type>('\0')' for char_type = char (common on Linux).
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wmaybe-uninitialized" // May be used uninitialized 'return {};'.
#  pragma GCC diagnostic ignored "-Wuseless-cast" // suppresses 'static_cast<char_type>('\0')' for char_type = char (common on Linux).
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 26495) // Variable 'static_str<N>::chars_' is uninitialized.
#  pragma warning(disable : 28020) // Arithmetic overflow: Using operator '-' on a 4 byte value and then casting the result to a 8 byte value.
#  pragma warning(disable : 26451) // The expression '0<=_Param_(1)&&_Param_(1)<=1-1' is not true at this call.
#  pragma warning(disable : 4514) // Unreferenced inline function has been removed.
#endif

// Checks magic_enum compiler compatibility.
#if defined(__clang__) && __clang_major__ >= 5 || defined(__GNUC__) && __GNUC__ >= 9 || defined(_MSC_VER) && _MSC_VER >= 1910 || defined(__RESHARPER__)
#  undef  MAGIC_ENUM_SUPPORTED
#  define MAGIC_ENUM_SUPPORTED 1
#endif

// Checks magic_enum compiler aliases compatibility.
#if defined(__clang__) && __clang_major__ >= 5 || defined(__GNUC__) && __GNUC__ >= 9 || defined(_MSC_VER) && _MSC_VER >= 1920
#  undef  MAGIC_ENUM_SUPPORTED_ALIASES
#  define MAGIC_ENUM_SUPPORTED_ALIASES 1
#endif

// Specify the calling convention for compilers that need it in order to get reliable mangled names under different
// compiler flags. In particular, MSVC allows changing the default calling convention on x86.
#if defined(__clang__) || defined(__GNUC__)
#define MAGIC_ENUM_CALLING_CONVENTION
#elif defined(_MSC_VER)
#define MAGIC_ENUM_CALLING_CONVENTION __cdecl
#endif

// Enum value must be greater or equals than MAGIC_ENUM_RANGE_MIN. By default MAGIC_ENUM_RANGE_MIN = -128.
// If need another min range for all enum types by default, redefine the macro MAGIC_ENUM_RANGE_MIN.
#if !defined(MAGIC_ENUM_RANGE_MIN)
#  define MAGIC_ENUM_RANGE_MIN -128
#endif

// Enum value must be less or equals than MAGIC_ENUM_RANGE_MAX. By default MAGIC_ENUM_RANGE_MAX = 127.
// If need another max range for all enum types by default, redefine the macro MAGIC_ENUM_RANGE_MAX.
#if !defined(MAGIC_ENUM_RANGE_MAX)
#  define MAGIC_ENUM_RANGE_MAX 127
#endif

// Improve ReSharper C++ intellisense performance with builtins, avoiding unnecessary template instantiations.
#if defined(__RESHARPER__)
#  undef MAGIC_ENUM_GET_ENUM_NAME_BUILTIN
#  undef MAGIC_ENUM_GET_TYPE_NAME_BUILTIN
#  if __RESHARPER__ >= 20230100
#    define MAGIC_ENUM_GET_ENUM_NAME_BUILTIN(V) __rscpp_enumerator_name(V)
#    define MAGIC_ENUM_GET_TYPE_NAME_BUILTIN(T) __rscpp_type_name<T>()
#  else
#    define MAGIC_ENUM_GET_ENUM_NAME_BUILTIN(V) nullptr
#    define MAGIC_ENUM_GET_TYPE_NAME_BUILTIN(T) nullptr
#  endif
#endif

namespace magic_enum {

// If need another optional type, define the macro MAGIC_ENUM_USING_ALIAS_OPTIONAL.
#if defined(MAGIC_ENUM_USING_ALIAS_OPTIONAL)
MAGIC_ENUM_USING_ALIAS_OPTIONAL
#else
using std::optional;
#endif

// If need another string_view type, define the macro MAGIC_ENUM_USING_ALIAS_STRING_VIEW.
#if defined(MAGIC_ENUM_USING_ALIAS_STRING_VIEW)
MAGIC_ENUM_USING_ALIAS_STRING_VIEW
#else
using std::string_view;
#endif

// If need another string type, define the macro MAGIC_ENUM_USING_ALIAS_STRING.
#if defined(MAGIC_ENUM_USING_ALIAS_STRING)
MAGIC_ENUM_USING_ALIAS_STRING
#else
using std::string;
#endif

using char_type = string_view::value_type;
static_assert(std::is_same_v<string_view::value_type, string::value_type>, "magic_enum::customize requires same string_view::value_type and string::value_type");
static_assert([] {
  if constexpr (std::is_same_v<char_type, wchar_t>) {
    constexpr const char     c[] =  "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789|";
    constexpr const wchar_t wc[] = L"abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789|";
    static_assert(std::size(c) == std::size(wc), "magic_enum::customize identifier characters are multichars in wchar_t.");

    for (std::size_t i = 0; i < std::size(c); ++i) {
      if (c[i] != wc[i]) {
        return false;
      }
    }
  }
  return true;
} (), "magic_enum::customize wchar_t is not compatible with ASCII.");

namespace customize {
    template <typename E, typename = void>
    struct enum_range;
}

namespace detail {
    template<typename E,typename = void>
    constexpr inline std::size_t prefix_length_or_zero = 0;

    template<typename E>
    constexpr inline auto prefix_length_or_zero<E, std::void_t<decltype(customize::enum_range<E>::prefix_length)>> = std::size_t{ customize::enum_range<E>::prefix_length };
}



namespace customize {

template<bool IsFlags = false,int Min = MAGIC_ENUM_RANGE_MIN,int Max = MAGIC_ENUM_RANGE_MAX,std::size_t PrefixLength = 0>
struct adl_info_holder {
    constexpr static int max = Max;
    constexpr static int min = Min;
    constexpr static bool is_flags =IsFlags;
    constexpr static std::size_t prefix_length = PrefixLength;

    template<int min,int max>
    constexpr static adl_info_holder<IsFlags,min,max,PrefixLength> minmax() { return {};}
    template<bool is_flag>
    constexpr static adl_info_holder<is_flag,Min,Max,PrefixLength> flag() { return {};}
    template<std::size_t prefix_len>
    constexpr static adl_info_holder<IsFlags,Min,Max,prefix_len> prefix() { return {};}
};

constexpr adl_info_holder<> adl_info()
{
     return {};
}

// Enum value must be in range [MAGIC_ENUM_RANGE_MIN, MAGIC_ENUM_RANGE_MAX]. By default MAGIC_ENUM_RANGE_MIN = -128, MAGIC_ENUM_RANGE_MAX = 127.
// If need another range for all enum types by default, redefine the macro MAGIC_ENUM_RANGE_MIN and MAGIC_ENUM_RANGE_MAX.
// If need another range for specific enum type, add specialization enum_range for necessary enum type.
template <typename E,typename /*= void*/>
struct enum_range {
    static constexpr int min = MAGIC_ENUM_RANGE_MIN;
    static constexpr int max = MAGIC_ENUM_RANGE_MAX;
};

template <typename E>
struct enum_range<E, decltype(void(magic_enum_define_range_adl(E{}))) >
: decltype(magic_enum_define_range_adl(E{})) {};

static_assert(MAGIC_ENUM_RANGE_MAX > MAGIC_ENUM_RANGE_MIN, "MAGIC_ENUM_RANGE_MAX must be greater than MAGIC_ENUM_RANGE_MIN.");

namespace detail {

enum class customize_tag {
  default_tag,
  invalid_tag,
  custom_tag
};

} // namespace magic_enum::customize::detail

class customize_t : public std::pair<detail::customize_tag, string_view> {
 public:
  constexpr customize_t(string_view srt) : std::pair<detail::customize_tag, string_view>{detail::customize_tag::custom_tag, srt} {}
  constexpr customize_t(const char_type* srt) : customize_t{string_view{srt}} {}
  constexpr customize_t(detail::customize_tag tag) : std::pair<detail::customize_tag, string_view>{tag, string_view{}} {
    MAGIC_ENUM_ASSERT(tag != detail::customize_tag::custom_tag);
  }
};

// Default customize.
inline constexpr auto default_tag = customize_t{detail::customize_tag::default_tag};
// Invalid customize.
inline constexpr auto invalid_tag = customize_t{detail::customize_tag::invalid_tag};

// If need custom names for enum, add specialization enum_name for necessary enum type.
template <typename E>
constexpr customize_t enum_name(E) noexcept {
  return default_tag;
}

// If need custom type name for enum, add specialization enum_type_name for necessary enum type.
template <typename E>
constexpr customize_t enum_type_name() noexcept {
  return default_tag;
}

} // namespace magic_enum::customize

namespace detail {

template <typename T>
struct supported
#if defined(MAGIC_ENUM_SUPPORTED) && MAGIC_ENUM_SUPPORTED || defined(MAGIC_ENUM_NO_CHECK_SUPPORT)
  : std::true_type {};
#else
  : std::false_type {};
#endif

template <auto V, typename E = std::decay_t<decltype(V)>, std::enable_if_t<std::is_enum_v<E>, int> = 0>
using enum_constant = std::integral_constant<E, V>;

template <typename... T>
inline constexpr bool always_false_v = false;

template <typename T, typename = void>
struct has_is_flags : std::false_type {};

template <typename T>
struct has_is_flags<T, std::void_t<decltype(customize::enum_range<T>::is_flags)>> : std::bool_constant<std::is_same_v<bool, std::decay_t<decltype(customize::enum_range<T>::is_flags)>>> {};

template <typename T, typename = void>
struct range_min : std::integral_constant<int, MAGIC_ENUM_RANGE_MIN> {};

template <typename T>
struct range_min<T, std::void_t<decltype(customize::enum_range<T>::min)>> : std::integral_constant<decltype(customize::enum_range<T>::min), customize::enum_range<T>::min> {};

template <typename T, typename = void>
struct range_max : std::integral_constant<int, MAGIC_ENUM_RANGE_MAX> {};

template <typename T>
struct range_max<T, std::void_t<decltype(customize::enum_range<T>::max)>> : std::integral_constant<decltype(customize::enum_range<T>::max), customize::enum_range<T>::max> {};

struct str_view {
  const char* str_ = nullptr;
  std::size_t size_ = 0;
};

template <std::uint16_t N>
class static_str {
 public:
  constexpr explicit static_str(str_view str) noexcept : static_str{str.str_, std::make_integer_sequence<std::uint16_t, N>{}} {
    MAGIC_ENUM_ASSERT(str.size_ == N);
  }

  constexpr explicit static_str(const char* const str) noexcept : static_str{ str, std::make_integer_sequence<std::uint16_t, N>{} } {
  }

  constexpr explicit static_str(string_view str) noexcept : static_str{str.data(), std::make_integer_sequence<std::uint16_t, N>{}} {
    MAGIC_ENUM_ASSERT(str.size() == N);
  }

  constexpr const char_type* data() const noexcept { return chars_; }

  constexpr std::uint16_t size() const noexcept { return N; }

  constexpr operator string_view() const noexcept { return {data(), size()}; }

 private:
  template <std::uint16_t... I>
  constexpr static_str(const char* str, std::integer_sequence<std::uint16_t, I...>) noexcept : chars_{static_cast<char_type>(str[I])..., static_cast<char_type>('\0')} {}

  template <std::uint16_t... I>
  constexpr static_str(string_view str, std::integer_sequence<std::uint16_t, I...>) noexcept : chars_{str[I]..., static_cast<char_type>('\0')} {}

  char_type chars_[static_cast<std::size_t>(N) + 1];
};

template <>
class static_str<0> {
 public:
  constexpr explicit static_str() = default;

  constexpr explicit static_str(str_view) noexcept {}

  constexpr explicit static_str(string_view) noexcept {}

  constexpr const char_type* data() const noexcept { return nullptr; }

  constexpr std::uint16_t size() const noexcept { return 0; }

  constexpr operator string_view() const noexcept { return {}; }
};

template <typename Op = std::equal_to<>>
class case_insensitive {
  static constexpr char_type to_lower(char_type c) noexcept {
    return (c >= static_cast<char_type>('A') && c <= static_cast<char_type>('Z')) ? static_cast<char_type>(c + (static_cast<char_type>('a') - static_cast<char_type>('A'))) : c;
  }

 public:
  template <typename L, typename R>
  constexpr auto operator()(L lhs, R rhs) const noexcept -> std::enable_if_t<std::is_same_v<std::decay_t<L>, char_type> && std::is_same_v<std::decay_t<R>, char_type>, bool> {
    return Op{}(to_lower(lhs), to_lower(rhs));
  }
};

constexpr std::size_t find(string_view str, char_type c) noexcept {
#if defined(__clang__) && __clang_major__ < 9 && defined(__GLIBCXX__) || defined(_MSC_VER) && _MSC_VER < 1920 && !defined(__clang__)
// https://stackoverflow.com/questions/56484834/constexpr-stdstring-viewfind-last-of-doesnt-work-on-clang-8-with-libstdc
// https://developercommunity.visualstudio.com/content/problem/360432/vs20178-regression-c-failed-in-test.html
  constexpr bool workaround = true;
#else
  constexpr bool workaround = false;
#endif

  if constexpr (workaround) {
    for (std::size_t i = 0; i < str.size(); ++i) {
      if (str[i] == c) {
        return i;
      }
    }

    return string_view::npos;
  } else {
    return str.find(c);
  }
}

template <typename BinaryPredicate>
inline constexpr bool is_default_predicate_v = std::is_same_v<std::decay_t<BinaryPredicate>, std::equal_to<string_view::value_type>> || std::is_same_v<std::decay_t<BinaryPredicate>, std::equal_to<>>;


template <typename BinaryPredicate>
inline constexpr bool is_nothrow_invocable_v = is_default_predicate_v<BinaryPredicate> || std::is_nothrow_invocable_r_v<bool, BinaryPredicate, char_type, char_type>;


template <typename BinaryPredicate>
constexpr bool cmp_equal(string_view lhs, string_view rhs, [[maybe_unused]] BinaryPredicate&& p) noexcept(is_nothrow_invocable_v<BinaryPredicate>) {
#if defined(_MSC_VER) && _MSC_VER < 1920 && !defined(__clang__)
  // https://developercommunity.visualstudio.com/content/problem/360432/vs20178-regression-c-failed-in-test.html
  // https://developercommunity.visualstudio.com/content/problem/232218/c-constexpr-string-view.html
  constexpr bool workaround = true;
#else
  constexpr bool workaround = false;
#endif

  if constexpr (!is_default_predicate_v<BinaryPredicate> || workaround) {
    if (lhs.size() != rhs.size()) {
      return false;
    }

    const auto size = lhs.size();
    for (std::size_t i = 0; i < size; ++i) {
      if (!p(lhs[i], rhs[i])) {
        return false;
      }
    }

    return true;
  } else {
    return lhs == rhs;
  }
}

template <typename L, typename R>
constexpr bool cmp_less(L lhs, R rhs) noexcept {
  static_assert(std::is_integral_v<L> && std::is_integral_v<R>, "magic_enum::detail::cmp_less requires integral type.");

  if constexpr (std::is_signed_v<L> == std::is_signed_v<R>) {
    // If same signedness (both signed or both unsigned).
    return lhs < rhs;
  } else if constexpr (std::is_same_v<L, bool>) { // bool special case
      return static_cast<R>(lhs) < rhs;
  } else if constexpr (std::is_same_v<R, bool>) { // bool special case
      return lhs < static_cast<L>(rhs);
  } else if constexpr (std::is_signed_v<R>) {
    // If 'right' is negative, then result is 'false', otherwise cast & compare.
    return rhs > 0 && lhs < static_cast<std::make_unsigned_t<R>>(rhs);
  } else {
    // If 'left' is negative, then result is 'true', otherwise cast & compare.
    return lhs < 0 || static_cast<std::make_unsigned_t<L>>(lhs) < rhs;
  }
}

template <typename I>
constexpr I log2(I value) noexcept {
  static_assert(std::is_integral_v<I>, "magic_enum::detail::log2 requires integral type.");

  if constexpr (std::is_same_v<I, bool>) { // bool special case
    return MAGIC_ENUM_ASSERT(false), value;
  } else {
    auto ret = I{0};
    for (; value > I{1}; value >>= I{1}, ++ret) {}

    return ret;
  }
}

#if defined(__cpp_lib_array_constexpr) && __cpp_lib_array_constexpr >= 201603L
#  define MAGIC_ENUM_ARRAY_CONSTEXPR 1
#else
template <typename T, std::size_t N, std::size_t... I>
constexpr std::array<std::remove_cv_t<T>, N> to_array(T (&a)[N], std::index_sequence<I...>) noexcept {
  return {{a[I]...}};
}
#endif

template <typename T>
inline constexpr bool is_enum_v = std::is_enum_v<T> && std::is_same_v<T, std::decay_t<T>>;

template <typename E>
constexpr auto MAGIC_ENUM_CALLING_CONVENTION n() noexcept {
  static_assert(is_enum_v<E>, "magic_enum::detail::n requires enum type.");

  if constexpr (supported<E>::value) {
#if defined(MAGIC_ENUM_GET_TYPE_NAME_BUILTIN)
    constexpr auto name_ptr = MAGIC_ENUM_GET_TYPE_NAME_BUILTIN(E);
    constexpr auto name = name_ptr ? str_view{name_ptr, std::char_traits<char>::length(name_ptr)} : str_view{};
#elif defined(__clang__)
    str_view name;
    if constexpr (sizeof(__PRETTY_FUNCTION__) == sizeof(__FUNCTION__)) {
      static_assert(always_false_v<E>, "magic_enum::detail::n requires __PRETTY_FUNCTION__.");
      return str_view{};
    } else {
      name.size_ = sizeof(__PRETTY_FUNCTION__) - 36;
      name.str_ = __PRETTY_FUNCTION__ + 34;
    }
#elif defined(__GNUC__)
    auto name = str_view{__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 1};
    if constexpr (sizeof(__PRETTY_FUNCTION__) == sizeof(__FUNCTION__)) {
      static_assert(always_false_v<E>, "magic_enum::detail::n requires __PRETTY_FUNCTION__.");
      return str_view{};
    } else if (name.str_[name.size_ - 1] == ']') {
      name.size_ -= 50;
      name.str_ += 49;
    } else {
      name.size_ -= 40;
      name.str_ += 37;
    }
#elif defined(_MSC_VER)
    // CLI/C++ workaround (see https://github.com/Neargye/magic_enum/issues/284).
    str_view name;
    name.str_ = __FUNCSIG__;
    name.str_ += 40;
    name.size_ += sizeof(__FUNCSIG__) - 57;
#else
    auto name = str_view{};
#endif
    std::size_t p = 0;
    for (std::size_t i = name.size_; i > 0; --i) {
      if (name.str_[i] == ':') {
        p = i + 1;
        break;
      }
    }
    if (p > 0) {
      name.size_ -= p;
      name.str_ += p;
    }
    return name;
  } else {
    return str_view{}; // Unsupported compiler or Invalid customize.
  }
}

template <typename E>
constexpr auto type_name() noexcept {
  [[maybe_unused]] constexpr auto custom = customize::enum_type_name<E>();
  static_assert(std::is_same_v<std::decay_t<decltype(custom)>, customize::customize_t>, "magic_enum::customize requires customize_t type.");
  if constexpr (custom.first == customize::detail::customize_tag::custom_tag) {
    constexpr auto name = custom.second;
    static_assert(!name.empty(), "magic_enum::customize requires not empty string.");
    return static_str<name.size()>{name};
  } else if constexpr (custom.first == customize::detail::customize_tag::invalid_tag) {
    return static_str<0>{};
  } else if constexpr (custom.first == customize::detail::customize_tag::default_tag) {
    constexpr auto name = n<E>();
    return static_str<name.size_>{name};
  } else {
    static_assert(always_false_v<E>, "magic_enum::customize invalid.");
  }
}

template <typename E>
inline constexpr auto type_name_v = type_name<E>();

template <auto V>
constexpr auto MAGIC_ENUM_CALLING_CONVENTION n() noexcept {
  static_assert(is_enum_v<decltype(V)>, "magic_enum::detail::n requires enum type.");

  if constexpr (supported<decltype(V)>::value) {
#if defined(MAGIC_ENUM_GET_ENUM_NAME_BUILTIN)
    constexpr auto name_ptr = MAGIC_ENUM_GET_ENUM_NAME_BUILTIN(V);
    auto name = name_ptr ? str_view{name_ptr, std::char_traits<char>::length(name_ptr)} : str_view{};
#elif defined(__clang__)
    str_view name;
    if constexpr (sizeof(__PRETTY_FUNCTION__) == sizeof(__FUNCTION__)) {
      static_assert(always_false_v<decltype(V)>, "magic_enum::detail::n requires __PRETTY_FUNCTION__.");
      return str_view{};
    } else {
      name.size_ = sizeof(__PRETTY_FUNCTION__) - 36;
      name.str_ = __PRETTY_FUNCTION__ + 34;
    }
    if (name.size_ > 22 && name.str_[0] == '(' && name.str_[1] == 'a' && name.str_[10] == ' ' && name.str_[22] == ':') {
      name.size_ -= 23;
      name.str_ += 23;
    }
    if (name.str_[0] == '(' || name.str_[0] == '-' || (name.str_[0] >= '0' && name.str_[0] <= '9')) {
      name = str_view{};
    }
#elif defined(__GNUC__)
    auto name = str_view{__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 1};
    if constexpr (sizeof(__PRETTY_FUNCTION__) == sizeof(__FUNCTION__)) {
      static_assert(always_false_v<decltype(V)>, "magic_enum::detail::n requires __PRETTY_FUNCTION__.");
      return str_view{};
    } else if (name.str_[name.size_ - 1] == ']') {
      name.size_ -= 55;
      name.str_ += 54;
    } else {
      name.size_ -= 40;
      name.str_ += 37;
    }
    if (name.str_[0] == '(') {
      name = str_view{};
    }
#elif defined(_MSC_VER)
    str_view name;
    if ((__FUNCSIG__[5] == '_' && __FUNCSIG__[35] != '(') || (__FUNCSIG__[5] == 'c' && __FUNCSIG__[41] != '(')) {
      // CLI/C++ workaround (see https://github.com/Neargye/magic_enum/issues/284).
      name.str_ = __FUNCSIG__;
      name.str_ += 35;
      name.size_ = sizeof(__FUNCSIG__) - 52;
    }
#else
    auto name = str_view{};
#endif
    std::size_t p = 0;
    for (std::size_t i = name.size_; i > 0; --i) {
      if (name.str_[i] == ':') {
        p = i + 1;
        break;
      }
    }
    if (p > 0) {
      name.size_ -= p;
      name.str_ += p;
    }
    return name;
  } else {
    return str_view{}; // Unsupported compiler or Invalid customize.
  }
}

#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER < 1920
#  define MAGIC_ENUM_VS_2017_WORKAROUND 1
#endif

#if defined(MAGIC_ENUM_VS_2017_WORKAROUND)
template <typename E, E V>
constexpr auto MAGIC_ENUM_CALLING_CONVENTION n() noexcept {
  static_assert(is_enum_v<E>, "magic_enum::detail::n requires enum type.");

#  if defined(MAGIC_ENUM_GET_ENUM_NAME_BUILTIN)
  constexpr auto name_ptr = MAGIC_ENUM_GET_ENUM_NAME_BUILTIN(V);
  auto name = name_ptr ? str_view{name_ptr, std::char_traits<char>::length(name_ptr)} : str_view{};
#  else
  // CLI/C++ workaround (see https://github.com/Neargye/magic_enum/issues/284).
  str_view name;
  name.str_ = __FUNCSIG__;
  name.size_ = sizeof(__FUNCSIG__) - 17;
  std::size_t p = 0;
  for (std::size_t i = name.size_; i > 0; --i) {
    if (name.str_[i] == ',' || name.str_[i] == ':') {
      p = i + 1;
      break;
    }
  }
  if (p > 0) {
    name.size_ -= p;
    name.str_ += p;
  }
  if (name.str_[0] == '(' || name.str_[0] == '-' || (name.str_[0] >= '0' && name.str_[0] <= '9')) {
    name = str_view{};
  }
  return name;
#  endif
}
#endif

template <typename E, E V>
constexpr auto enum_name() noexcept {
  [[maybe_unused]] constexpr auto custom = customize::enum_name<E>(V);
  static_assert(std::is_same_v<std::decay_t<decltype(custom)>, customize::customize_t>, "magic_enum::customize requires customize_t type.");
  if constexpr (custom.first == customize::detail::customize_tag::custom_tag) {
    constexpr auto name = custom.second;
    static_assert(!name.empty(), "magic_enum::customize requires not empty string.");
    return static_str<name.size()>{name};
  } else if constexpr (custom.first == customize::detail::customize_tag::invalid_tag) {
    return static_str<0>{};
  } else if constexpr (custom.first == customize::detail::customize_tag::default_tag) {
#if defined(MAGIC_ENUM_VS_2017_WORKAROUND)
    constexpr auto name = n<E, V>();
#else
    constexpr auto name = n<V>();
#endif
    return static_str<name.size_ - prefix_length_or_zero<E>>{name.str_ + prefix_length_or_zero<E>};
  } else {
    static_assert(always_false_v<E>, "magic_enum::customize invalid.");
  }
}

template <typename E, E V>
inline constexpr auto enum_name_v = enum_name<E, V>();

// CWG1766: Values outside the range of the values of an enumeration
// https://reviews.llvm.org/D130058, https://reviews.llvm.org/D131307
#if defined(__clang__) && __clang_major__ >= 16
template <typename E, auto V, typename = void>
inline constexpr bool is_enum_constexpr_static_cast_valid = false;
template <typename E, auto V>
inline constexpr bool is_enum_constexpr_static_cast_valid<E, V, std::void_t<std::integral_constant<E, static_cast<E>(V)>>> = true;
#else
template <typename, auto>
inline constexpr bool is_enum_constexpr_static_cast_valid = true;
#endif

template <typename E, auto V>
constexpr bool is_valid() noexcept {
  if constexpr (is_enum_constexpr_static_cast_valid<E, V>) {
    constexpr E v = static_cast<E>(V);
    [[maybe_unused]] constexpr auto custom = customize::enum_name<E>(v);
    static_assert(std::is_same_v<std::decay_t<decltype(custom)>, customize::customize_t>, "magic_enum::customize requires customize_t type.");
    if constexpr (custom.first == customize::detail::customize_tag::custom_tag) {
      constexpr auto name = custom.second;
      static_assert(!name.empty(), "magic_enum::customize requires not empty string.");
      return name.size() != 0;
    } else if constexpr (custom.first == customize::detail::customize_tag::default_tag) {
#if defined(MAGIC_ENUM_VS_2017_WORKAROUND)
      return n<E, v>().size_ != 0;
#else
      return n<v>().size_ != 0;
#endif
    } else {
      return false;
    }
  } else {
    return false;
  }
}

enum class enum_subtype {
  common,
  flags
};

template <typename E, int O, enum_subtype S, typename U = std::underlying_type_t<E>>
constexpr U ualue(std::size_t i) noexcept {
  if constexpr (std::is_same_v<U, bool>) { // bool special case
    static_assert(O == 0, "magic_enum::detail::ualue requires valid offset.");

    return static_cast<U>(i);
  } else if constexpr (S == enum_subtype::flags) {
    return static_cast<U>(U{1} << static_cast<U>(static_cast<int>(i) + O));
  } else {
    return static_cast<U>(static_cast<int>(i) + O);
  }
}

template <typename E, int O, enum_subtype S, typename U = std::underlying_type_t<E>>
constexpr E value(std::size_t i) noexcept {
  return static_cast<E>(ualue<E, O, S>(i));
}

template <typename E, enum_subtype S, typename U = std::underlying_type_t<E>>
constexpr int reflected_min() noexcept {
  if constexpr (S == enum_subtype::flags) {
    return 0;
  } else {
    constexpr auto lhs = range_min<E>::value;
    constexpr auto rhs = (std::numeric_limits<U>::min)();

    if constexpr (cmp_less(rhs, lhs)) {
      return lhs;
    } else {
      return rhs;
    }
  }
}

template <typename E, enum_subtype S, typename U = std::underlying_type_t<E>>
constexpr int reflected_max() noexcept {
  if constexpr (S == enum_subtype::flags) {
    return std::numeric_limits<U>::digits - 1;
  } else {
    constexpr auto lhs = range_max<E>::value;
    constexpr auto rhs = (std::numeric_limits<U>::max)();

    if constexpr (cmp_less(lhs, rhs)) {
      return lhs;
    } else {
      return rhs;
    }
  }
}

#define MAGIC_ENUM_FOR_EACH_256(T)                                                                                                                                                                 \
  T(  0)T(  1)T(  2)T(  3)T(  4)T(  5)T(  6)T(  7)T(  8)T(  9)T( 10)T( 11)T( 12)T( 13)T( 14)T( 15)T( 16)T( 17)T( 18)T( 19)T( 20)T( 21)T( 22)T( 23)T( 24)T( 25)T( 26)T( 27)T( 28)T( 29)T( 30)T( 31) \
  T( 32)T( 33)T( 34)T( 35)T( 36)T( 37)T( 38)T( 39)T( 40)T( 41)T( 42)T( 43)T( 44)T( 45)T( 46)T( 47)T( 48)T( 49)T( 50)T( 51)T( 52)T( 53)T( 54)T( 55)T( 56)T( 57)T( 58)T( 59)T( 60)T( 61)T( 62)T( 63) \
  T( 64)T( 65)T( 66)T( 67)T( 68)T( 69)T( 70)T( 71)T( 72)T( 73)T( 74)T( 75)T( 76)T( 77)T( 78)T( 79)T( 80)T( 81)T( 82)T( 83)T( 84)T( 85)T( 86)T( 87)T( 88)T( 89)T( 90)T( 91)T( 92)T( 93)T( 94)T( 95) \
  T( 96)T( 97)T( 98)T( 99)T(100)T(101)T(102)T(103)T(104)T(105)T(106)T(107)T(108)T(109)T(110)T(111)T(112)T(113)T(114)T(115)T(116)T(117)T(118)T(119)T(120)T(121)T(122)T(123)T(124)T(125)T(126)T(127) \
  T(128)T(129)T(130)T(131)T(132)T(133)T(134)T(135)T(136)T(137)T(138)T(139)T(140)T(141)T(142)T(143)T(144)T(145)T(146)T(147)T(148)T(149)T(150)T(151)T(152)T(153)T(154)T(155)T(156)T(157)T(158)T(159) \
  T(160)T(161)T(162)T(163)T(164)T(165)T(166)T(167)T(168)T(169)T(170)T(171)T(172)T(173)T(174)T(175)T(176)T(177)T(178)T(179)T(180)T(181)T(182)T(183)T(184)T(185)T(186)T(187)T(188)T(189)T(190)T(191) \
  T(192)T(193)T(194)T(195)T(196)T(197)T(198)T(199)T(200)T(201)T(202)T(203)T(204)T(205)T(206)T(207)T(208)T(209)T(210)T(211)T(212)T(213)T(214)T(215)T(216)T(217)T(218)T(219)T(220)T(221)T(222)T(223) \
  T(224)T(225)T(226)T(227)T(228)T(229)T(230)T(231)T(232)T(233)T(234)T(235)T(236)T(237)T(238)T(239)T(240)T(241)T(242)T(243)T(244)T(245)T(246)T(247)T(248)T(249)T(250)T(251)T(252)T(253)T(254)T(255)

template <typename E, enum_subtype S, std::size_t Size, int Min, std::size_t I>
constexpr void valid_count(bool* valid, std::size_t& count) noexcept {
#define MAGIC_ENUM_V(O)                                     \
  if constexpr ((I + O) < Size) {                           \
    if constexpr (is_valid<E, ualue<E, Min, S>(I + O)>()) { \
      valid[I + O] = true;                                  \
      ++count;                                              \
    }                                                       \
  }

  MAGIC_ENUM_FOR_EACH_256(MAGIC_ENUM_V)

  if constexpr ((I + 256) < Size) {
    valid_count<E, S, Size, Min, I + 256>(valid, count);
  }
#undef MAGIC_ENUM_V
}

template <std::size_t N>
struct valid_count_t {
  std::size_t count = 0;
  bool valid[N] = {};
};

template <typename E, enum_subtype S, std::size_t Size, int Min>
constexpr auto valid_count() noexcept {
  valid_count_t<Size> vc;
  valid_count<E, S, Size, Min, 0>(vc.valid, vc.count);
  return vc;
}

template <typename E, enum_subtype S, std::size_t Size, int Min>
constexpr auto values() noexcept {
  constexpr auto vc = valid_count<E, S, Size, Min>();

  if constexpr (vc.count > 0) {
#if defined(MAGIC_ENUM_ARRAY_CONSTEXPR)
    std::array<E, vc.count> values = {};
#else
    E values[vc.count] = {};
#endif
    for (std::size_t i = 0, v = 0; v < vc.count; ++i) {
      if (vc.valid[i]) {
        values[v++] = value<E, Min, S>(i);
      }
    }
#if defined(MAGIC_ENUM_ARRAY_CONSTEXPR)
    return values;
#else
    return to_array(values, std::make_index_sequence<vc.count>{});
#endif
  } else {
    return std::array<E, 0>{};
  }
}

template <typename E, enum_subtype S, typename U = std::underlying_type_t<E>>
constexpr auto values() noexcept {
  constexpr auto min = reflected_min<E, S>();
  constexpr auto max = reflected_max<E, S>();
  constexpr auto range_size = max - min + 1;
  static_assert(range_size > 0, "magic_enum::enum_range requires valid size.");

  return values<E, S, range_size, min>();
}

template <typename E, typename U = std::underlying_type_t<E>>
constexpr enum_subtype subtype(std::true_type) noexcept {
  if constexpr (std::is_same_v<U, bool>) { // bool special case
    return enum_subtype::common;
  } else if constexpr (has_is_flags<E>::value) {
    return customize::enum_range<E>::is_flags ? enum_subtype::flags : enum_subtype::common;
  } else {
#if defined(MAGIC_ENUM_AUTO_IS_FLAGS)
    constexpr auto flags_values = values<E, enum_subtype::flags>();
    constexpr auto default_values = values<E, enum_subtype::common>();
    if (flags_values.size() == 0 || default_values.size() > flags_values.size()) {
      return enum_subtype::common;
    }
    for (std::size_t i = 0; i < default_values.size(); ++i) {
      const auto v = static_cast<U>(default_values[i]);
      if (v != 0 && (v & (v - 1)) != 0) {
        return enum_subtype::common;
      }
    }
    return enum_subtype::flags;
#else
    return enum_subtype::common;
#endif
  }
}

template <typename T>
constexpr enum_subtype subtype(std::false_type) noexcept {
  // For non-enum type return default common subtype.
  return enum_subtype::common;
}

template <typename E, typename D = std::decay_t<E>>
inline constexpr auto subtype_v = subtype<D>(std::is_enum<D>{});

template <typename E, enum_subtype S>
inline constexpr auto values_v = values<E, S>();

template <typename E, enum_subtype S, typename D = std::decay_t<E>>
using values_t = decltype((values_v<D, S>));

template <typename E, enum_subtype S>
inline constexpr auto count_v = values_v<E, S>.size();

template <typename E, enum_subtype S, typename U = std::underlying_type_t<E>>
inline constexpr auto min_v = (count_v<E, S> > 0) ? static_cast<U>(values_v<E, S>.front()) : U{0};

template <typename E, enum_subtype S, typename U = std::underlying_type_t<E>>
inline constexpr auto max_v = (count_v<E, S> > 0) ? static_cast<U>(values_v<E, S>.back()) : U{0};

template <typename E, enum_subtype S, std::size_t... I>
constexpr auto names(std::index_sequence<I...>) noexcept {
  constexpr auto names = std::array<string_view, sizeof...(I)>{{enum_name_v<E, values_v<E, S>[I]>...}};
  return names;
}

template <typename E, enum_subtype S>
inline constexpr auto names_v = names<E, S>(std::make_index_sequence<count_v<E, S>>{});

template <typename E, enum_subtype S, typename D = std::decay_t<E>>
using names_t = decltype((names_v<D, S>));

template <typename E, enum_subtype S, std::size_t... I>
constexpr auto entries(std::index_sequence<I...>) noexcept {
  constexpr auto entries = std::array<std::pair<E, string_view>, sizeof...(I)>{{{values_v<E, S>[I], enum_name_v<E, values_v<E, S>[I]>}...}};
  return entries;
}

template <typename E, enum_subtype S>
inline constexpr auto entries_v = entries<E, S>(std::make_index_sequence<count_v<E, S>>{});

template <typename E, enum_subtype S, typename D = std::decay_t<E>>
using entries_t = decltype((entries_v<D, S>));

template <typename E, enum_subtype S, typename U = std::underlying_type_t<E>>
constexpr bool is_sparse() noexcept {
  if constexpr (count_v<E, S> == 0) {
    return false;
  } else if constexpr (std::is_same_v<U, bool>) { // bool special case
    return false;
  } else {
    constexpr auto max = (S == enum_subtype::flags) ? log2(max_v<E, S>) : max_v<E, S>;
    constexpr auto min = (S == enum_subtype::flags) ? log2(min_v<E, S>) : min_v<E, S>;
    constexpr auto range_size = max - min + 1;

    return range_size != count_v<E, S>;
  }
}

template <typename E, enum_subtype S = subtype_v<E>>
inline constexpr bool is_sparse_v = is_sparse<E, S>();

template <typename E, enum_subtype S>
struct is_reflected
#if defined(MAGIC_ENUM_NO_CHECK_REFLECTED_ENUM)
  : std::true_type {};
#else
  : std::bool_constant<std::is_enum_v<E> && (count_v<E, S> != 0)> {};
#endif

template <typename E, enum_subtype S>
inline constexpr bool is_reflected_v = is_reflected<std::decay_t<E>, S>{};

template <bool, typename R>
struct enable_if_enum {};

template <typename R>
struct enable_if_enum<true, R> {
  using type = R;
  static_assert(supported<R>::value, "magic_enum unsupported compiler (https://github.com/Neargye/magic_enum#compiler-compatibility).");
};

template <typename T, typename R, typename BinaryPredicate = std::equal_to<>, typename D = std::decay_t<T>>
using enable_if_t = typename enable_if_enum<std::is_enum_v<D> && std::is_invocable_r_v<bool, BinaryPredicate, char_type, char_type>, R>::type;

template <typename T, std::enable_if_t<std::is_enum_v<std::decay_t<T>>, int> = 0>
using enum_concept = T;

template <typename T, bool = std::is_enum_v<T>>
struct is_scoped_enum : std::false_type {};

template <typename T>
struct is_scoped_enum<T, true> : std::bool_constant<!std::is_convertible_v<T, std::underlying_type_t<T>>> {};

template <typename T, bool = std::is_enum_v<T>>
struct is_unscoped_enum : std::false_type {};

template <typename T>
struct is_unscoped_enum<T, true> : std::bool_constant<std::is_convertible_v<T, std::underlying_type_t<T>>> {};

template <typename T, bool = std::is_enum_v<std::decay_t<T>>>
struct underlying_type {};

template <typename T>
struct underlying_type<T, true> : std::underlying_type<std::decay_t<T>> {};

#if defined(MAGIC_ENUM_ENABLE_HASH) || defined(MAGIC_ENUM_ENABLE_HASH_SWITCH)

template <typename Value, typename = void>
struct constexpr_hash_t;

template <typename Value>
struct constexpr_hash_t<Value, std::enable_if_t<is_enum_v<Value>>> {
  constexpr auto operator()(Value value) const noexcept {
    using U = typename underlying_type<Value>::type;
    if constexpr (std::is_same_v<U, bool>) { // bool special case
      return static_cast<std::size_t>(value);
    } else {
      return static_cast<U>(value);
    }
  }
  using secondary_hash = constexpr_hash_t;
};

template <typename Value>
struct constexpr_hash_t<Value, std::enable_if_t<std::is_same_v<Value, string_view>>> {
  static constexpr std::uint32_t crc_table[256] {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
    0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
    0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL, 0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
    0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
    0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
    0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
    0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L, 0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
    0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
    0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
    0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
    0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L, 0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
    0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
    0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
    0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
    0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L, 0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
    0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
    0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
    0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
    0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L, 0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
    0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
    0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
    0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
    0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL, 0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
    0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
    0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
    0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
  };
  constexpr std::uint32_t operator()(string_view value) const noexcept {
    auto crc = static_cast<std::uint32_t>(0xffffffffL);
    for (const auto c : value) {
      crc = (crc >> 8) ^ crc_table[(crc ^ static_cast<std::uint32_t>(c)) & 0xff];
    }
    return crc ^ 0xffffffffL;
  }

  struct secondary_hash {
    constexpr std::uint32_t operator()(string_view value) const noexcept {
      auto acc = static_cast<std::uint64_t>(2166136261ULL);
      for (const auto c : value) {
        acc = ((acc ^ static_cast<std::uint64_t>(c)) * static_cast<std::uint64_t>(16777619ULL)) & (std::numeric_limits<std::uint32_t>::max)();
      }
      return static_cast<std::uint32_t>(acc);
    }
  };
};

template <typename Hash>
inline constexpr Hash hash_v{};

template <auto* GlobValues, typename Hash>
constexpr auto calculate_cases(std::size_t Page) noexcept {
  constexpr std::array values = *GlobValues;
  constexpr std::size_t size = values.size();

  using switch_t = std::invoke_result_t<Hash, typename decltype(values)::value_type>;
  static_assert(std::is_integral_v<switch_t> && !std::is_same_v<switch_t, bool>);
  const std::size_t values_to = (std::min)(static_cast<std::size_t>(256), size - Page);

  std::array<switch_t, 256> result{};
  auto fill = result.begin();
  {
    auto first = values.begin() + static_cast<std::ptrdiff_t>(Page);
    auto last = values.begin() + static_cast<std::ptrdiff_t>(Page + values_to);
    while (first != last) {
      *fill++ = hash_v<Hash>(*first++);
    }
  }

  // dead cases, try to avoid case collisions
  for (switch_t last_value = result[values_to - 1]; fill != result.end() && last_value != (std::numeric_limits<switch_t>::max)(); *fill++ = ++last_value) {
  }

  {
    auto it = result.begin();
    auto last_value = (std::numeric_limits<switch_t>::min)();
    for (; fill != result.end(); *fill++ = last_value++) {
      while (last_value == *it) {
        ++last_value, ++it;
      }
    }
  }

  return result;
}

template <typename R, typename F, typename... Args>
constexpr R invoke_r(F&& f, Args&&... args) noexcept(std::is_nothrow_invocable_r_v<R, F, Args...>) {
  if constexpr (std::is_void_v<R>) {
    std::forward<F>(f)(std::forward<Args>(args)...);
  } else {
    return static_cast<R>(std::forward<F>(f)(std::forward<Args>(args)...));
  }
}

enum class case_call_t {
  index,
  value
};

template <typename T = void>
inline constexpr auto default_result_type_lambda = []() noexcept(std::is_nothrow_default_constructible_v<T>) { return T{}; };

template <>
inline constexpr auto default_result_type_lambda<void> = []() noexcept {};

template <auto* Arr, typename Hash>
constexpr bool has_duplicate() noexcept {
  using value_t = std::decay_t<decltype((*Arr)[0])>;
  using hash_value_t = std::invoke_result_t<Hash, value_t>;
  std::array<hash_value_t, Arr->size()> hashes{};
  std::size_t size = 0;
  for (auto elem : *Arr) {
    hashes[size] = hash_v<Hash>(elem);
    for (auto i = size++; i > 0; --i) {
      if (hashes[i] < hashes[i - 1]) {
        auto tmp = hashes[i];
        hashes[i] = hashes[i - 1];
        hashes[i - 1] = tmp;
      } else if (hashes[i] == hashes[i - 1]) {
        return false;
      } else {
        break;
      }
    }
  }
  return true;
}

#define MAGIC_ENUM_CASE(val)                                                                                                  \
  case cases[val]:                                                                                                            \
    if constexpr ((val) + Page < size) {                                                                                      \
      if (!pred(values[val + Page], searched)) {                                                                              \
        break;                                                                                                                \
      }                                                                                                                       \
      if constexpr (CallValue == case_call_t::index) {                                                                        \
        if constexpr (std::is_invocable_r_v<result_t, Lambda, std::integral_constant<std::size_t, val + Page>>) {             \
          return detail::invoke_r<result_t>(std::forward<Lambda>(lambda), std::integral_constant<std::size_t, val + Page>{}); \
        } else if constexpr (std::is_invocable_v<Lambda, std::integral_constant<std::size_t, val + Page>>) {                  \
          MAGIC_ENUM_ASSERT(false && "magic_enum::detail::constexpr_switch wrong result type.");                                         \
        }                                                                                                                     \
      } else if constexpr (CallValue == case_call_t::value) {                                                                 \
        if constexpr (std::is_invocable_r_v<result_t, Lambda, enum_constant<values[val + Page]>>) {                           \
          return detail::invoke_r<result_t>(std::forward<Lambda>(lambda), enum_constant<values[val + Page]>{});               \
        } else if constexpr (std::is_invocable_r_v<result_t, Lambda, enum_constant<values[val + Page]>>) {                    \
          MAGIC_ENUM_ASSERT(false && "magic_enum::detail::constexpr_switch wrong result type.");                                         \
        }                                                                                                                     \
      }                                                                                                                       \
      break;                                                                                                                  \
    } else [[fallthrough]];

template <auto* GlobValues,
          case_call_t CallValue,
          std::size_t Page = 0,
          typename Hash = constexpr_hash_t<typename std::decay_t<decltype(*GlobValues)>::value_type>,
          typename BinaryPredicate = std::equal_to<>,
          typename Lambda,
          typename ResultGetterType>
constexpr decltype(auto) constexpr_switch(
    Lambda&& lambda,
    typename std::decay_t<decltype(*GlobValues)>::value_type searched,
    ResultGetterType&& def,
    BinaryPredicate&& pred = {}) {
  using result_t = std::invoke_result_t<ResultGetterType>;
  using hash_t = std::conditional_t<has_duplicate<GlobValues, Hash>(), Hash, typename Hash::secondary_hash>;
  static_assert(has_duplicate<GlobValues, hash_t>(), "magic_enum::detail::constexpr_switch duplicated hash found, please report it: https://github.com/Neargye/magic_enum/issues.");
  constexpr std::array values = *GlobValues;
  constexpr std::size_t size = values.size();
  constexpr std::array cases = calculate_cases<GlobValues, hash_t>(Page);

  switch (hash_v<hash_t>(searched)) {
    MAGIC_ENUM_FOR_EACH_256(MAGIC_ENUM_CASE)
    default:
      if constexpr (size > 256 + Page) {
        return constexpr_switch<GlobValues, CallValue, Page + 256, Hash>(std::forward<Lambda>(lambda), searched, std::forward<ResultGetterType>(def));
      }
      break;
  }
  return def();
}

#undef MAGIC_ENUM_CASE

#endif

} // namespace magic_enum::detail

// Checks is magic_enum supported compiler.
inline constexpr bool is_magic_enum_supported = detail::supported<void>::value;

template <typename T>
using Enum = detail::enum_concept<T>;

// Checks whether T is an Unscoped enumeration type.
// Provides the member constant value which is equal to true, if T is an [Unscoped enumeration](https://en.cppreference.com/w/cpp/language/enum#Unscoped_enumeration) type. Otherwise, value is equal to false.
template <typename T>
struct is_unscoped_enum : detail::is_unscoped_enum<T> {};

template <typename T>
inline constexpr bool is_unscoped_enum_v = is_unscoped_enum<T>::value;

// Checks whether T is an Scoped enumeration type.
// Provides the member constant value which is equal to true, if T is an [Scoped enumeration](https://en.cppreference.com/w/cpp/language/enum#Scoped_enumerations) type. Otherwise, value is equal to false.
template <typename T>
struct is_scoped_enum : detail::is_scoped_enum<T> {};

template <typename T>
inline constexpr bool is_scoped_enum_v = is_scoped_enum<T>::value;

// If T is a complete enumeration type, provides a member typedef type that names the underlying type of T.
// Otherwise, if T is not an enumeration type, there is no member type. Otherwise (T is an incomplete enumeration type), the program is ill-formed.
template <typename T>
struct underlying_type : detail::underlying_type<T> {};

template <typename T>
using underlying_type_t = typename underlying_type<T>::type;

template <auto V>
using enum_constant = detail::enum_constant<V>;

// Returns type name of enum.
template <typename E>
[[nodiscard]] constexpr auto enum_type_name() noexcept -> detail::enable_if_t<E, string_view> {
  constexpr string_view name = detail::type_name_v<std::decay_t<E>>;
  static_assert(!name.empty(), "magic_enum::enum_type_name enum type does not have a name.");

  return name;
}

// Returns number of enum values.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_count() noexcept -> detail::enable_if_t<E, std::size_t> {
  return detail::count_v<std::decay_t<E>, S>;
}

// Returns enum value at specified index.
// No bounds checking is performed: the behavior is undefined if index >= number of enum values.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_value(std::size_t index) noexcept -> detail::enable_if_t<E, std::decay_t<E>> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  if constexpr (detail::is_sparse_v<D, S>) {
    return MAGIC_ENUM_ASSERT(index < detail::count_v<D, S>), detail::values_v<D, S>[index];
  } else {
    constexpr auto min = (S == detail::enum_subtype::flags) ? detail::log2(detail::min_v<D, S>) : detail::min_v<D, S>;

    return MAGIC_ENUM_ASSERT(index < detail::count_v<D, S>), detail::value<D, min, S>(index);
  }
}

// Returns enum value at specified index.
template <typename E, std::size_t I, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_value() noexcept -> detail::enable_if_t<E, std::decay_t<E>> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");
  static_assert(I < detail::count_v<D, S>, "magic_enum::enum_value out of range.");

  return enum_value<D, S>(I);
}

// Returns std::array with enum values, sorted by enum value.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_values() noexcept -> detail::enable_if_t<E, detail::values_t<E, S>> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  return detail::values_v<D, S>;
}

// Returns integer value from enum value.
template <typename E>
[[nodiscard]] constexpr auto enum_integer(E value) noexcept -> detail::enable_if_t<E, underlying_type_t<E>> {
  return static_cast<underlying_type_t<E>>(value);
}

// Returns underlying value from enum value.
template <typename E>
[[nodiscard]] constexpr auto enum_underlying(E value) noexcept -> detail::enable_if_t<E, underlying_type_t<E>> {
  return static_cast<underlying_type_t<E>>(value);
}

// Obtains index in enum values from enum value.
// Returns optional with index.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_index(E value) noexcept -> detail::enable_if_t<E, optional<std::size_t>> {
  using D = std::decay_t<E>;
  using U = underlying_type_t<D>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  if constexpr (detail::is_sparse_v<D, S> || (S == detail::enum_subtype::flags)) {
#if defined(MAGIC_ENUM_ENABLE_HASH)
    return detail::constexpr_switch<&detail::values_v<D, S>, detail::case_call_t::index>(
        [](std::size_t i) { return optional<std::size_t>{i}; },
        value,
        detail::default_result_type_lambda<optional<std::size_t>>);
#else
    for (std::size_t i = 0; i < detail::count_v<D, S>; ++i) {
      if (enum_value<D, S>(i) == value) {
        return i;
      }
    }
    return {}; // Invalid value or out of range.
#endif
  } else {
    const auto v = static_cast<U>(value);
    if (v >= detail::min_v<D, S> && v <= detail::max_v<D, S>) {
      return static_cast<std::size_t>(v - detail::min_v<D, S>);
    }
    return {}; // Invalid value or out of range.
  }
}

// Obtains index in enum values from enum value.
// Returns optional with index.
template <detail::enum_subtype S, typename E>
[[nodiscard]] constexpr auto enum_index(E value) noexcept -> detail::enable_if_t<E, optional<std::size_t>> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  return enum_index<D, S>(value);
}

// Obtains index in enum values from static storage enum variable.
template <auto V, detail::enum_subtype S = detail::subtype_v<std::decay_t<decltype(V)>>>
[[nodiscard]] constexpr auto enum_index() noexcept -> detail::enable_if_t<decltype(V), std::size_t> {\
  using D = std::decay_t<decltype(V)>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");
  constexpr auto index = enum_index<D, S>(V);
  static_assert(index, "magic_enum::enum_index enum value does not have a index.");

  return *index;
}

// Returns name from static storage enum variable.
// This version is much lighter on the compile times and is not restricted to the enum_range limitation.
template <auto V>
[[nodiscard]] constexpr auto enum_name() noexcept -> detail::enable_if_t<decltype(V), string_view> {
  constexpr string_view name = detail::enum_name_v<std::decay_t<decltype(V)>, V>;
  static_assert(!name.empty(), "magic_enum::enum_name enum value does not have a name.");

  return name;
}

// Returns name from enum value.
// If enum value does not have name or value out of range, returns empty string.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_name(E value) noexcept -> detail::enable_if_t<E, string_view> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  if (const auto i = enum_index<D, S>(value)) {
    return detail::names_v<D, S>[*i];
  }
  return {};
}

// Returns name from enum value.
// If enum value does not have name or value out of range, returns empty string.
template <detail::enum_subtype S, typename E>
[[nodiscard]] constexpr auto enum_name(E value) -> detail::enable_if_t<E, string_view> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  return enum_name<D, S>(value);
}

// Returns std::array with names, sorted by enum value.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_names() noexcept -> detail::enable_if_t<E, detail::names_t<E, S>> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  return detail::names_v<D, S>;
}

// Returns std::array with pairs (value, name), sorted by enum value.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_entries() noexcept -> detail::enable_if_t<E, detail::entries_t<E, S>> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  return detail::entries_v<D, S>;
}

// Allows you to write magic_enum::enum_cast<foo>("bar", magic_enum::case_insensitive);
inline constexpr auto case_insensitive = detail::case_insensitive<>{};

// Obtains enum value from integer value.
// Returns optional with enum value.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_cast(underlying_type_t<E> value) noexcept -> detail::enable_if_t<E, optional<std::decay_t<E>>> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

  if constexpr (detail::is_sparse_v<D, S> || (S == detail::enum_subtype::flags)) {
#if defined(MAGIC_ENUM_ENABLE_HASH)
    return detail::constexpr_switch<&detail::values_v<D, S>, detail::case_call_t::value>(
        [](D v) { return optional<D>{v}; },
        static_cast<D>(value),
        detail::default_result_type_lambda<optional<D>>);
#else
    for (std::size_t i = 0; i < detail::count_v<D, S>; ++i) {
      if (value == static_cast<underlying_type_t<D>>(enum_value<D, S>(i))) {
        return static_cast<D>(value);
      }
    }
    return {}; // Invalid value or out of range.
#endif
  } else {
    if (value >= detail::min_v<D, S> && value <= detail::max_v<D, S>) {
      return static_cast<D>(value);
    }
    return {}; // Invalid value or out of range.
  }
}

// Obtains enum value from name.
// Returns optional with enum value.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>, typename BinaryPredicate = std::equal_to<>>
[[nodiscard]] constexpr auto enum_cast(string_view value, [[maybe_unused]] BinaryPredicate p = {}) noexcept(detail::is_nothrow_invocable_v<BinaryPredicate>) -> detail::enable_if_t<E, optional<std::decay_t<E>>, BinaryPredicate> {
  using D = std::decay_t<E>;
  static_assert(detail::is_reflected_v<D, S>, "magic_enum requires enum implementation and valid max and min.");

#if defined(MAGIC_ENUM_ENABLE_HASH)
  if constexpr (detail::is_default_predicate_v<BinaryPredicate>) {
    return detail::constexpr_switch<&detail::names_v<D, S>, detail::case_call_t::index>(
        [](std::size_t i) { return optional<D>{detail::values_v<D, S>[i]}; },
        value,
        detail::default_result_type_lambda<optional<D>>,
        [&p](string_view lhs, string_view rhs) { return detail::cmp_equal(lhs, rhs, p); });
  }
#endif
  for (std::size_t i = 0; i < detail::count_v<D, S>; ++i) {
    if (detail::cmp_equal(value, detail::names_v<D, S>[i], p)) {
      return enum_value<D, S>(i);
    }
  }
  return {}; // Invalid value or out of range.
}

// Checks whether enum contains value with such value.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_contains(E value) noexcept -> detail::enable_if_t<E, bool> {
  using D = std::decay_t<E>;
  using U = underlying_type_t<D>;

  return static_cast<bool>(enum_cast<D, S>(static_cast<U>(value)));
}

// Checks whether enum contains value with such value.
template <detail::enum_subtype S, typename E>
[[nodiscard]] constexpr auto enum_contains(E value) noexcept -> detail::enable_if_t<E, bool> {
  using D = std::decay_t<E>;
  using U = underlying_type_t<D>;

  return static_cast<bool>(enum_cast<D, S>(static_cast<U>(value)));
}

// Checks whether enum contains value with such integer value.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_contains(underlying_type_t<E> value) noexcept -> detail::enable_if_t<E, bool> {
  using D = std::decay_t<E>;

  return static_cast<bool>(enum_cast<D, S>(value));
}

// Checks whether enum contains enumerator with such name.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>, typename BinaryPredicate = std::equal_to<>>
[[nodiscard]] constexpr auto enum_contains(string_view value, BinaryPredicate p = {}) noexcept(detail::is_nothrow_invocable_v<BinaryPredicate>) -> detail::enable_if_t<E, bool, BinaryPredicate> {
  using D = std::decay_t<E>;

  return static_cast<bool>(enum_cast<D, S>(value, std::move(p)));
}

// Returns true if the enum integer value is in the range of values that can be reflected.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_reflected(underlying_type_t<E> value) noexcept -> detail::enable_if_t<E, bool> {
  using D = std::decay_t<E>;

  if constexpr (detail::is_reflected_v<D, S>) {
    constexpr auto min = detail::reflected_min<E, S>();
    constexpr auto max = detail::reflected_max<E, S>();
    return value >= min && value <= max;
  } else {
    return false;
  }
}

// Returns true if the enum value is in the range of values that can be reflected.
template <typename E, detail::enum_subtype S = detail::subtype_v<E>>
[[nodiscard]] constexpr auto enum_reflected(E value) noexcept -> detail::enable_if_t<E, bool> {
  using D = std::decay_t<E>;

  return enum_reflected<D, S>(static_cast<underlying_type_t<D>>(value));
}

// Returns true if the enum value is in the range of values that can be reflected.
template <detail::enum_subtype S, typename E>
[[nodiscard]] constexpr auto enum_reflected(E value) noexcept -> detail::enable_if_t<E, bool> {
  using D = std::decay_t<E>;

  return enum_reflected<D, S>(value);
}

template <bool AsFlags = true>
inline constexpr auto as_flags = AsFlags ? detail::enum_subtype::flags : detail::enum_subtype::common;

template <bool AsFlags = true>
inline constexpr auto as_common = AsFlags ? detail::enum_subtype::common : detail::enum_subtype::flags;

namespace bitwise_operators {

template <typename E, detail::enable_if_t<E, int> = 0>
constexpr E operator~(E rhs) noexcept {
  return static_cast<E>(~static_cast<underlying_type_t<E>>(rhs));
}

template <typename E, detail::enable_if_t<E, int> = 0>
constexpr E operator|(E lhs, E rhs) noexcept {
  return static_cast<E>(static_cast<underlying_type_t<E>>(lhs) | static_cast<underlying_type_t<E>>(rhs));
}

template <typename E, detail::enable_if_t<E, int> = 0>
constexpr E operator&(E lhs, E rhs) noexcept {
  return static_cast<E>(static_cast<underlying_type_t<E>>(lhs) & static_cast<underlying_type_t<E>>(rhs));
}

template <typename E, detail::enable_if_t<E, int> = 0>
constexpr E operator^(E lhs, E rhs) noexcept {
  return static_cast<E>(static_cast<underlying_type_t<E>>(lhs) ^ static_cast<underlying_type_t<E>>(rhs));
}

template <typename E, detail::enable_if_t<E, int> = 0>
constexpr E& operator|=(E& lhs, E rhs) noexcept {
  return lhs = (lhs | rhs);
}

template <typename E, detail::enable_if_t<E, int> = 0>
constexpr E& operator&=(E& lhs, E rhs) noexcept {
  return lhs = (lhs & rhs);
}

template <typename E, detail::enable_if_t<E, int> = 0>
constexpr E& operator^=(E& lhs, E rhs) noexcept {
  return lhs = (lhs ^ rhs);
}

} // namespace magic_enum::bitwise_operators

} // namespace magic_enum

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif

#undef MAGIC_ENUM_GET_ENUM_NAME_BUILTIN
#undef MAGIC_ENUM_GET_TYPE_NAME_BUILTIN
#undef MAGIC_ENUM_VS_2017_WORKAROUND
#undef MAGIC_ENUM_ARRAY_CONSTEXPR
#undef MAGIC_ENUM_FOR_EACH_256

#endif // NEARGYE_MAGIC_ENUM_HPP

// ============================================================================
// File: extern/visit_struct/visit_struct.hpp
// ============================================================================

//  (C) Copyright 2015 - 2018 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef VISIT_STRUCT_HPP_INCLUDED
#define VISIT_STRUCT_HPP_INCLUDED

/***
 * Provides a facility to declare a structure as "visitable" and apply a visitor
 * to it. The list of members is a compile-time data structure, and there is no
 * run-time overhead.
 */


 // Library version

#define VISIT_STRUCT_VERSION_MAJOR 1
#define VISIT_STRUCT_VERSION_MINOR 2
#define VISIT_STRUCT_VERSION_PATCH 0

#define VISIT_STRUCT_STRING_HELPER(X) #X
#define VISIT_STRUCT_STRING(X) VISIT_STRUCT_STRING_HELPER(X)

#define VISIT_STRUCT_VERSION_STRING VISIT_STRUCT_STRING(VISIT_STRUCT_VERSION_MAJOR) "." VISIT_STRUCT_STRING(VISIT_STRUCT_VERSION_MINOR) "." VISIT_STRUCT_STRING(VISIT_STRUCT_VERSION_PATCH)

// For MSVC 2013 support, we put constexpr behind a define.

# ifndef VISIT_STRUCT_CONSTEXPR
#   if (defined _MSC_VER) && (_MSC_VER <= 1800)
#     define VISIT_STRUCT_CONSTEXPR
#   else
#     define VISIT_STRUCT_CONSTEXPR constexpr
#   endif
# endif

// After C++14 the apply_visitor function can be constexpr.
// We target C++11, but such functions are tagged VISIT_STRUCT_CXX14_CONSTEXPR.

# ifndef VISIT_STRUCT_CXX14_CONSTEXPR
#   if ((defined _MSC_VER) && (_MSC_VER <= 1900)) || (!defined __cplusplus) || (__cplusplus == 201103L)
#     define VISIT_STRUCT_CXX14_CONSTEXPR
#   else
#     define VISIT_STRUCT_CXX14_CONSTEXPR constexpr
#   endif
# endif

// After C++20 we can use __VA_OPT__ and can also visit empty struct
# ifndef VISIT_STRUCT_PP_HAS_VA_OPT
#   if (defined _MSVC_TRADITIONAL && !_MSVC_TRADITIONAL) || (defined __cplusplus && __cplusplus >= 202000L)
#     define VISIT_STRUCT_PP_HAS_VA_OPT true
#   else
#     define VISIT_STRUCT_PP_HAS_VA_OPT false
#   endif
# endif

namespace visit_struct {

    namespace traits {

        // Primary template which is specialized to register a type
        // The context parameter is set when a user wants to register multiple visitation patterns,
        // to include or exclude some field in different contexts.
        template <typename T, typename CONTEXT = void>
        struct visitable;

        // Helper template which checks if a type is registered
        template <typename T, typename CONTEXT = void, typename ENABLE = void>
        struct is_visitable : std::false_type {};

        template <typename T, typename CONTEXT>
        struct is_visitable<T,
            CONTEXT,
            typename std::enable_if<traits::visitable<T, CONTEXT>::value>::type>
            : std::true_type {
        };

        // Helper template which removes cv and reference from a type (saves some typing)
        template <typename T>
        struct clean {
            typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
        };

        template <typename T>
        using clean_t = typename clean<T>::type;

        // Mini-version of std::common_type (we only require C++11)
        template <typename T, typename U>
        struct common_type {
            typedef decltype(true ? std::declval<T>() : std::declval<U>()) type;
        };

    } // end namespace traits

    // Tag for tag dispatch
    template <typename T>
    struct type_c { using type = T; };

    // Accessor type: function object encapsulating a pointer-to-member
    template <typename MemPtr, MemPtr ptr>
    struct accessor {
        template <typename T>
        VISIT_STRUCT_CONSTEXPR auto operator()(T&& t) const -> decltype(std::forward<T>(t).*ptr) {
            return std::forward<T>(t).*ptr;
        }

        static VISIT_STRUCT_CONSTEXPR const auto value = ptr;
    };

    //
    // User-interface
    //

    // Return number of fields in a visitable struct
    template <typename S>
    VISIT_STRUCT_CONSTEXPR std::size_t field_count() {
        return traits::visitable<traits::clean_t<S>>::field_count;
    }

    template <typename S>
    VISIT_STRUCT_CONSTEXPR std::size_t field_count(S&&) { return field_count<S>(); }


    // apply_visitor (one struct instance)
    template <typename S, typename V>
    VISIT_STRUCT_CXX14_CONSTEXPR auto apply_visitor(V&& v, S&& s) ->
        typename std::enable_if<
        traits::is_visitable<traits::clean_t<S>>::value
        >::type {
        traits::visitable<traits::clean_t<S>>::apply(std::forward<V>(v), std::forward<S>(s));
    }

    // apply_visitor (two struct instances)
    template <typename S1, typename S2, typename V>
    VISIT_STRUCT_CXX14_CONSTEXPR auto apply_visitor(V&& v, S1&& s1, S2&& s2) ->
        typename std::enable_if<
        traits::is_visitable<
        traits::clean_t<typename traits::common_type<S1, S2>::type>
        >::value
        >::type {
        using common_S = typename traits::common_type<S1, S2>::type;
        traits::visitable<traits::clean_t<common_S>>::apply(std::forward<V>(v),
            std::forward<S1>(s1),
            std::forward<S2>(s2));
    }

    // for_each (Alternate syntax for apply_visitor, reverses order of arguments)
    template <typename V, typename S>
    VISIT_STRUCT_CXX14_CONSTEXPR auto for_each(S&& s, V&& v) ->
        typename std::enable_if<
        traits::is_visitable<traits::clean_t<S>>::value
        >::type {
        traits::visitable<traits::clean_t<S>>::apply(std::forward<V>(v), std::forward<S>(s));
    }

    // for_each with two structure instances
    template <typename S1, typename S2, typename V>
    VISIT_STRUCT_CXX14_CONSTEXPR auto for_each(S1&& s1, S2&& s2, V&& v) ->
        typename std::enable_if<
        traits::is_visitable<
        traits::clean_t<typename traits::common_type<S1, S2>::type>
        >::value
        >::type {
        using common_S = typename traits::common_type<S1, S2>::type;
        traits::visitable<traits::clean_t<common_S>>::apply(std::forward<V>(v),
            std::forward<S1>(s1),
            std::forward<S2>(s2));
    }

    // Visit the types (visit_struct::type_c<...>) of the registered members
    template <typename S, typename V>
    VISIT_STRUCT_CXX14_CONSTEXPR auto visit_types(V&& v) ->
        typename std::enable_if<
        traits::is_visitable<traits::clean_t<S>>::value
        >::type {
        traits::visitable<traits::clean_t<S>>::visit_types(std::forward<V>(v));
    }

    // Visit the member pointers (&S::a) of the registered members
    template <typename S, typename V>
    VISIT_STRUCT_CXX14_CONSTEXPR auto visit_pointers(V&& v) ->
        typename std::enable_if<
        traits::is_visitable<traits::clean_t<S>>::value
        >::type {
        traits::visitable<traits::clean_t<S>>::visit_pointers(std::forward<V>(v));
    }

    // Visit the accessors (function objects) of the registered members
    template <typename S, typename V>
    VISIT_STRUCT_CXX14_CONSTEXPR auto visit_accessors(V&& v) ->
        typename std::enable_if<
        traits::is_visitable<traits::clean_t<S>>::value
        >::type {
        traits::visitable<traits::clean_t<S>>::visit_accessors(std::forward<V>(v));
    }


    // Apply visitor (with no instances)
    // This calls visit_pointers, for backwards compat reasons
    template <typename S, typename V>
    VISIT_STRUCT_CXX14_CONSTEXPR auto apply_visitor(V&& v) ->
        typename std::enable_if<
        traits::is_visitable<traits::clean_t<S>>::value
        >::type {
        visit_struct::visit_pointers<S>(std::forward<V>(v));
    }


    // Get value by index (like std::get for tuples)
    template <int idx, typename S>
    VISIT_STRUCT_CONSTEXPR auto get(S&& s) ->
        typename std::enable_if <
        traits::is_visitable<traits::clean_t<S>>::value,
        decltype(traits::visitable<traits::clean_t<S>>::get_value(std::integral_constant<int, idx>{}, std::forward<S>(s)))
        > ::type {
        return traits::visitable<traits::clean_t<S>>::get_value(std::integral_constant<int, idx>{}, std::forward<S>(s));
    }

    // Get name of field, by index
    template <int idx, typename S>
    VISIT_STRUCT_CONSTEXPR auto get_name() ->
        typename std::enable_if <
        traits::is_visitable<traits::clean_t<S>>::value,
        decltype(traits::visitable<traits::clean_t<S>>::get_name(std::integral_constant<int, idx>{}))
        > ::type {
        return traits::visitable<traits::clean_t<S>>::get_name(std::integral_constant<int, idx>{});
    }

    template <int idx, typename S>
    VISIT_STRUCT_CONSTEXPR auto get_name(S&&) -> decltype(get_name<idx, S>()) {
        return get_name<idx, S>();
    }

    // Get member pointer, by index
    template <int idx, typename S>
    VISIT_STRUCT_CONSTEXPR auto get_pointer() ->
        typename std::enable_if <
        traits::is_visitable<traits::clean_t<S>>::value,
        decltype(traits::visitable<traits::clean_t<S>>::get_pointer(std::integral_constant<int, idx>{}))
        > ::type {
        return traits::visitable<traits::clean_t<S>>::get_pointer(std::integral_constant<int, idx>{});
    }

    template <int idx, typename S>
    VISIT_STRUCT_CONSTEXPR auto get_pointer(S&&) -> decltype(get_pointer<idx, S>()) {
        return get_pointer<idx, S>();
    }

    // Get member accessor, by index
    template <int idx, typename S>
    VISIT_STRUCT_CONSTEXPR auto get_accessor() ->
        typename std::enable_if <
        traits::is_visitable<traits::clean_t<S>>::value,
        decltype(traits::visitable<traits::clean_t<S>>::get_accessor(std::integral_constant<int, idx>{}))
        > ::type {
        return traits::visitable<traits::clean_t<S>>::get_accessor(std::integral_constant<int, idx>{});
    }

    template <int idx, typename S>
    VISIT_STRUCT_CONSTEXPR auto get_accessor(S&&) -> decltype(get_accessor<idx, S>()) {
        return get_accessor<idx, S>();
    }

    // Get type, by index
    template <int idx, typename S>
    struct type_at_s {
        using type_c = decltype(traits::visitable<traits::clean_t<S>>::type_at(std::integral_constant<int, idx>{}));
        using type = typename type_c::type;
    };

    template <int idx, typename S>
    using type_at = typename type_at_s<idx, S>::type;

    // Get name of structure
    template <typename S>
    VISIT_STRUCT_CONSTEXPR auto get_name() ->
        typename std::enable_if<
        traits::is_visitable<traits::clean_t<S>>::value,
        decltype(traits::visitable<traits::clean_t<S>>::get_name())
        >::type {
        return traits::visitable<traits::clean_t<S>>::get_name();
    }

    template <typename S>
    VISIT_STRUCT_CONSTEXPR auto get_name(S&&) -> decltype(get_name<S>()) {
        return get_name<S>();
    }

    // Alternate visitation patterns can be registered using VISITABLE_STRUCT_IN_CONTEXT.
    // Then, use visit_struct::context<C>::for_each and similar to refer to special contexts.
    template <typename CONTEXT>
    struct context {

        // Return number of fields in a visitable struct
        template <typename S>
        VISIT_STRUCT_CONSTEXPR static std::size_t field_count() {
            return traits::visitable<traits::clean_t<S>, CONTEXT>::field_count;
        }

        template <typename S>
        VISIT_STRUCT_CONSTEXPR static std::size_t field_count(S&&) { return field_count<S>(); }


        // apply_visitor (one struct instance)
        template <typename S, typename V>
        VISIT_STRUCT_CXX14_CONSTEXPR static auto apply_visitor(V&& v, S&& s) ->
            typename std::enable_if<
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value
            >::type {
            traits::visitable<traits::clean_t<S>, CONTEXT>::apply(std::forward<V>(v), std::forward<S>(s));
        }

        // apply_visitor (two struct instances)
        template <typename S1, typename S2, typename V>
        VISIT_STRUCT_CXX14_CONSTEXPR static auto apply_visitor(V&& v, S1&& s1, S2&& s2) ->
            typename std::enable_if<
            traits::is_visitable<
            traits::clean_t<typename traits::common_type<S1, S2>::type>,
            CONTEXT
            >::value
            >::type {
            using common_S = typename traits::common_type<S1, S2>::type;
            traits::visitable<traits::clean_t<common_S>, CONTEXT>::apply(std::forward<V>(v),
                std::forward<S1>(s1),
                std::forward<S2>(s2));
        }

        // for_each (Alternate syntax for apply_visitor, reverses order of arguments)
        template <typename V, typename S>
        VISIT_STRUCT_CXX14_CONSTEXPR static auto for_each(S&& s, V&& v) ->
            typename std::enable_if<
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value
            >::type {
            traits::visitable<traits::clean_t<S>, CONTEXT>::apply(std::forward<V>(v), std::forward<S>(s));
        }

        // for_each with two structure instances
        template <typename S1, typename S2, typename V>
        VISIT_STRUCT_CXX14_CONSTEXPR static auto for_each(S1&& s1, S2&& s2, V&& v) ->
            typename std::enable_if<
            traits::is_visitable<
            traits::clean_t<typename traits::common_type<S1, S2>::type>,
            CONTEXT
            >::value
            >::type {
            using common_S = typename traits::common_type<S1, S2>::type;
            traits::visitable<traits::clean_t<common_S>, CONTEXT>::apply(std::forward<V>(v),
                std::forward<S1>(s1),
                std::forward<S2>(s2));
        }

        // Visit the types (visit_struct::type_c<...>) of the registered members
        template <typename S, typename V>
        VISIT_STRUCT_CXX14_CONSTEXPR static auto visit_types(V&& v) ->
            typename std::enable_if<
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value
            >::type {
            traits::visitable<traits::clean_t<S>, CONTEXT>::visit_types(std::forward<V>(v));
        }

        // Visit the member pointers (&S::a) of the registered members
        template <typename S, typename V>
        VISIT_STRUCT_CXX14_CONSTEXPR static auto visit_pointers(V&& v) ->
            typename std::enable_if<
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value
            >::type {
            traits::visitable<traits::clean_t<S>, CONTEXT>::visit_pointers(std::forward<V>(v));
        }

        // Visit the accessors (function objects) of the registered members
        template <typename S, typename V>
        VISIT_STRUCT_CXX14_CONSTEXPR static auto visit_accessors(V&& v) ->
            typename std::enable_if<
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value
            >::type {
            traits::visitable<traits::clean_t<S>, CONTEXT>::visit_accessors(std::forward<V>(v));
        }


        // Apply visitor (with no instances)
        // This calls visit_pointers, for backwards compat reasons
        template <typename S, typename V>
        VISIT_STRUCT_CXX14_CONSTEXPR static auto apply_visitor(V&& v) ->
            typename std::enable_if<
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value
            >::type {
            visit_struct::visit_pointers<S>(std::forward<V>(v));
        }


        // Get value by index (like std::get for tuples)
        template <int idx, typename S>
        VISIT_STRUCT_CONSTEXPR static auto get(S&& s) ->
            typename std::enable_if <
            traits::is_visitable<traits::clean_t<S>>::value,
            decltype(traits::visitable<traits::clean_t<S>, CONTEXT>::get_value(std::integral_constant<int, idx>{}, std::forward<S>(s)))
            > ::type {
            return traits::visitable<traits::clean_t<S>, CONTEXT>::get_value(std::integral_constant<int, idx>{}, std::forward<S>(s));
        }

        // Get name of field, by index
        template <int idx, typename S>
        VISIT_STRUCT_CONSTEXPR static auto get_name() ->
            typename std::enable_if <
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value,
            decltype(traits::visitable<traits::clean_t<S>, CONTEXT>::get_name(std::integral_constant<int, idx>{}))
            > ::type {
            return traits::visitable<traits::clean_t<S>, CONTEXT>::get_name(std::integral_constant<int, idx>{});
        }

        template <int idx, typename S>
        VISIT_STRUCT_CONSTEXPR static auto get_name(S&&) -> decltype(get_name<idx, S>()) {
            return get_name<idx, S>();
        }

        // Get member pointer, by index
        template <int idx, typename S>
        VISIT_STRUCT_CONSTEXPR static auto get_pointer() ->
            typename std::enable_if <
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value,
            decltype(traits::visitable<traits::clean_t<S>, CONTEXT>::get_pointer(std::integral_constant<int, idx>{}))
            > ::type {
            return traits::visitable<traits::clean_t<S>, CONTEXT>::get_pointer(std::integral_constant<int, idx>{});
        }

        template <int idx, typename S>
        VISIT_STRUCT_CONSTEXPR static auto get_pointer(S&&) -> decltype(get_pointer<idx, S>()) {
            return get_pointer<idx, S>();
        }

        // Get member accessor, by index
        template <int idx, typename S>
        VISIT_STRUCT_CONSTEXPR static auto get_accessor() ->
            typename std::enable_if <
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value,
            decltype(traits::visitable<traits::clean_t<S>, CONTEXT>::get_accessor(std::integral_constant<int, idx>{}))
            > ::type {
            return traits::visitable<traits::clean_t<S>, CONTEXT>::get_accessor(std::integral_constant<int, idx>{});
        }

        template <int idx, typename S>
        VISIT_STRUCT_CONSTEXPR static auto get_accessor(S&&) -> decltype(get_accessor<idx, S>()) {
            return get_accessor<idx, S>();
        }

        // Get type, by index
        template <int idx, typename S>
        struct type_at_s {
            using type_c = decltype(traits::visitable<traits::clean_t<S>, CONTEXT>::type_at(std::integral_constant<int, idx>{}));
            using type = typename type_c::type;
        };

        template <int idx, typename S>
        using type_at = typename type_at_s<idx, S>::type;

        // Get name of structure
        template <typename S>
        VISIT_STRUCT_CONSTEXPR static auto get_name() ->
            typename std::enable_if<
            traits::is_visitable<traits::clean_t<S>, CONTEXT>::value,
            decltype(traits::visitable<traits::clean_t<S>, CONTEXT>::get_name())
            >::type {
            return traits::visitable<traits::clean_t<S>, CONTEXT>::get_name();
        }

        template <typename S>
        VISIT_STRUCT_CONSTEXPR static auto get_name(S&&) -> decltype(get_name<S>()) {
            return get_name<S>();
        }
    };


    /***
     * To implement the VISITABLE_STRUCT macro, we need a map-macro, which can take
     * the name of a macro and some other arguments, and apply that macro to each other argument.
     *
     * There are some techniques you can use within C preprocessor to accomplish this succinctly,
     * by settng up "recursive" macros.
     *
     * But this can also cause it to give worse error messages when something goes wrong.
     *
     * We are now doing it in a more "dumb", bulletproof way which has the advantage that it is
     * more portable and gives better error messages.
     * For discussion see IMPLEMENTATION_NOTES.md
     *
     * The code below is based on a patch from Jarod42, and is now generated by a python script.
     * The purpose of the generated code is to define VISIT_STRUCT_PP_MAP as described.
     */

     /*** Generated code ***/

    static VISIT_STRUCT_CONSTEXPR const int max_visitable_members = 69;

#define VISIT_STRUCT_EXPAND(x) x
#define VISIT_STRUCT_PP_ARG_N( \
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,\
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _20,\
        _21, _22, _23, _24, _25, _26, _27, _28, _29, _30,\
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,\
        _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,\
        _51, _52, _53, _54, _55, _56, _57, _58, _59, _60,\
        _61, _62, _63, _64, _65, _66, _67, _68, _69, N, ...) N

#if VISIT_STRUCT_PP_HAS_VA_OPT
#define VISIT_STRUCT_PP_NARG(...) VISIT_STRUCT_EXPAND(VISIT_STRUCT_PP_ARG_N(0 __VA_OPT__(,) __VA_ARGS__,  \
          69, 68, 67, 66, 65, 64, 63, 62, 61, 60,  \
          59, 58, 57, 56, 55, 54, 53, 52, 51, 50,  \
          49, 48, 47, 46, 45, 44, 43, 42, 41, 40,  \
          39, 38, 37, 36, 35, 34, 33, 32, 31, 30,  \
          29, 28, 27, 26, 25, 24, 23, 22, 21, 20,  \
          19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  \
          9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#else
#define VISIT_STRUCT_PP_NARG(...) VISIT_STRUCT_EXPAND(VISIT_STRUCT_PP_ARG_N(0, __VA_ARGS__,  \
        69, 68, 67, 66, 65, 64, 63, 62, 61, 60,  \
        59, 58, 57, 56, 55, 54, 53, 52, 51, 50,  \
        49, 48, 47, 46, 45, 44, 43, 42, 41, 40,  \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30,  \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20,  \
        19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  \
        9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#endif

    /* need extra level to force extra eval */
#define VISIT_STRUCT_CONCAT_(a,b) a ## b
#define VISIT_STRUCT_CONCAT(a,b) VISIT_STRUCT_CONCAT_(a,b)

#define VISIT_STRUCT_APPLYF0(f)
#define VISIT_STRUCT_APPLYF1(f,_1) f(_1)
#define VISIT_STRUCT_APPLYF2(f,_1,_2) f(_1) f(_2)
#define VISIT_STRUCT_APPLYF3(f,_1,_2,_3) f(_1) f(_2) f(_3)
#define VISIT_STRUCT_APPLYF4(f,_1,_2,_3,_4) f(_1) f(_2) f(_3) f(_4)
#define VISIT_STRUCT_APPLYF5(f,_1,_2,_3,_4,_5) f(_1) f(_2) f(_3) f(_4) f(_5)
#define VISIT_STRUCT_APPLYF6(f,_1,_2,_3,_4,_5,_6) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6)
#define VISIT_STRUCT_APPLYF7(f,_1,_2,_3,_4,_5,_6,_7) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7)
#define VISIT_STRUCT_APPLYF8(f,_1,_2,_3,_4,_5,_6,_7,_8) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8)
#define VISIT_STRUCT_APPLYF9(f,_1,_2,_3,_4,_5,_6,_7,_8,_9) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9)
#define VISIT_STRUCT_APPLYF10(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10)
#define VISIT_STRUCT_APPLYF11(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11)
#define VISIT_STRUCT_APPLYF12(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12)
#define VISIT_STRUCT_APPLYF13(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13)
#define VISIT_STRUCT_APPLYF14(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14)
#define VISIT_STRUCT_APPLYF15(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15)
#define VISIT_STRUCT_APPLYF16(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16)
#define VISIT_STRUCT_APPLYF17(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17)
#define VISIT_STRUCT_APPLYF18(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18)
#define VISIT_STRUCT_APPLYF19(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19)
#define VISIT_STRUCT_APPLYF20(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20)
#define VISIT_STRUCT_APPLYF21(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21)
#define VISIT_STRUCT_APPLYF22(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22)
#define VISIT_STRUCT_APPLYF23(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23)
#define VISIT_STRUCT_APPLYF24(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24)
#define VISIT_STRUCT_APPLYF25(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25)
#define VISIT_STRUCT_APPLYF26(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26)
#define VISIT_STRUCT_APPLYF27(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27)
#define VISIT_STRUCT_APPLYF28(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28)
#define VISIT_STRUCT_APPLYF29(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29)
#define VISIT_STRUCT_APPLYF30(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30)
#define VISIT_STRUCT_APPLYF31(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31)
#define VISIT_STRUCT_APPLYF32(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32)
#define VISIT_STRUCT_APPLYF33(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33)
#define VISIT_STRUCT_APPLYF34(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34)
#define VISIT_STRUCT_APPLYF35(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35)
#define VISIT_STRUCT_APPLYF36(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36)
#define VISIT_STRUCT_APPLYF37(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37)
#define VISIT_STRUCT_APPLYF38(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38)
#define VISIT_STRUCT_APPLYF39(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39)
#define VISIT_STRUCT_APPLYF40(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40)
#define VISIT_STRUCT_APPLYF41(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41)
#define VISIT_STRUCT_APPLYF42(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42)
#define VISIT_STRUCT_APPLYF43(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43)
#define VISIT_STRUCT_APPLYF44(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44)
#define VISIT_STRUCT_APPLYF45(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45)
#define VISIT_STRUCT_APPLYF46(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46)
#define VISIT_STRUCT_APPLYF47(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47)
#define VISIT_STRUCT_APPLYF48(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48)
#define VISIT_STRUCT_APPLYF49(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49)
#define VISIT_STRUCT_APPLYF50(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50)
#define VISIT_STRUCT_APPLYF51(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51)
#define VISIT_STRUCT_APPLYF52(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52)
#define VISIT_STRUCT_APPLYF53(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53)
#define VISIT_STRUCT_APPLYF54(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54)
#define VISIT_STRUCT_APPLYF55(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55)
#define VISIT_STRUCT_APPLYF56(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56)
#define VISIT_STRUCT_APPLYF57(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57)
#define VISIT_STRUCT_APPLYF58(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58)
#define VISIT_STRUCT_APPLYF59(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59)
#define VISIT_STRUCT_APPLYF60(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60)
#define VISIT_STRUCT_APPLYF61(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61)
#define VISIT_STRUCT_APPLYF62(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61) f(_62)
#define VISIT_STRUCT_APPLYF63(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61) f(_62) f(_63)
#define VISIT_STRUCT_APPLYF64(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61) f(_62) f(_63) f(_64)
#define VISIT_STRUCT_APPLYF65(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,_65) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61) f(_62) f(_63) f(_64) f(_65)
#define VISIT_STRUCT_APPLYF66(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,_65,_66) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61) f(_62) f(_63) f(_64) f(_65) f(_66)
#define VISIT_STRUCT_APPLYF67(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,_65,_66,_67) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61) f(_62) f(_63) f(_64) f(_65) f(_66) f(_67)
#define VISIT_STRUCT_APPLYF68(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,_65,_66,_67,_68) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61) f(_62) f(_63) f(_64) f(_65) f(_66) f(_67) f(_68)
#define VISIT_STRUCT_APPLYF69(f,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,_65,_66,_67,_68,_69) f(_1) f(_2) f(_3) f(_4) f(_5) f(_6) f(_7) f(_8) f(_9) f(_10) f(_11) f(_12) f(_13) f(_14) f(_15) f(_16) f(_17) f(_18) f(_19) f(_20) f(_21) f(_22) f(_23) f(_24) f(_25) f(_26) f(_27) f(_28) f(_29) f(_30) f(_31) f(_32) f(_33) f(_34) f(_35) f(_36) f(_37) f(_38) f(_39) f(_40) f(_41) f(_42) f(_43) f(_44) f(_45) f(_46) f(_47) f(_48) f(_49) f(_50) f(_51) f(_52) f(_53) f(_54) f(_55) f(_56) f(_57) f(_58) f(_59) f(_60) f(_61) f(_62) f(_63) f(_64) f(_65) f(_66) f(_67) f(_68) f(_69)

#define VISIT_STRUCT_APPLY_F_(M, ...) VISIT_STRUCT_EXPAND(M(__VA_ARGS__))
#if VISIT_STRUCT_PP_HAS_VA_OPT
#define VISIT_STRUCT_PP_MAP(f, ...) VISIT_STRUCT_EXPAND(VISIT_STRUCT_APPLY_F_(VISIT_STRUCT_CONCAT(VISIT_STRUCT_APPLYF, VISIT_STRUCT_PP_NARG(__VA_ARGS__)), f __VA_OPT__(,) __VA_ARGS__))
#else
#define VISIT_STRUCT_PP_MAP(f, ...) VISIT_STRUCT_EXPAND(VISIT_STRUCT_APPLY_F_(VISIT_STRUCT_CONCAT(VISIT_STRUCT_APPLYF, VISIT_STRUCT_PP_NARG(__VA_ARGS__)), f, __VA_ARGS__))
#endif

/*** End generated code ***/

/***
 * These macros are used with VISIT_STRUCT_PP_MAP
 */

#define VISIT_STRUCT_FIELD_COUNT(MEMBER_NAME)                                                      \
  + 1

#define VISIT_STRUCT_MEMBER_HELPER(MEMBER_NAME)                                                    \
  std::forward<V>(visitor)(#MEMBER_NAME, std::forward<S>(struct_instance).MEMBER_NAME);

#define VISIT_STRUCT_MEMBER_HELPER_PTR(MEMBER_NAME)                                                \
  std::forward<V>(visitor)(#MEMBER_NAME, &this_type::MEMBER_NAME);

#define VISIT_STRUCT_MEMBER_HELPER_TYPE(MEMBER_NAME)                                               \
  std::forward<V>(visitor)(#MEMBER_NAME, visit_struct::type_c<decltype(this_type::MEMBER_NAME)>{});

#define VISIT_STRUCT_MEMBER_HELPER_ACC(MEMBER_NAME)                                                \
  std::forward<V>(visitor)(#MEMBER_NAME, visit_struct::accessor<decltype(&this_type::MEMBER_NAME), &this_type::MEMBER_NAME>{});


#define VISIT_STRUCT_MEMBER_HELPER_PAIR(MEMBER_NAME)                                               \
  std::forward<V>(visitor)(#MEMBER_NAME, std::forward<S1>(s1).MEMBER_NAME, std::forward<S2>(s2).MEMBER_NAME);

#define VISIT_STRUCT_MAKE_GETTERS(MEMBER_NAME)                                                     \
  template <typename S>                                                                            \
  static VISIT_STRUCT_CONSTEXPR auto                                                               \
    get_value(std::integral_constant<int, fields_enum::MEMBER_NAME>, S && s) ->                    \
    decltype((std::forward<S>(s).MEMBER_NAME)) {                                                   \
    return std::forward<S>(s).MEMBER_NAME;                                                         \
  }                                                                                                \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR auto                                                               \
    get_name(std::integral_constant<int, fields_enum::MEMBER_NAME>) ->                             \
      decltype(#MEMBER_NAME) {                                                                     \
    return #MEMBER_NAME;                                                                           \
  }                                                                                                \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR auto                                                               \
    get_pointer(std::integral_constant<int, fields_enum::MEMBER_NAME>) ->                          \
      decltype(&this_type::MEMBER_NAME) {                                                          \
    return &this_type::MEMBER_NAME;                                                                \
  }                                                                                                \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR auto                                                               \
    get_accessor(std::integral_constant<int, fields_enum::MEMBER_NAME>) ->                         \
      visit_struct::accessor<decltype(&this_type::MEMBER_NAME), &this_type::MEMBER_NAME > {        \
    return {};                                                                                     \
  }                                                                                                \
                                                                                                   \
  static auto                                                                                      \
    type_at(std::integral_constant<int, fields_enum::MEMBER_NAME>) ->                              \
      visit_struct::type_c<decltype(this_type::MEMBER_NAME)>;


 // This macro specializes the trait, provides "apply" method which does the work.
 // Below, template parameter S should always be the same as STRUCT_NAME modulo const and reference.
 // The interface defined above ensures that STRUCT_NAME is clean_t<S> basically.
 //
 // Note: The code to make the indexed getters work is more convoluted than I'd like.
 //       PP_MAP doesn't give you the index of each member. And rather than hack it so that it will
 //       do that, what we do instead is:
 //       1: Declare an enum `field_enum` in the scope of visitable, which maps names to indices.
 //          This gives an easy way for the macro to get the index from the name token.
 //       2: Intuitively we'd like to use template partial specialization to make indices map to
 //          values, and have a new specialization for each member. But, specializations can only
 //          be made at namespace scope. So to keep things tidy and contained within this trait,
 //          we use tag dispatch with std::integral_constant<int> instead.

#define VISITABLE_STRUCT(STRUCT_NAME, ...)                                                         \
namespace visit_struct {                                                                           \
namespace traits {                                                                                 \
                                                                                                   \
template <>                                                                                        \
struct visitable<STRUCT_NAME, void> {                                                              \
                                                                                                   \
  using this_type = STRUCT_NAME;                                                                   \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR auto get_name()                                                    \
    -> decltype(#STRUCT_NAME) {                                                                    \
    return #STRUCT_NAME;                                                                           \
  }                                                                                                \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR const std::size_t field_count = 0                                  \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_FIELD_COUNT, __VA_ARGS__);                                    \
                                                                                                   \
  template <typename V, typename S>                                                                \
  VISIT_STRUCT_CXX14_CONSTEXPR static void apply(V && visitor, S && struct_instance)               \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER, __VA_ARGS__)                                   \
  }                                                                                                \
                                                                                                   \
  template <typename V, typename S1, typename S2>                                                  \
  VISIT_STRUCT_CXX14_CONSTEXPR static void apply(V && visitor, S1 && s1, S2 && s2)                 \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_PAIR, __VA_ARGS__)                              \
  }                                                                                                \
                                                                                                   \
  template <typename V>                                                                            \
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_pointers(V && visitor)                            \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_PTR, __VA_ARGS__)                               \
  }                                                                                                \
                                                                                                   \
  template <typename V>                                                                            \
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_types(V && visitor)                               \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_TYPE, __VA_ARGS__)                              \
  }                                                                                                \
                                                                                                   \
  template <typename V>                                                                            \
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_accessors(V && visitor)                           \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_ACC, __VA_ARGS__)                               \
  }                                                                                                \
                                                                                                   \
  struct fields_enum {                                                                             \
    enum index { __VA_ARGS__ };                                                                    \
  };                                                                                               \
                                                                                                   \
  VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MAKE_GETTERS, __VA_ARGS__)                                      \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR const bool value = true;                                           \
};                                                                                                 \
                                                                                                   \
}                                                                                                  \
}                                                                                                  \
static_assert(true, "")

#define VISITABLE_STRUCT_IN_CONTEXT(CONTEXT, STRUCT_NAME, ...)                                     \
namespace visit_struct {                                                                           \
namespace traits {                                                                                 \
                                                                                                   \
template <>                                                                                        \
struct visitable<STRUCT_NAME, CONTEXT> {                                                           \
                                                                                                   \
  using this_type = STRUCT_NAME;                                                                   \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR auto get_name()                                                    \
    -> decltype(#STRUCT_NAME) {                                                                    \
    return #STRUCT_NAME;                                                                           \
  }                                                                                                \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR const std::size_t field_count = 0                                  \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_FIELD_COUNT, __VA_ARGS__);                                    \
                                                                                                   \
  template <typename V, typename S>                                                                \
  VISIT_STRUCT_CXX14_CONSTEXPR static void apply(V && visitor, S && struct_instance)               \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER, __VA_ARGS__)                                   \
  }                                                                                                \
                                                                                                   \
  template <typename V, typename S1, typename S2>                                                  \
  VISIT_STRUCT_CXX14_CONSTEXPR static void apply(V && visitor, S1 && s1, S2 && s2)                 \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_PAIR, __VA_ARGS__)                              \
  }                                                                                                \
                                                                                                   \
  template <typename V>                                                                            \
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_pointers(V && visitor)                            \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_PTR, __VA_ARGS__)                               \
  }                                                                                                \
                                                                                                   \
  template <typename V>                                                                            \
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_types(V && visitor)                               \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_TYPE, __VA_ARGS__)                              \
  }                                                                                                \
                                                                                                   \
  template <typename V>                                                                            \
  VISIT_STRUCT_CXX14_CONSTEXPR static void visit_accessors(V && visitor)                           \
  {                                                                                                \
    VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MEMBER_HELPER_ACC, __VA_ARGS__)                               \
  }                                                                                                \
                                                                                                   \
  struct fields_enum {                                                                             \
    enum index { __VA_ARGS__ };                                                                    \
  };                                                                                               \
                                                                                                   \
  VISIT_STRUCT_PP_MAP(VISIT_STRUCT_MAKE_GETTERS, __VA_ARGS__)                                      \
                                                                                                   \
  static VISIT_STRUCT_CONSTEXPR const bool value = true;                                           \
};                                                                                                 \
                                                                                                   \
}                                                                                                  \
}                                                                                                  \
static_assert(true, "")

} // end namespace visit_struct

#endif // VISIT_STRUCT_HPP_INCLUDED

// ============================================================================
// File: ImReflect_entry.hpp
// ============================================================================

#include <imgui.h>


// ============================================================================
// File: ImReflect_macro.hpp
// ============================================================================


/*
Define this underneath your class or struct with the appropriate member variables
Example:
struct MyStruct {
	int a;
	float b;
	std::string c;
};
IMGUI_REFLECT(MyStruct, a, b, c)
*/

#define IMGUI_REFLECT(T, ...) \
VISITABLE_STRUCT_IN_CONTEXT(ImReflect::Detail::ImContext, T, __VA_ARGS__);

struct ImReflect_global_tag {};

namespace ImReflect {
    namespace Detail {
        struct ImContext {};
    }

    struct ImInput_t : ImReflect_global_tag {};
    inline constexpr ImInput_t input{};

    /* forward declare */
    template<class T, class ENABLE = void>
    struct type_settings;

    template<class T>
    struct type_response;
}

/* forward declare */
namespace svh {
	template<template<class> class BaseTemplate>
	struct scope;
}

/* forward declare */
using ImSettings = svh::scope<ImReflect::type_settings>;
using ImResponse = svh::scope<ImReflect::type_response>;

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
	template<typename T>
	struct ImRequired : svh::scope<type_settings>, ImReflect::Detail::required<T> {};

	using ImSettings = svh::scope<type_settings>;

	template<class T, class ENABLE>
	struct type_settings : ImRequired<T> {};

	/* RESPONSES */
	template<class T>
	struct type_response; /* Forward declare */

	using ImResponse = svh::scope<type_response>;

	template<class T>
	struct type_response : /* ImResponse is inherited in required */ ImReflect::Detail::required_response<T> {};

	namespace Detail {

		struct ImInputLib_t : ImReflect_global_tag { /* Library only tag */ };
		inline constexpr ImInputLib_t input_lib{};

		template <typename T>
		inline constexpr bool has_imreflect_input_v =
			svh::is_tag_invocable_v<ImInput_t, const char*, T&, ImSettings&, ImResponse&> ||
			svh::is_tag_invocable_v<ImInputLib_t, const char*, T&, ImSettings&, ImResponse&> ||
			visit_struct::traits::is_visitable<std::remove_cv_t<T>, ImContext>::value;

		/* Forward declare */
		template<typename T>
		void InputImpl(const char* label, T& value, ImSettings& settings, ImResponse& response);

		template<typename T>
		void imgui_input_visit_field(const char* label, T& value, ImSettings& settings, ImResponse& response) {
			constexpr bool is_const = std::is_const_v<T>;
			ImGui::PushID(label);
			const bool empty = std::string(label).empty();
			if (!empty) ImGui::SeparatorText(label);
			if constexpr (is_const) {
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
					ImGui::SetTooltip("Const object");
				}
			}
			if (!empty) ImGui::Indent();
			visit_struct::context<ImContext>::for_each(value,
				[&](const char* name, auto& field) {
					auto& member_settings = settings.get_member(value, field);
					auto& member_response = response.get_member(value, field);

					std::string label = member_settings.has_label() ? member_settings.get_label() : name;

					ImGui::PushID(name);
					InputImpl(label.c_str(), field, member_settings, member_response); // recurse
					ImGui::PopID();
				});
			if (!empty) ImGui::Unindent();
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

			const bool same_line = type_settings.on_same_line();
			const bool separator = type_settings.has_separator();
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
			/* If type is reflected */
			else if constexpr (visit_struct::traits::is_visitable<std::remove_cv_t<T>, ImContext>::value) {
				imgui_input_visit_field(label, value, type_settings, type_response);
			}
			/* Try tag_invoke with default library implementations */
			else if constexpr (svh::is_tag_invocable_v<ImInputLib_t, const char*, T&, ImSettings&, ImResponse&>) {
				tag_invoke(input_lib, label, value, type_settings, type_response);
			} else {
				//TODO: add link to documentation
				static_assert(svh::always_false<T>::value, "ImReflect Error: No suitable Input implementation found for type T");
			}

			if (disabled) {
				ImGui::EndDisabled();
			}
			if (same_line) {
				ImGui::SameLine();
			}
			if (separator) {
				ImGui::Separator();
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

template<typename T>
using ImRequired = ImReflect::ImRequired<T>;
using ImSettings = ImReflect::ImSettings;
using ImResponse = ImReflect::ImResponse;

// ============================================================================
// File: ImReflect_helper.hpp
// ============================================================================

#include <imgui_internal.h>

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

// ============================================================================
// File: ImReflect_primitives.hpp
// ============================================================================


#include <string>

/* Helpers */
namespace ImReflect::Detail {

	template<typename T> /* Only bool */
	constexpr bool is_bool_v = std::is_same_v<std::remove_cv_t<T>, bool>;
	template<typename T>
	using enable_if_bool_t = std::enable_if_t<is_bool_v<T>, void>;

	template<typename T> /* All numbers except bool */
	constexpr bool is_numeric_v = (std::is_integral_v<T> || std::is_floating_point_v<T>) && !is_bool_v<T>;
	template<typename T>
	using enable_if_numeric_t = std::enable_if_t<is_numeric_v<std::remove_cv_t<T>>, void>;

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
namespace ImReflect::Detail {

	/* Generic min/max settings for numeric types */
	template<typename T>
	struct min_max {
	private:
		T _min = default_min();
		T _max = default_max();
		bool _clamp = false;
	public:

		type_settings<T>& min(const T v) { _min = v; RETURN_THIS; }
		type_settings<T>& max(const T v) { _max = v; RETURN_THIS; }
		/* Enforce clamping after input */
		type_settings<T>& clamp(const bool v = true) { _clamp = v; RETURN_THIS; }

		const T& get_min() const { return _min; }
		const T& get_max() const { return _max; }
		bool is_clamped() const { return _clamp; }
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

	/* Generic step settings for ``ImGui::InputScalar`` */
	template<typename T>
	struct input_step {
	private:
		T _step = T(1);
		T _step_fast = T(10);
	public:

		type_settings<T>& step(const T v) { _step = v; RETURN_THIS; }
		type_settings<T>& step_fast(const T v) { _step_fast = v; RETURN_THIS; }

		const T& get_step() const { return _step; }
		const T& get_step_fast() const { return _step_fast; }
	};

	/* Generic speed settings for sliders and draggers */
	template<typename T>
	struct drag_speed {
	private:
		float _speed = 1.0f;
	public:
		/* Optional default speed */
		drag_speed(const float v = 1.0f) : _speed(v) {}
		type_settings<T>& speed(const float v) { _speed = v; RETURN_THIS; }
		float get_speed() const { return _speed; }
	};

	/* format settings for input widgets */
	template<typename T>
	struct format_settings {
	private:
		std::string get_default_format() const {
			constexpr auto data_type = imgui_data_type_trait<T>::value;
			const ImGuiDataTypeInfo* data_type_info = ImGui::DataTypeGetInfo(data_type);
			if (data_type_info && data_type_info->PrintFmt) {
				return std::string(data_type_info->PrintFmt);
			}

			// Fallbacks based on type
			if constexpr (std::is_integral_v<T>) {
				if constexpr (std::is_signed_v<T>) return "%d";
				else return "%u";
			} else if constexpr (std::is_floating_point_v<T>) {
				return "%.3f";
			} else {

				return "%d"; // Final fallback
			}
		}

		std::string _prefix = "";
		std::string _format = "";
		std::string _suffix = "";

	public:
		/*
		* NOTE: Prefix does not work if using a input widget. Only works with sliders and draggers.
		* See https://github.com/ocornut/imgui/issues/6829
		*/
		type_settings<T>& prefix(const std::string& val) { _prefix = val; RETURN_THIS; }
		type_settings<T>& format(const std::string& val) { _format = val;  RETURN_THIS; }
		type_settings<T>& suffix(const std::string& val) { _suffix = val; RETURN_THIS; }

		// Integer formats
		type_settings<T>& as_decimal() { _format = "%d";  RETURN_THIS; }
		type_settings<T>& as_unsigned() { _format = "%u";  RETURN_THIS; }
		type_settings<T>& as_hex(bool uppercase = false) { _format = uppercase ? "%X" : "%x"; RETURN_THIS; }
		type_settings<T>& as_octal() { _format = "%o";  RETURN_THIS; }

		// Integer with width/padding
		type_settings<T>& as_int_padded(int width, char pad_char = '0') { _format = "%" + std::string(1, pad_char) + std::to_string(width) + "d"; RETURN_THIS; }

		// Floating point formats
		type_settings<T>& as_float(int precision = 3) { _format = "%." + std::to_string(precision) + "f"; RETURN_THIS; }
		type_settings<T>& as_double(int precision = 6) { _format = "%." + std::to_string(precision) + "lf"; RETURN_THIS; }
		type_settings<T>& as_scientific(int precision = 2, bool uppercase = false) { _format = "%." + std::to_string(precision) + (uppercase ? "E" : "e"); RETURN_THIS; }
		type_settings<T>& as_general(int precision = 6, bool uppercase = false) { _format = "%." + std::to_string(precision) + (uppercase ? "G" : "g"); RETURN_THIS; }

		// Width and alignment
		type_settings<T>& width(int w) {
			if (_format.empty()) _format = get_base_format();
			_format = insert_width(_format, w, false);
			RETURN_THIS;
		}

		type_settings<T>& width_left_aligned(int w) {
			if (_format.empty()) _format = get_base_format();
			_format = insert_width(_format, w, true);
			RETURN_THIS;
		}

		// Sign options
		type_settings<T>& always_show_sign() {
			if (_format.empty()) _format = get_base_format();
			_format = insert_flag(_format, '+');
			RETURN_THIS;
		}

		type_settings<T>& space_for_positive() {
			if (_format.empty()) _format = get_base_format();
			_format = insert_flag(_format, ' ');
			RETURN_THIS;
		}

		// Padding options
		type_settings<T>& zero_pad(int width) { _format = "%0" + std::to_string(width) + get_type_specifier(); RETURN_THIS; }

		// Character and percentage
		type_settings<T>& as_char() { _format = "%c";  RETURN_THIS; }
		type_settings<T>& as_percentage(int precision = 1) { _format = "%." + std::to_string(precision) + "f%%"; RETURN_THIS; }

		// Utility methods
		type_settings<T>& clear_format() { _format = ""; RETURN_THIS; }
		type_settings<T>& reset() {
			_prefix.clear();
			_format.clear();
			_suffix.clear();
			RETURN_THIS;
		}

		// Get final format string
		std::string get_format() const {
			std::string core_format = _format.empty() ? get_default_format() : _format;
			if (core_format.empty()) core_format = get_default_format();
			return _prefix + core_format + _suffix;
		}

	private:
		std::string get_base_format() const {
			if constexpr (std::is_integral_v<T>) {
				return std::is_signed_v<T> ? "%d" : "%u";
			} else if constexpr (std::is_floating_point_v<T>) {
				return "%.3f";
			}
			return "%d";
		}

		std::string get_type_specifier() const {
			if constexpr (std::is_integral_v<T>) {
				return std::is_signed_v<T> ? "d" : "u";
			} else if constexpr (std::is_floating_point_v<T>) {
				return "f";
			}
			return "d";
		}

		std::string insert_width(const std::string& fmt, int width, bool left_align) const {
			size_t percent_pos = fmt.find('%');
			if (percent_pos == std::string::npos) return fmt;

			std::string result = fmt;
			std::string width_str = (left_align ? "-" : "") + std::to_string(width);
			result.insert(percent_pos + 1, width_str);
			return result;
		}

		std::string insert_flag(const std::string& fmt, char flag) const {
			size_t percent_pos = fmt.find('%');
			if (percent_pos == std::string::npos) return fmt;

			std::string result = fmt;
			result.insert(percent_pos + 1, 1, flag);
			return result;
		}
	};

	template<typename T>
	struct true_false_text {
	private:
		std::string _true_text = "True";
		std::string _false_text = "False";

	public:
		type_settings<T>& true_text(const std::string& text) { _true_text = text; RETURN_THIS; }
		type_settings<T>& false_text(const std::string& text) { _false_text = text; RETURN_THIS; }
		const std::string& get_true_text() const { return _true_text; }
		const std::string& get_false_text() const { return _false_text; };
	};

	/* Slider flag settings */
	template<typename T>
	struct slider_flags {
	private:
		ImGuiSliderFlags _flags = ImGuiSliderFlags_None;

		inline void set_flag(const ImGuiSliderFlags flag, const bool enabled) {
			if (enabled) _flags = static_cast<ImGuiSliderFlags>(_flags | flag);
			else _flags = static_cast<ImGuiSliderFlags>(_flags & ~flag);
		}
	public:
		type_settings<T>& logarithmic(const bool v = true) { set_flag(ImGuiSliderFlags_Logarithmic, v); RETURN_THIS; }
		type_settings<T>& no_round_to_format(const bool v = true) { set_flag(ImGuiSliderFlags_NoRoundToFormat, v); RETURN_THIS; }
		type_settings<T>& no_input(const bool v = true) { set_flag(ImGuiSliderFlags_NoInput, v); RETURN_THIS; }
		type_settings<T>& wrap_around(const bool v = true) { set_flag(ImGuiSliderFlags_WrapAround, v); RETURN_THIS; }
		type_settings<T>& clamp_on_input(const bool v = true) { set_flag(ImGuiSliderFlags_ClampOnInput, v); RETURN_THIS; }
		type_settings<T>& clamp_zero_range(const bool v = true) { set_flag(ImGuiSliderFlags_ClampZeroRange, v); RETURN_THIS; }
		type_settings<T>& no_speed_tweaks(const bool v = true) { set_flag(ImGuiSliderFlags_NoSpeedTweaks, v); RETURN_THIS; }
		type_settings<T>& always_clamp(const bool v = true) { set_flag(ImGuiSliderFlags_AlwaysClamp, v); RETURN_THIS; }

		const ImGuiSliderFlags& get_slider_flags() const { return _flags; }
	};

	enum class input_type_widget {
		Input,
		Drag,
		Slider,
		Radio,
		Checkbox,
		Dropdown,
		Button
	};

	struct input_type {
		input_type(const input_type_widget type) : _type(type) {}
		input_type_widget _type;

		const input_type_widget& get_input_type() const { return _type; }
	};

	template<typename T>
	struct input_widget : virtual input_type, input_step<T> {
		input_widget() : input_type(input_type_widget::Input) {}
		type_settings<T>& as_input() { this->_type = input_type_widget::Input; RETURN_THIS; }
		bool is_input() const { return this->_type == input_type_widget::Input; }
	};

	template<typename T>
	struct drag_widget : virtual input_type {
		drag_widget() : input_type(input_type_widget::Drag) {}
		type_settings<T>& as_drag() { this->_type = input_type_widget::Drag; RETURN_THIS; }
		bool is_drag() const { return this->_type == input_type_widget::Drag; }
	};

	template<typename T>
	struct slider_widget : virtual input_type {
		slider_widget() : input_type(input_type_widget::Slider) {}
		type_settings<T>& as_slider() { this->_type = input_type_widget::Slider; RETURN_THIS; }
		bool is_slider() const { return this->_type == input_type_widget::Slider; }
	};

	template<typename T>
	struct radio_widget : virtual input_type {
		radio_widget() : input_type(input_type_widget::Radio) {}
		type_settings<T>& as_radio() { this->_type = input_type_widget::Radio; RETURN_THIS; }
		bool is_radio() const { return this->_type == input_type_widget::Radio; }
	};

	template<typename T>
	struct checkbox_widget : virtual input_type {
		checkbox_widget() : input_type(input_type_widget::Checkbox) {}
		type_settings<T>& as_checkbox() { this->_type = input_type_widget::Checkbox; RETURN_THIS; }
		bool is_checkbox() const { return this->_type == input_type_widget::Checkbox; }
	};

	template<typename T>
	struct dropdown_widget : virtual input_type {
		dropdown_widget() : input_type(input_type_widget::Dropdown) {}
		type_settings<T>& as_dropdown() { this->_type = input_type_widget::Dropdown; RETURN_THIS; }
		bool is_dropdown() const { return this->_type == input_type_widget::Dropdown; }
	};

	template<typename T>
	struct button_widget : virtual input_type {
		button_widget() : input_type(input_type_widget::Button) {}
		type_settings<T>& as_button() { this->_type = input_type_widget::Button; RETURN_THIS; }
		bool is_button() const { return this->_type == input_type_widget::Button; }
	};
}

/* Input fields for primitive types */
namespace ImReflect {

	/* ========================= all integral types except bool ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_numeric_t<T>> : ImRequired<T>,
		/* Need to specify ``ImReflect`` before Detail::min_max otherwise intellisense wont work */
		ImReflect::Detail::min_max<T>,
		ImReflect::Detail::drag_speed<T>,
		ImReflect::Detail::input_widget<T>,
		ImReflect::Detail::drag_widget<T>,
		ImReflect::Detail::slider_widget<T>,
		ImReflect::Detail::input_flags<T>,
		ImReflect::Detail::format_settings<T>,
		ImReflect::Detail::slider_flags<T> {
		/* Default to input widget */
		type_settings() : ImReflect::Detail::input_type(ImReflect::Detail::input_type_widget::Input) {}
	};

	template<typename T>
	std::enable_if_t<ImReflect::Detail::is_numeric_v<T>, void>
		tag_invoke(Detail::ImInputLib_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
		auto& num_settings = settings.get<T>();
		auto& num_response = response.get<T>();

		const auto& min = num_settings.get_min();
		const auto& max = num_settings.get_max();
		const auto& fmt = num_settings.get_format();
		const auto& speed = num_settings.get_speed();
		constexpr auto data_type = Detail::imgui_data_type_trait<T>::value;

		bool changed = false;
		if constexpr (std::is_const_v<T> == false) {

			if (num_settings.is_slider()) {
				const auto id = Detail::scope_id("slider");
				changed = ImGui::SliderScalar(label, data_type, &value, &min, &max, fmt.c_str(), num_settings.get_slider_flags());
			} else if (num_settings.is_drag()) {
				const auto id = Detail::scope_id("drag");
				changed = ImGui::DragScalar(label, data_type, &value, speed, &min, &max, fmt.c_str(), num_settings.get_slider_flags());
			} else if (num_settings.is_input()) {
				const auto id = Detail::scope_id("input");
				const auto& step = num_settings.get_step();
				const auto& step_fast = num_settings.get_step_fast();
				changed = ImGui::InputScalar(label, data_type, &value, &step, &step_fast, fmt.c_str(), num_settings.get_input_flags());
			} else {
				throw std::runtime_error("Unknown input type");
			}

			/* Clamp if needed */
			if (num_settings.is_clamped()) {
				if (value < min) value = min;
				if (value > max) value = max;
			}
		} else {
			/* Const value, just display */
			ImReflect::Detail::text_label(label);
			ImReflect::Detail::imgui_tooltip("Value is const");

			ImGui::BeginDisabled();
			ImGui::SameLine();
			ImGui::Text(fmt.c_str(), value);
			ImGui::EndDisabled();
		}

		if (changed) {
			num_response.changed();
		}
		ImReflect::Detail::check_input_states(num_response);
	}

	/* ========================= bool ========================= */
	template<typename T>
	struct type_settings<T, Detail::enable_if_bool_t<T>> : ImRequired<T>,
		ImReflect::Detail::checkbox_widget<T>,
		ImReflect::Detail::radio_widget<T>,
		ImReflect::Detail::button_widget<T>,
		ImReflect::Detail::dropdown_widget<T>,
		ImReflect::Detail::true_false_text<T> {
		/* Default to checkbox */
		type_settings() : ImReflect::Detail::input_type(ImReflect::Detail::input_type_widget::Checkbox) {}
	};

	template<typename T>
	std::enable_if_t<ImReflect::Detail::is_bool_v<T>, void>
		tag_invoke(Detail::ImInputLib_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
		auto& bool_settings = settings.get<T>();
		auto& bool_response = response.get<T>();

		bool changed = false;

		if constexpr (std::is_const_v<T> == false) {

			if (bool_settings.is_checkbox()) {
				const auto id = Detail::scope_id("checkbox");

				changed = ImGui::Checkbox(label, &value);
			} else if (bool_settings.is_radio()) {
				const auto id = Detail::scope_id("radio");

				int int_value = value ? 1 : 0;
				changed = ImGui::RadioButton(bool_settings.get_true_text().c_str(), &int_value, 1);
				ImGui::SameLine();
				changed |= ImGui::RadioButton(bool_settings.get_false_text().c_str(), &int_value, 0);
				value = (int_value != 0);

				ImGui::SameLine();
				ImReflect::Detail::text_label(label);
			} else if (bool_settings.is_dropdown()) {
				const auto id = Detail::scope_id("dropdown");

				const char* items[] = { bool_settings.get_false_text().c_str(), bool_settings.get_true_text().c_str() };
				int int_value = value ? 1 : 0;
				changed = ImGui::Combo(label, &int_value, items, IM_ARRAYSIZE(items));

				value = (int_value != 0);
			} else if (bool_settings.is_button()) {
				const auto id = Detail::scope_id("button");

				const auto& button_label = value ? bool_settings.get_true_text() : bool_settings.get_false_text();
				if (ImGui::Button(button_label.c_str())) {
					value = !value;
					changed = true;
				}

				ImGui::SameLine();
				ImReflect::Detail::text_label(label);
			} else {
				throw std::runtime_error("Unknown input type for bool");
			}
		} else {
			/* Const value, just display */
			ImReflect::Detail::text_label(label);
			ImReflect::Detail::imgui_tooltip("Value is const");

			ImGui::BeginDisabled();
			ImGui::SameLine();
			ImGui::Text("%s", value ? bool_settings.get_true_text().c_str() : bool_settings.get_false_text().c_str());
			ImGui::EndDisabled();
		}
		if (changed) {
			bool_response.changed();
		}
		ImReflect::Detail::check_input_states(bool_response);

	}

	/* TODO: delete this, only for testing*/
	enum class TestEnum {

	};
	static_assert(std::is_enum_v<TestEnum>, "TestEnum is not an enum");

	/* ========================= enums ========================= */
	template<typename E>
	struct type_settings<E, std::enable_if_t<std::is_enum_v<E>, void>> : ImRequired<E>,
		ImReflect::Detail::radio_widget<E>,
		ImReflect::Detail::dropdown_widget<E>,
		ImReflect::Detail::drag_widget<E>,
		ImReflect::Detail::drag_speed<E>,
		ImReflect::Detail::slider_widget<E> {
		type_settings() :
			/* Default settings */
			ImReflect::Detail::input_type(ImReflect::Detail::input_type_widget::Dropdown),
			ImReflect::Detail::drag_speed<E>(0.01f) {
		}
	};

	/* Use magic enum to reflect */
	template<typename E>
	std::enable_if_t<std::is_enum_v<E>, void>
		tag_invoke(Detail::ImInputLib_t, const char* label, E& value, ImSettings& settings, ImResponse& response) {
		auto& enum_settings = settings.get<E>();
		auto& enum_response = response.get<E>();

		constexpr bool is_const = std::is_const_v<E>;

		const auto enum_values = magic_enum::enum_values<E>();
		const auto& enum_names = magic_enum::enum_names<E>();
		const int enum_count = static_cast<int>(enum_values.size());

		bool changed = false;
		if (is_const) ImGui::BeginDisabled();
		if (enum_settings.is_radio()) {
			const auto id = Detail::scope_id("radio_enum");

			int int_value = static_cast<int>(value);

			const float child_width = ImGui::CalcItemWidth();
			const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
			const ImGuiStyle& style = ImGui::GetStyle();
			const float child_height = label_size.y + style.ScrollbarSize + (style.WindowPadding.y) * 2.0f;
			const ImVec2 child_size(child_width, child_height);

			/* Evenly spread radio buttons */
			if (ImGui::BeginChild("##radio_enum", child_size, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
				for (int i = 0; i < enum_count; ++i) {
					changed |= ImGui::RadioButton(enum_names[i].data(), &int_value, static_cast<int>(enum_values[i]));
					if (is_const) ImReflect::Detail::imgui_tooltip("Value is const");
					if (i < enum_count - 1) ImGui::SameLine();
				}
				if constexpr (is_const == false) value = static_cast<E>(int_value);
			}

			ImGui::EndChild();
			ImGui::SameLine();
			ImReflect::Detail::text_label(label);
			if constexpr (is_const) ImReflect::Detail::imgui_tooltip("Value is const");
		} else if (enum_settings.is_dropdown()) {
			const auto id = Detail::scope_id("dropdown_enum");

			int int_value = static_cast<int>(value);

			std::vector<const char*> item_vec;
			for (const auto& name : enum_names) {
				item_vec.push_back(name.data());
			}

			changed = ImGui::Combo(label, &int_value, item_vec.data(), enum_count);
			if constexpr (is_const == false) value = static_cast<E>(int_value);
			else ImReflect::Detail::imgui_tooltip("Value is const");
		} else if (enum_settings.is_slider()) {
			const auto id = Detail::scope_id("slider_enum");

			int int_value = static_cast<int>(value);
			const int min = static_cast<int>(enum_values.front());
			const int max = static_cast<int>(enum_values.back());

			const char* index_name = magic_enum::enum_name(value).data();

			changed = ImGui::SliderInt(label, &int_value, min, max, index_name);
			if constexpr (is_const == false) value = static_cast<E>(int_value);
			else ImReflect::Detail::imgui_tooltip("Value is const");
		} else if (enum_settings.is_drag()) {
			const auto id = Detail::scope_id("drag_enum");

			int int_value = static_cast<int>(value);
			const int min = static_cast<int>(enum_values.front());
			const int max = static_cast<int>(enum_values.back());

			const auto& speed = enum_settings.get_speed();
			const char* index_name = magic_enum::enum_name(value).data();

			changed = ImGui::DragInt(label, &int_value, speed, min, max, index_name);
			if constexpr (is_const == false) value = static_cast<E>(int_value);
			else ImReflect::Detail::imgui_tooltip("Value is const");
		} else {
			throw std::runtime_error("Unknown input type for enum");
		}
		if (is_const) ImGui::EndDisabled();

		if (changed) {
			enum_response.changed();
		}
		ImReflect::Detail::check_input_states(enum_response);
	}
}

static_assert(svh::is_tag_invocable_v<ImReflect::Detail::ImInputLib_t, const char*, int&, ImSettings&, ImResponse&>, "int tag_invoke not found");



// ============================================================================
// File: ImReflect_std.hpp
// ============================================================================

#include <imgui_stdlib.h>



#include <algorithm>
#include <vector>
#include <list>
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
			size_t item_count = 0;
			if constexpr (has_size) {
				item_count = value.size();
			} else {
			}

			const auto disabled_plus_button = []() {
				ImGui::SameLine();
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
					ImGui::SameLine();
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

			const auto disabled_minus_button = []() {
				ImGui::SameLine();
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
						ImGui::SameLine();
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
				map_settings.template push<std::tuple>().same_line(true);
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

		if constexpr (type_count <= 1) {
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
