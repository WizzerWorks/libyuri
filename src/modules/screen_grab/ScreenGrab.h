/*
 * ScreenGrab.h
 *
 *  Created on: 7.3.2013
 *      Author: neneko
 */

#ifndef SCREENGRAB_H_
#define SCREENGRAB_H_

#include "yuri/core/BasicIOThread.h"
#include "X11/Xlib.h"
namespace yuri {
namespace screen {

class ScreenGrab: public core::BasicIOThread
{
private:
	ScreenGrab(log::Log &log_, core::pwThreadBase parent, core::Parameters &parameters);
	virtual void run();
	virtual bool set_param(const core::Parameter &param);
	std::string display;
	double fps;
	yuri::shared_ptr<Display> dpy;
	Window win;
public:
	virtual ~ScreenGrab();
	IO_THREAD_GENERATOR_DECLARATION
	static core::pParameters configure();
};

} /* namespace screen */
} /* namespace yuri */


#endif /* SCREENGRAB_H_ */
