#include "utf-8/decoder.h"

#include <cassert>

// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, which we use a lot here, was written when UTF-8 had
// support for characters of 5 and 6 bytes, which no longer is the case! Additionally, when 5 and 6-byte were supported,
// the 4-byte span was larger than what current applies. Also, as recommended by Unicode, we emit a replacement
// character (U+FFFD) for every "maximal subpart" in error, and it seems like the document above uses an obsolete
// definition of that notion. More specifically, the UTF-8 specification is now as restrictive as possible on allowed
// byte values at every position, allowing the detection of errors as early as possible, typically yielding shorter
// maximal subparts and hence more numerous replacement characters.
//
// For all these reasons, while the problematic examples specified in the document above provide excellent test cases,
// the expected result quite often differs from that of the document.

namespace {

void test_normal()
{
	utf8::decoder decoder{};

	// Characters of various lengths from Wikipedia

	assert(decoder.decode('a') == 97U);
	assert(not decoder.fetch().has_value());
	assert(decoder.decode('b') == 98U);
	assert(not decoder.fetch().has_value());
	assert(decoder.decode('$') == 0x24U);
	assert(not decoder.fetch().has_value());
	assert(not decoder.decode("£"[0]).has_value());
	assert(decoder.decode("£"[1]) == 0xa3U);
	assert(not decoder.decode("И"[0]).has_value());
	assert(decoder.decode("И"[1]) == 0x418U);
	assert(not decoder.decode("ह"[0]).has_value());
	assert(not decoder.decode("ह"[1]).has_value());
	assert(decoder.decode("ह"[2]) == 0x939U);
	assert(not decoder.decode("€"[0]).has_value());
	assert(not decoder.decode("€"[1]).has_value());
	assert(decoder.decode("€"[2]) == 0x20acU);
	assert(not decoder.decode("한"[0]).has_value());
	assert(not decoder.decode("한"[1]).has_value());
	assert(decoder.decode("한"[2]) == 0xd55cU);
	assert(not decoder.decode("𐍈"[0]).has_value());
	assert(not decoder.decode("𐍈"[1]).has_value());
	assert(not decoder.decode("𐍈"[2]).has_value());
	assert(decoder.decode("𐍈"[3]) == 0x10348U);
	assert(not decoder.fetch().has_value());
	assert(not decoder.check_last_error().has_value());
}

void test_first_of_a_certain_len()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 2.1

	assert(decoder.decode(0) == 0x0U);

	assert(not decoder.decode(0xc2).has_value());
	assert(decoder.decode(0x80) == 0x80U);

	assert(not decoder.decode(0xe0).has_value());
	assert(not decoder.decode(0xa0).has_value());
	assert(decoder.decode(0x80) == 0x800U);

	assert(not decoder.decode(0xf0).has_value());
	assert(not decoder.decode(0x90).has_value());
	assert(not decoder.decode(0x80).has_value());
	assert(decoder.decode(0x80) == 0x10000U);
}

void test_last_of_a_certain_len()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 2.2

	assert(decoder.decode(0x7f) == 0x7fU);

	assert(not decoder.decode(0xdf).has_value());
	assert(decoder.decode(0xbf) == 0x7ffU);

	assert(not decoder.decode(0xef).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode(0xbf) == 0xffffU);

	assert(not decoder.decode(0xf4).has_value());
	assert(not decoder.decode(0x8f).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode(0xbf) == 0x10ffffU);
}

void test_other_boundary_conditions()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 2.3

	assert(not decoder.decode(0xed).has_value());
	assert(not decoder.decode(0x9f).has_value());
	assert(decoder.decode(0xbf) == 0xd7ffU);

	assert(not decoder.decode(0xee).has_value());
	assert(not decoder.decode(0x80).has_value());
	assert(decoder.decode(0x80) == 0xe000U);

	assert(not decoder.decode(0xef).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode(0xbd) == 0xfffdU);

	// First 4-byte sequence out of range
	assert(not decoder.decode(0xf4).has_value());
	assert(decoder.decode(0x90) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.decode('"') == 0x22U);
}

