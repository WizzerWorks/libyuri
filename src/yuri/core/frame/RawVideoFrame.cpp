/*
 * RawVideoFrame.cpp
 *
 *  Created on: 8.9.2013
 *      Author: neneko
 */

#include "RawVideoFrame.h"
#include "raw_frame_types.h"
#include "raw_frame_params.h"
#include "yuri/core/thread/FixedMemoryAllocator.h"
namespace yuri {
namespace core  {


pRawVideoFrame RawVideoFrame::create_empty(format_t format, resolution_t resolution, bool fixed, interlace_t interlace, field_order_t field_order)
{
	pRawVideoFrame frame;
	try {
		const auto& info = raw_format::get_format_info(format);
//		size_t planes = info.planes.size();
		// Creating with 0 planes and them emplacing planes into it.
		frame = make_shared<RawVideoFrame>(format, resolution, 0);
		for (const auto& p: info.planes) {
			const size_t line_size_nom = resolution.width * p.bit_depth.first;
			const size_t line_size_den = p.bit_depth.second * p.sub_x * 8;
			const size_t line_size_unaligned = line_size_nom / line_size_den + line_size_nom % line_size_den;
			const size_t line_size = p.alignment_requirement?line_size_unaligned+line_size_unaligned%p.alignment_requirement:line_size_unaligned;
			const size_t frame_size = line_size * resolution.height / p.sub_y;
			if (!fixed) {
				frame->emplace_back(frame_size, resolution, line_size);
			} else {
				auto mem = FixedMemoryAllocator::get_block(frame_size);
				Plane::vector_type data{mem.first, frame_size, mem.second};
				frame->emplace_back(std::move(data), resolution, line_size);
			}

		}
		frame->set_interlacing(interlace);
		frame->set_field_order(field_order);

	}
	catch (std::runtime_error&) {}
	return frame;
}
pRawVideoFrame RawVideoFrame::create_empty(format_t format, resolution_t resolution, const uint8_t* data, size_t size, bool fixed, interlace_t interlace, field_order_t field_order)
{
	pRawVideoFrame frame = create_empty(format, resolution, fixed, interlace, field_order);
	if (frame) {
		if (PLANE_SIZE(frame,0) < size) size = PLANE_SIZE(frame,0);
		std::copy(data, data + size, PLANE_DATA(frame,0).begin());
	}
	return frame;
}

RawVideoFrame::RawVideoFrame(format_t format, resolution_t resolution, size_t plane_count)
:VideoFrame(format, resolution)
{
	set_planes_count(plane_count);
}
RawVideoFrame::~RawVideoFrame() noexcept
{

}

void RawVideoFrame::set_planes_count(index_t count)
{
	Plane p(0, {0, 0}, 0);
	planes_.resize(count, p);
}

void RawVideoFrame::push_back(const Plane& plane)
{
	planes_.push_back(plane);
}
void RawVideoFrame::push_back(Plane&& plane)
{
	planes_.push_back(plane);
}

pFrame RawVideoFrame::do_get_copy() const {
	pRawVideoFrame frame = make_shared<RawVideoFrame>(get_format(), get_resolution());
	RawVideoFrame& rvframe = *frame;
	copy_parameters(rvframe);
	std::copy(begin(),end(),rvframe.begin());
	return frame;
}

size_t RawVideoFrame::do_get_size() const noexcept
{
	return std::accumulate(planes_.begin(), planes_.end(), 0,
			[](const size_t& a, const Plane& plane){return a + plane.get_size();});
}

void RawVideoFrame::copy_parameters(Frame& other) const {
	try {
		RawVideoFrame& frame = dynamic_cast<RawVideoFrame&>(other);
		frame.set_planes_count(get_planes_count());
	}
	catch (std::bad_cast&) {
		throw std::runtime_error("Tried to set VideoFrame params to a type not related to VideoFrame");
	}
	VideoFrame::copy_parameters(other);
}
}
}



