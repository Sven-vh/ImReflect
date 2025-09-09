#pragma once
#include <extern/svh/scope.hpp>

namespace ImGui {
	namespace Reflect {

		using ImContext = svh::scope;

		struct ImResponse {
			bool Changed = false;
		};

		/* Builder class */
		template<typename T>
		struct InputBuilder : ImContext {
			const char* label = nullptr;
			T* value = nullptr;

			ImResponse Execute() {
				return Input(label, *value, static_cast<const ImContext&>(*this));
			}

			operator ImResponse() {
				return Execute();
			}
		};

		/* Single Input Field */
		template<typename T>
		ImResponse Input(const char* label, T& value, const ImContext& ctx) {
			(void*)&ctx; // Unused
			(void*)&value; // Unused
			(void*)&label; // Unused
			return {};
		}

		template<typename T>
		InputBuilder<T> Input(const char* label, T& value) {
			InputBuilder<T> builder;
			builder.label = label;
			builder.value = &value;
			return builder;
		}
	}
}