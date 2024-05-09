#pragma once

#include <array>
#include <cstdint>
#include <optional>

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// Copyright (c) 2024 Alain Mosnier <alain@wanamoon.net>
// See https://github.com/amosnier/utf-8/blob/main/src/utf8/LICENSE for details.

namespace utf8 {

/// @brief UTF-8 decoder, one byte at a time
///
/// From our interpretation of the Unicode specification:
///
/// - We shall emit a replacement character (U+FFFD) for every "maximal subpart" in error, i.e. if a subpart that was so
/// far legal is interrupted by an unexpected byte, we shall emit a replacement character for that subpart, and reset
/// decoding at the interrupting byte. A maximal subpart's length may be as low as one byte, if a start byte was
/// expected but a non-start byte is found instead.
/// - If a sequence is terminated while we were decoding a multi-byte character, we shall emit a concluding replacement
/// character.
///
/// When it comes to design, this decoder is implemented as a Finite State Machine, as specified at
/// http://unicode.org/mail-arch/unicode-ml/y2003-m02/att-0467/01-The_Algorithm_to_Valide_an_UTF-8_String. That
/// specification is for UTF-8 validation, but the exact same FSM can also be used for decoding, by building the decoded
/// code points one byte at a time, instead of just validating the input. In fact,
/// https://bjoern.hoehrmann.de/utf-8/decoder/dfa/ does exactly that. In particular, both algorithms use the exact
/// same "character classes" and states, the former apparently being significantly older (2003) than the latter (2009),
/// but the latter does not seem to refer to the former.
///
/// This implementation is a port of Björn Höhrmann's implementation to modern C++.
///
/// The UTF-8 encoded range is mostly contiguous, from U+0000 to U+010000, but there are exceptions: so called UTF-16
/// surrogate halves, U-D800-U+DFFF, would be encoded on three bytes but are invalid UTF-8 sequences. Our state machine,
/// like the ones it is derived from, automatically takes care of this case.
class decoder {
	// The following table contains a mapping of byte values to "character classes" (zero to eleven). By our
	// definition of character class, for any character, its class is all we need to know how to treat it for
	// decoding, in every state.
	static constexpr std::array<uint8_t, 0x100> char_classes_{
	    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 00..0f
	    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 10..1f
	    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 20..2f
	    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 30..3f
	    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 40..4f
	    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 50..5f
	    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 60..6f
	    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 70..7f
	    0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, // 80..8f
	    0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, // 90..9f
	    0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, // a0..af
	    0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, // b0..bf
	    0x8, 0x8, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, // c0..cf
	    0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, // d0..df
	    0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, // e0..ef
	    0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, // f0..ff
	};

	static constexpr auto num_classes_ = 12;
	static constexpr auto num_fsm_rows_ = 8;
	static constexpr unsigned long replacement_char_ = 0xfffd;

	enum class state : uint8_t { start = 0, next1, next2, next3, next4, next5, next6, next7, error };

	// The following table encodes the FSM, i.e. for every couple { state, received character class }, the resulting
	// next state. Because, we have automatic (and specific) error handling, transitions from the error state are
	// not handled here (unlike in Bjoern Hoehrmann's implementation).
	static constexpr std::array<state, num_fsm_rows_ * num_classes_> fsm_{
	    state::start, state::error, state::next1, state::next2, state::next4, state::next7, // state::start
	    state::next6, state::error, state::error, state::error, state::next3, state::next5, // state::start
	    state::error, state::start, state::error, state::error, state::error, state::error, // state::next1
	    state::error, state::start, state::error, state::start, state::error, state::error, // state::next1
	    state::error, state::next1, state::error, state::error, state::error, state::error, // state::next2
	    state::error, state::next1, state::error, state::next1, state::error, state::error, // state::next2
	    state::error, state::error, state::error, state::error, state::error, state::error, // state::next3
	    state::error, state::next1, state::error, state::error, state::error, state::error, // state::next3
	    state::error, state::next1, state::error, state::error, state::error, state::error, // state::next4
	    state::error, state::error, state::error, state::next1, state::error, state::error, // state::next4
	    state::error, state::error, state::error, state::error, state::error, state::error, // state::next5
	    state::error, state::next2, state::error, state::next2, state::error, state::error, // state::next5
	    state::error, state::next2, state::error, state::error, state::error, state::error, // state::next6
	    state::error, state::next2, state::error, state::next2, state::error, state::error, // state::next6
	    state::error, state::next2, state::error, state::error, state::error, state::error, // state::next7
	    state::error, state::error, state::error, state::error, state::error, state::error, // state::next7
	};

