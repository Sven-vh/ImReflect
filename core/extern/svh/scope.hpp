// Source: https://github.com/Sven-vh/fluent-builder-pattern
#pragma once
#include <unordered_map>
#include <typeindex>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <type_traits>

/* Whether to insert a default object when calling get at root level if not found in any scope*/
#ifndef SVH_AUTO_INSERT
#define SVH_AUTO_INSERT true
#endif

// Forward declare
template<class T>
struct type_settings;

namespace svh {

	struct scope {
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
		type_settings<T>& push() {
			return _push<T>();
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
			auto& next = _push<T>();
			/* If rest is something, we recurse */
			/* If rest is nothing, we fall back to single T push */
			return next.template push<U, Rest...>();
		}

		/// <summary>
		/// Push a new scope for type T with default values.
		/// </summary>
		/// <typeparam name="T">The type of the scope to push</typeparam>
		/// <returns>Reference to the pushed scope</returns>
		/// <exception cref="std::runtime_error">If an existing child has an unexpected type</exception>
		template<class T>
		type_settings<T>& push_default() {
			const std::type_index key = get_type_key<T>();

			/* reset if present */
			auto it = children.find(key);
			if (it != children.end()) {
				auto* found = dynamic_cast<type_settings<T>*>(it->second.get());
				if (!found) {
					throw std::runtime_error("Existing child has unexpected type");
				}
				*found = type_settings<T>{}; // Reset to default
				return *found;
			}

			/* Else create new */
			return emplace_new<T>();
		}

		/// <summary>
		/// Pop to parent scope. Throws if at root.
		/// </summary>
		/// <returns>Reference to the parent scope</returns>
		/// <exception cref="std::runtime_error">If at root</exception>
		scope& pop(int count = 1) const {
			if (!has_parent() && count == 1) {
				throw std::runtime_error("No parent to pop to");
			}
			if (count == 1) {
				return *parent;
			}

			return parent->pop(--count);
		}

		/// <summary>
		/// Get the scope for type T. If not found, recurse to parent.
		/// If not found and at root, optionally insert a default one depending on ``SVH_AUTO_INSERT``.
		/// </summary>
		/// <typeparam name="T">The type of the scope to get</typeparam>
		/// <returns>Reference to the found scope</returns>
		/// <exception cref="std::runtime_error">If not found and at root and ``SVH_AUTO_INSERT`` is false</exception>
		template <class T>
		type_settings<T>& get() {
			return _get<T>();
		}

		template <class T, class U, class... Rest>
		auto& get() {
			auto& next = _get<T>();
			/* If rest is something, we recurse */
			/* If rest is nothing, we fall back to single T get */
			return next.template get<U, Rest...>();
		}

		/// <summary>
		/// Get the scope for type T. If not found, recurse to parent.
		/// If not found and at root, throw.
		/// </summary>
		/// <typeparam name="T">The type of the scope to get</typeparam>
		/// <returns>Reference to the found scope</returns>
		/// <exception cref="std::runtime_error">If not found</exception>
		template <class T>
		const type_settings<T>& get() const {
			return _get<T>();
		}

		template <class T, class U, class... Rest>
		const auto& get() const {
			auto& next = _get<T>();
			/* If rest is something, we recurse */
			/* If rest is nothing, we fall back to single T get */
			return next.template get<U, Rest...>();
		}

		/// <summary>
		/// Find the scope for type T. If not found, recurse to parent.
		/// If not found and at root, return nullptr.
		/// </summary>
		/// <typeparam name="T">The type of the scope to find</typeparam>
		/// <returns>Pointer to the found scope or nullptr if not found</returns>
		/// <exception cref="std::runtime_error">If an existing child has an unexpected type</exception>
		template <class T>
		type_settings<T>* find() const {
			const std::type_index key = get_type_key<T>();
			auto it = children.find(key);
			if (it != children.end()) {
				auto* found = dynamic_cast<type_settings<T>*>(it->second.get());
				if (!found) {
					throw std::runtime_error("Existing child has unexpected type");
				}
				return found;
			}
			if (has_parent()) {
				return parent->find<T>();
			}
			return nullptr; // Not found
		}

		/// <summary>
		/// Debug log the scope tree to console.
		/// </summary>
		/// <param name="indent">Indentation level</param>
		void debug_log(int indent = 0) const {
			std::string prefix(indent, ' ');
			for (const auto& pair : children) {
				const auto& key = pair.first;
				const auto& child = pair.second;
				const auto& name = key.name();
				std::cout << prefix << name << "\n";
				child->debug_log(indent + 2);
			}
		}

	private:
		scope* parent = nullptr; /* Root level */
		std::unordered_map<std::type_index, std::shared_ptr<scope>> children; /* shared since we need to copy the base*/

		bool is_root() const { return parent == nullptr; }
		bool has_parent() const { return parent != nullptr; }

		template<class T>
		constexpr std::type_index get_type_key() const { return std::type_index{ typeid(std::decay_t<T>) }; }

		template<class T>
		type_settings<T>& emplace_new() {
			const std::type_index key = get_type_key<T>();
			auto child = std::make_unique<type_settings<T>>();
			auto& ref = *child;
			child->parent = this;
			children.emplace(key, std::move(child));
			return ref;
		}

		/* Actual implementation fo push */
		template<class T>
		type_settings<T>& _push() {
			const std::type_index key = get_type_key<T>();

			/* Reuse if present */
			auto it = children.find(key);
			if (it != children.end()) {
				auto* found = dynamic_cast<type_settings<T>*>(it->second.get());
				if (!found) {
					throw std::runtime_error("Existing child has unexpected type");
				}
				return *found;
			}

			/* copy if found recursive */
			if (has_parent()) {
				auto* found = find<T>();
				if (found) {
					auto child = std::make_unique<type_settings<T>>(*found); /* Copy */
					auto& ref = *child;
					child->parent = this;
					child->children.clear(); /* Clear children, we only copy the base settings */
					children.emplace(key, std::move(child));
					return ref;
				}
			}

			/* Else create new */
			return emplace_new<T>();
		}


		template<class T>
		type_settings<T>& _get() {

			auto* found = find<T>();
			if (found) {
				return *found;
			}

			if (is_root() && SVH_AUTO_INSERT) {
				return emplace_new<T>();
			}

			throw std::runtime_error("Type not found");
		}

		template<class T>
		const type_settings<T>& _get() const {
			auto* found = find<T>();
			if (found) {
				return *found;
			}
			throw std::runtime_error("Type not found");
		}
	};
} // namespace svh

template<class T>
struct type_settings : svh::scope {};

/* Macros for indenting */
#define ____
#define ________
#define ____________
#define ________________
