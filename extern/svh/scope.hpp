#pragma once
#include <unordered_map>
#include <map>
#include <typeindex>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <type_traits>

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
			std::string prefix(indent, '==');
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