	unsigned long code_{};
	state state_{state::start};

	enum class to_deliver { nothing, code_point, error };

	to_deliver to_deliver_{};

	/// @brief Calculate next state
	///
	/// @param s Current state
	/// @param type Received byte type
	///
	/// @return The next state
	constexpr static auto next_state(state s, uint8_t type) -> state
	{
		return fsm_.at(static_cast<uint8_t>(s) * num_classes_ + type);
	}

	/// @brief Extract payload from start byte
	///
	/// @param byte The start byte
	/// @param type The start byte type
	///
	/// @return The payload
	constexpr static auto start_byte_payload(uint8_t byte, uint8_t type) -> uint8_t
	{
		static constexpr auto byte_mask = 0xff;
		return byte & (byte_mask >> type);
	}

public:
	/// @brief Decode one byte
	///
	/// @param byte The byte to decode
	///
	/// @return A decoded code point if there is one or nothing otherwise
	///
	/// @warning Because a byte in error could interrupt the decoding of a so far valid multi-byte code point, a
	/// single input byte can effectively generate two code points. Hence, if this function returns something, the
	/// invoker shall once invoke the fetch function. Failure to do so might lead to missing code points.
	constexpr auto decode(char8_t byte) -> std::optional<unsigned long>
	{
		const auto type = char_classes_.at(byte);

		static constexpr auto data_mask = 0x3f;
		static constexpr auto data_shift = 6;

		to_deliver_ = to_deliver::nothing;

		const auto new_state = next_state(state_, type);

		if (new_state == state::error) {
			if (state_ == state::start) { // single byte in error
				return replacement_char_;
			}
			state_ = next_state(state::start, type);
			switch (state_) {
			case state::error: // interruption by byte in error
				state_ = state::start;
				to_deliver_ = to_deliver::error;
				return replacement_char_;
			case state::start: // interruption by single-byte code point
				code_ = start_byte_payload(byte, type);
				to_deliver_ = to_deliver::code_point;
				return replacement_char_;
			default: // interruption by multi-byte start byte
				code_ = start_byte_payload(byte, type);
				return replacement_char_;
			}
		}

		code_ = (state_ == state::start) ? start_byte_payload(byte, type)
						 : (code_ << data_shift) | (byte & data_mask);
		state_ = new_state;

		if (state_ == state::start) {
			return code_;
		}

		return {};
	}

	/// @brief Fetch any extra decoded code point
	///
	/// @return An extra decoded code point if there is one or nothing otherwise
	constexpr auto fetch() -> std::optional<unsigned long>
	{
		const auto code = to_deliver_ == to_deliver::code_point ? std::optional{code_}
				  : to_deliver_ == to_deliver::error	? std::optional{replacement_char_}
									: std::nullopt;

		to_deliver_ = to_deliver::nothing;
		return code;
	}

	/// @brief Check for error at the end of the UTF-8 sequence
	///
	/// @return Replacement character in case of error or nothing otherwise
	///
	/// @note This function is meant to be called once at the end of the UTF-8 sequence, but nothing prevents the
	/// invoker from calling it at other occasions, although that does not really make sense. Since this function
	/// is const (does not change the decoder state), the invoker could, too, "change their mind" and decide after
	/// invoking this function that this was not the end after all. Again, that does not really make sense, but
	/// preventing it is not either really necessary.
	[[nodiscard]] constexpr auto check_last_error() const -> std::optional<unsigned long>
	{
		return state_ != state::start ? std::optional{replacement_char_} : std::nullopt;
	}
};

} // namespace utf8
