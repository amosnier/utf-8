#include "utf-8.h"

#include <algorithm>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

auto main() -> int
{
	static_assert(std::ranges::equal("$£Иह€한𐍈" | utf8::views::decode,
					 std::array{0x00000024, 0x000000a3, 0x00000418, 0x00000939, 0x000020ac,
						    0x0000d55c, 0x00010348, 0x00000000}));
	static_assert(std::ranges::equal(std::array{char8_t{0x24}, char8_t{0xc2}} | utf8::views::decode,
					 std::array{0x00000024, 0x0000fffd}));
	return 0;
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