void test_code_point_after_interruption()
{
	utf8 ::decoder decoder{};

	assert(not decoder.decode(0xf4).has_value());
	assert(not decoder.decode(0x8f).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode('"') == 0xfffdU);
	assert(decoder.fetch() == 0x22U);

	assert(not decoder.decode(0xf4).has_value());
	assert(not decoder.decode(0x8f).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode("£"[0]) == 0xfffdU);
	assert(decoder.decode("£"[1]) == 0xa3U);

	assert(not decoder.decode(0xf4).has_value());
	assert(not decoder.decode(0x8f).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode("ह"[0]) == 0xfffdU);
	assert(not decoder.decode("ह"[1]).has_value());
	assert(decoder.decode("ह"[2]) == 0x939U);

	assert(not decoder.decode(0xf4).has_value());
	assert(not decoder.decode(0x8f).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode("𐍈"[0]) == 0xfffdU);
	assert(not decoder.decode("𐍈"[1]).has_value());
	assert(not decoder.decode("𐍈"[2]).has_value());
	assert(decoder.decode("𐍈"[3]) == 0x10348U);
}

void test_unexpected_continuation_bytes()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 3.1

	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
	assert(decoder.decode(0x80) == 0xfffdU);

	assert(decoder.decode(0x8c) == 0xfffdU);
	assert(decoder.decode(0x8f) == 0xfffdU);
	assert(decoder.decode(0x94) == 0xfffdU);
	assert(decoder.decode(0xa1) == 0xfffdU);
	assert(decoder.decode(0xac) == 0xfffdU);
	assert(decoder.decode(0xb5) == 0xfffdU);
	assert(decoder.decode(0xbc) == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
}

void test_lonely_two_byte_start()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 3.2.1

	// Not actually valid starts byte
	assert(decoder.decode(0xc0) == 0xfffdU);
	assert(decoder.decode(' ') == 0x20U);
	assert(decoder.decode(0xc1) == 0xfffdU);
	assert(decoder.decode(' ') == 0x20U);

	// Valid start bytes
	assert(not decoder.decode(0xc2).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);
	assert(not decoder.decode(0xcf).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);
	assert(not decoder.decode(0xd0).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);
	assert(not decoder.decode(0xdf).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);
}

void test_lonely_three_byte_start()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 3.2.2

	assert(not decoder.decode(0xe0).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);
	assert(not decoder.decode(0xe8).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);
	assert(not decoder.decode(0xef).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);
}

void test_lonely_four_byte_start()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 3.2.3

	// Valid start bytes
	assert(not decoder.decode(0xf0).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);
	assert(not decoder.decode(0xf4).has_value());
	assert(decoder.decode(' ') == 0xfffdU);
	assert(decoder.fetch() == 0x20U);

	// Not actually valid starts byte, but the result in this case, at the code point level, is the same.
	assert(decoder.decode(0xf5) == 0xfffdU);
	assert(decoder.decode(' ') == 0x20U);
	assert(decoder.decode(0xf7) == 0xfffdU);
	assert(decoder.decode(' ') == 0x20U);
}

void test_interrupted_three_byte_sequences()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 3.3

	// Not actually a valid start sequences
	assert(not decoder.decode(0xe0).has_value());
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(not decoder.decode(0xe0).has_value());
	assert(decoder.decode(0x90) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);

	// Valid start sequences
	assert(not decoder.decode(0xe0).has_value());
	assert(not decoder.decode(0xa0).has_value());
	assert(decoder.decode('"') == 0xfffdU);
	assert(decoder.fetch() == 0x22U);
	assert(not decoder.decode(0xef).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode('"') == 0xfffdU);
	assert(decoder.fetch() == 0x22U);
}

