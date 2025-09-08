#pragma once
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