#pragma once

#include "utf-8/decoder.h"

#include <ranges>

namespace utf8 {

namespace detail {

template <typename R, typename T>
concept viewable_range_of = std::ranges::viewable_range<R> and std::same_as<std::ranges::range_value_t<R>, T>;

template <typename R, typename T>
concept input_range_of = std::ranges::input_range<R> and std::same_as<std::ranges::range_value_t<R>, T>;

} // namespace detail

/// @brief Decode a UTF-8 range into Unicode code points
/// @tparam V The input range type
template <detail::input_range_of<char8_t> V>
	requires std::ranges::view<V>
class decode_view : public std::ranges::view_interface<decode_view<V>> {
	V view_{};

	struct nothing {};

	class iterator {
		std::ranges::iterator_t<V> it_{};
		std::ranges::sentinel_t<V> end_{};
		utf8::decoder decoder_{};
		unsigned long code_{};
		bool has_last_error_{};

		constexpr void try_decode_one_code_point()
		{
			const auto code = decoder_.fetch();

			if (code.has_value()) {
				code_ = *code;
				return;
			}

			if (it_ != end_) {
				++it_;
			}

			decode();
		}
		constexpr void decode()
		{
			std::optional<unsigned long> code;

			while (it_ != end_ && not(code = decoder_.decode(*it_)).has_value()) {
				++it_;
			}

			if (it_ == end_) {
				if ((code = decoder_.check_last_error()).has_value()) {
					has_last_error_ = true;
					code_ = *code;
				}
			} else {
				code_ = *code;
			}
		}

	public:
		using difference_type = ptrdiff_t;
		using value_type = unsigned long;

		constexpr iterator(auto &&it, auto &&end)
		    : it_{std::forward<decltype(it)>(it)}, end_{std::forward<decltype(end)>(end)}
		{
			decode();
		}
		constexpr auto operator++() -> iterator &
		{
			if (has_last_error_) {
				has_last_error_ = false;
			} else {
				try_decode_one_code_point();
			}
			return *this;
		}
		constexpr void operator++(int) { ++(*this); }
		constexpr auto operator*() const -> value_type { return code_; }
		constexpr auto operator==(nothing /*not_used*/) const -> bool
		{
			return it_ == end_ and not has_last_error_;
		}
	};

public:
	constexpr decode_view(V view) : view_{std::move(view)} {}
	constexpr auto begin() -> iterator { return {std::ranges::begin(view_), std::ranges::end(view_)}; }
	constexpr auto end() -> nothing { return {}; }
};

// Deduction guide
template <typename R>
decode_view(R &&) -> decode_view<std::views::all_t<R>>;

namespace views::detail {

struct decode : std::ranges::range_adaptor_closure<decode> {
	template <utf8::detail::viewable_range_of<char8_t> R>
	constexpr auto operator()(R &&arg) const
	{
		return decode_view{std::forward<R>(arg)};
	}

	// Overload for ranges of char, since those often contains UTF-8 data nowadays.
	template <utf8::detail::viewable_range_of<char> R>
	constexpr auto operator()(R &&arg) const
	{
		return decode_view{std::forward<R>(arg) |
				   std::views::transform([](char c) { return std::bit_cast<char8_t>(c); })};
	}
};

} // namespace views::detail

namespace views {

constexpr inline detail::decode decode{};

} // namespace views

} // namespace utf8