void test_interrupted_four_byte_sequences()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 3.3

	// Not actually valid start sequences
	assert(not decoder.decode(0xf0).has_value());
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.decode('"') == 0x22U);
	assert(decoder.decode(0xf7) == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
	assert(decoder.decode('"') == 0x22U);

	// Valid start sequences
	assert(not decoder.decode(0xf0).has_value());
	assert(not decoder.decode(0x90).has_value());
	assert(not decoder.decode(0x80).has_value());
	assert(decoder.decode('"') == 0xfffdU);
	assert(decoder.fetch() == 0x22U);
	assert(not decoder.decode(0xf4).has_value());
	assert(not decoder.decode(0x8f).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode('"') == 0xfffdU);
	assert(decoder.fetch() == 0x22U);
}

void test_impossible_bytes()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 3.5
	assert(decoder.decode(0xfe) == 0xfffdU);
	assert(decoder.decode(0xff) == 0xfffdU);
	assert(decoder.decode(0xfe) == 0xfffdU);
	assert(decoder.decode(0xfe) == 0xfffdU);
	assert(decoder.decode(0xff) == 0xfffdU);
	assert(decoder.decode(0xff) == 0xfffdU);
}

void test_overlong()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 4.1

	assert(decoder.decode(0xc0) == 0xfffdU);
	assert(decoder.decode(0xaf) == 0xfffdU);

	assert(not decoder.decode(0xe0).has_value());
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(decoder.decode(0xaf) == 0xfffdU);

	assert(not decoder.decode(0xf0).has_value());
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(decoder.decode(0x80) == 0xfffdU);
	assert(decoder.decode(0xaf) == 0xfffdU);
}

void test_max_overlong()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 4.1

	assert(decoder.decode(0xc1) == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);

	assert(not decoder.decode(0xe0).has_value());
	assert(decoder.decode(0x9f) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);

	assert(not decoder.decode(0xf0).has_value());
	assert(decoder.decode(0x8f) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
}

void test_surrogates()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 5.1

	assert(not decoder.decode(0xed).has_value());
	assert(decoder.decode(0xa0) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(decoder.decode(0x80) == 0xfffdU);

	assert(not decoder.decode(0xed).has_value());
	assert(decoder.decode(0xbf) == 0xfffdU);
	assert(decoder.fetch() == 0xfffdU);
	assert(decoder.decode(0xbf) == 0xfffdU);
}

void test_non_characters()
{
	utf8 ::decoder decoder{};

	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt, 5.3
	//
	// As mandated in the latest Unicode specification at the time of writing, we transparently decode
	// so-called non-characters.

	assert(not decoder.decode(0xef).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode(0xbe) == 0xfffeU);

	assert(not decoder.decode(0xef).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode(0xbf) == 0xffffU);

	assert(not decoder.decode(0xef).has_value());
	assert(not decoder.decode(0xb7).has_value());
	assert(decoder.decode(0x90) == 0xfdd0U);

	assert(not decoder.decode(0xef).has_value());
	assert(not decoder.decode(0xb7).has_value());
	assert(decoder.decode(0xaf) == 0xfdefU);

	assert(not decoder.decode(0xf0).has_value());
	assert(not decoder.decode(0x9f).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode(0xbe) == 0x1fffeU);

	assert(not decoder.decode(0xf2).has_value());
	assert(not decoder.decode(0xaf).has_value());
	assert(not decoder.decode(0xbf).has_value());
	assert(decoder.decode(0xbf) == 0xaffffU);
}

} // namespace

auto main() -> int
{
	test_normal();
	test_first_of_a_certain_len();
	test_last_of_a_certain_len();
	test_other_boundary_conditions();
	test_code_point_after_interruption();
	test_unexpected_continuation_bytes();
	test_lonely_two_byte_start();
	test_lonely_three_byte_start();
	test_lonely_four_byte_start();
	test_interrupted_three_byte_sequences();
	test_interrupted_four_byte_sequences();
	test_impossible_bytes();
	test_overlong();
	test_max_overlong();
	test_surrogates();
	test_non_characters();

	return 0;
}
