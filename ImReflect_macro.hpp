#pragma once

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

namespace ImReflect {
	namespace Detail {
		struct ImContext {};
	}
}