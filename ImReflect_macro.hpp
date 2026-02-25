#pragma once
#include <extern/visit_struct/visit_struct.hpp>

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