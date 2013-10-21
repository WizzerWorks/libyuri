/*
 * raw_audio_frame_params.cpp
 *
 *  Created on: 21.10.2013
 *      Author: neneko
 */

#include "raw_audio_frame_types.h"
#include "raw_audio_frame_params.h"

namespace yuri {
namespace core {
namespace raw_audio_format {


namespace {
format_info_map_t raw_audio_formats_info = {
		{unsigned_8bit, {unsigned_8bit, "Unsigned 8bit", {"u8"}, 8}},
		{unsigned_16bit, {unsigned_16bit, "Unsigned 16bit (little endian)", {"u16", "u16_le"}, 16}},
		{signed_16bit, {signed_16bit, "Signed 16bit (little endian)", {"s16","s16_le"}, 16}},
		{unsigned_24bit, {unsigned_24bit, "Unsigned 24bit (little endian)", {"u24","u24_le"}, 24}},
		{signed_24bit, {signed_24bit, "Signed 24bit (little endian)", {"s24", "s24_le"}, 24}},
		{unsigned_32bit, {unsigned_32bit, "Unsigned 32bit (little endian)", {"u32", "u32_le"}, 32}},
		{signed_32bit, {signed_32bit, "Signed 32bit (little endian)", {"s32","s32_le"}, 32}},
		{unsigned_48bit, {unsigned_48bit, "Unsigned 48bit (little endian)", {"u48","u48_le"}, 48}},
		{signed_48bit, {signed_48bit, "Signed 148bit (little endian)", {"s48","s48_le"}, 48}},
		{float_32bit, {float_32bit, "Float 32 bit (little endian)", {"f32","f32_le"}, 32}},

		{unsigned_16bit_be, {unsigned_16bit_be, "Unsigned 16bit (big endian)", {"u16_be"}, 16, false}},
		{signed_16bit_be, {signed_16bit_be, "Signed 16bit (big endian)", {"s16_be"}, 16, false}},
		{unsigned_24bit_be, {unsigned_24bit_be, "Unsigned 24bit (big endian)", {"u24_be"}, 24, false}},
		{signed_24bit_be, {signed_24bit_be, "Signed 24bit (big endian)", {"s24_be"}, 24, false}},
		{unsigned_32bit_be, {unsigned_32bit_be, "Unsigned 32bit (big endian)", {"u32_be"}, 32, false}},
		{signed_32bit_be, {signed_32bit_be, "Signed 32bit (big endian)", {"s32_be"}, 32, false}},
		{unsigned_48bit_be, {unsigned_48bit_be, "Unsigned 48bit (big endian)", {"u48_be"}, 48, false}},
		{signed_48bit_be, {signed_48bit_be, "Signed 148bit (big endian)", {"s48_be"}, 48, false}},
		{float_32bit_be, {float_32bit_be, "Float 32 bit (big endian)", {"f32_be"}, 32, false}},
};

}

const raw_audio_format_t &get_format_info(format_t format)
{
	auto it = raw_audio_formats_info.find(format);
	if (it == raw_audio_formats_info.end()) throw std::runtime_error("Unknown format");
	return it->second;
}



}
}
}

