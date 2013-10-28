/*!
 * @file 		GLXWindow.cpp
 * @author 		Zdenek Travnicek
 * @date 		31.5.2008
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2008 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "GLXWindow.h"
#include "yuri/graphics/GL.h"
#include <X11/XKBlib.h>
#include <GL/glxext.h>

namespace yuri
{
namespace graphics
{
#ifdef GLXWINDOW_USING_GLOBAL_MUTEX
yuri::mutex	GLXWindow::global_mutex;
#endif

std::map<GLXContext,shared_ptr<GLXWindow> > GLXWindow::used_contexts;
yuri::mutex GLXWindow::contexts_map_mutex;

GLXWindow::GLXWindow(log::Log &log_, core::pwThreadBase parent, core::Parameters &p)
	:WindowBase(log_, parent, p),win(0),noAttr(0),vi(0),
	override_redirect(false),hideDecoration(true),screen_number(0),vsync(false)
{
	core::pParameters def_params = configure();
	params.merge(*def_params);
	params.merge(p);
	log.set_label("[GLX] ");
}

GLXWindow::~GLXWindow()
{
	if (display) {
		glXDestroyContext(display.get(),glc);
		XDestroyWindow(display.get(),win);
		//XCloseDisplay(display);
		display.reset();
	}
	log[log::debug] << "GLXWindow::~GLXWindow" << std::endl;
}
core::pParameters GLXWindow::configure()
{
	core::pParameters p(new core::Parameters());
	(*p)["display"]=":0.0";
	(*p)["stereo"]=false;
	(*p)["width"]=640;
	(*p)["height"]=480;
	(*p)["x"]=0;
	(*p)["y"]=0;
	(*p)["cursor"]=false;
	return p;
}
void GLXWindow::initAttr()
{
	GLint defatr[]= {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER};
	addAttributes(4, defatr);
	if (use_stereo) addAttribute(GLX_STEREO);
}


void GLXWindow::addAttributes(int no, GLint *attrs)
{
	if (!noAttr) {
		noAttr=1;
	}
	attributes.resize(noAttr+no);
	for (int i=0;i<no;++i) attributes[noAttr+i-1]=attrs[i];
	noAttr+=no;
	attributes[noAttr-1]=None;

}

void GLXWindow::addAttribute(GLint attr)
{
	addAttributes(1,&attr);
}

bool GLXWindow::load_config()
{

	screen = params["display"].get<std::string>();
	use_stereo = params["stereo"].get<bool>();
	show_cursor = params["cursor"].get<bool>();
	x = params["x"].get<yuri::ssize_t>();
	y = params["y"].get<yuri::ssize_t>();
	width = params["width"].get<yuri::size_t>();
	height = params["height"].get<yuri::size_t>();
	return true;
}

bool GLXWindow::create_window()
{
	display.reset(XOpenDisplay(screen.c_str()),DisplayDeleter());
	if (!display) return false;
	log[log::debug] << "Connected to display " << screen;
	std::string::size_type ind=screen.find_last_of(':');
	if (ind!=std::string::npos) {
		ind = screen.find_first_of('.',ind);
		if (ind != std::string::npos) {
			screen_number=atoi(screen.substr(ind+1).c_str());
		}
	}
	log[log::debug] << "Screen number is " << screen_number;
	root=RootWindow(display.get(),screen_number);
	if (!root) return false;
	log[log::debug] << "Found root window";
	vi = glXChooseVisual(display.get(), screen_number, &attributes[0]);
	if (!vi) return false;
	log[log::debug] << "Found visual " << vi->visualid;
	cmap = XCreateColormap(display.get(), root, vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask
					| KeyReleaseMask;//ResizeRedirectMask;
	swa.border_pixel = 0;
	swa.background_pixel = 0;

	win = XCreateWindow(display.get(), root, x, y, width, height, 0, vi->depth,
			InputOutput, vi->visual, CWBackPixel | CWBorderPixel |CWColormap
			| CWEventMask, &swa);
	log[log::debug] << "X Window Created";
	return true;
}
bool GLXWindow::create()
{
	log[log::debug] << "creating GLX window";
	if (!load_config()) return false;
	initAttr();
#ifdef GLXWINDOW_USING_GLOBAL_MUTEX
	boost::mutex::scoped_lock l(global_mutex);
#else
	yuri::lock_t l(local_mutex);
#endif
	if (!create_window()) return false;
	setHideDecoration(hideDecoration);
	XStoreName(display.get(),win,winname.c_str());
	yuri::lock_t bgl(GL::big_gpu_lock);
	glc = glXCreateContext(display.get(), vi, NULL, GL_TRUE);
	glXMakeCurrent(display.get(), win, glc);
	log[log::debug] << "Created GLX Context";
	bgl.unlock();
	log[log::debug] << "Cursor " << (show_cursor ? "will" : "won't") << " be shown";

	if (!show_cursor) {
		log[log::debug] << "Creating cursor";
		Pixmap pixmap;
		Cursor cursor;
		XColor color;
		// Create 1x1 1bpp pixmap
		pixmap = XCreatePixmap(display.get(), win, 1, 1, 1);
		std::fill_n(reinterpret_cast<char*>(&color),sizeof(XColor),0);
//		memset((void*) &color, 0, sizeof(XColor));
		cursor = XCreatePixmapCursor(display.get(), pixmap, pixmap, &color, &color,
				0, 0);
		XDefineCursor(display.get(),win,cursor);
	}
	add_used_context(glc,dynamic_pointer_cast<GLXWindow>(get_this_ptr().lock_t()));
	do_move();
	do_show();

	return true;
}
namespace {
typedef struct
{
    unsigned long   flags;
    unsigned long   functions;
    unsigned long   decorations;
    long            input_mode;
    unsigned long   status;
} wm_hints;
}
void GLXWindow::setHideDecoration(bool value)
{
	if (value) {

		wm_hints hints;
		Atom mh = None;
		mh=XInternAtom(display.get(),"_MOTIF_WM_HINTS",0);
		hints.flags = 2;//MWM_HINTS_DECORATIONS;
		hints.decorations = 0;
		hints.functions = 0;
		hints.input_mode = 0;
		hints.status = 0;
		int r = XChangeProperty(display.get(), win,mh, mh, 32,PropModeReplace,
				(unsigned char *) &hints, 5);
		log[log::info] << "XChangeProperty returned " << r;
	}

}

void GLXWindow::swap_buffers()
{
#ifdef GLXWINDOW_USING_GLOBAL_MUTEX
	yuri::lock_t l(global_mutex);
#else
	yuri::lock_t l(local_mutex);
#endif
	assert(display);
	assert(win);
	glXSwapBuffers(display.get(), win);
}

void GLXWindow::show(bool /*value*/)
{
#ifdef GLXWINDOW_USING_GLOBAL_MUTEX
	yuri::lock_t l(global_mutex);
#else
	yuri::lock_t l(local_mutex);
#endif
	do_show();
}

void GLXWindow::move()
{
#ifdef GLXWINDOW_USING_GLOBAL_MUTEX
	yuri::lock_t l(global_mutex);
#else
	yuri::lock_t l(local_mutex);
#endif
	do_move();
}
void GLXWindow::do_show()
{
	assert(display);
	assert(win);
	XMapWindow(display.get(), win);
	XMoveWindow(display.get(), win,x,y);
}
void GLXWindow::do_move()
{
	assert(display);
	assert(win);
	XMoveWindow(display.get(), win,x,y);
	XRaiseWindow(display.get(),win);
}

bool GLXWindow::process_events()
{
#ifdef GLXWINDOW_USING_GLOBAL_MUTEX
	yuri::lock_t l(global_mutex);
#else
	yuri::lock_t l(local_mutex);
#endif
	assert(display);
	assert(win);
	if (XCheckWindowEvent(display.get(),win,StructureNotifyMask|KeyPressMask
			|KeyReleaseMask,&xev))
	{
		switch (xev.type) {
		case DestroyNotify:
			log[log::debug] << "DestroyNotify received" << std::endl;
			request_end();
			//parent->stop();
			break;
		case ConfigureNotify:
			if (resize(xev.xconfigure.width,xev.xconfigure.height))
				glViewport(0,0,xev.xconfigure.width,xev.xconfigure.height);
			break;
		case KeyPress:
			log[log::debug] << "Key " << do_get_keyname(xev.xkey.keycode) << " (" <<
				xev.xkey.keycode << ") pressed" <<std::endl;
			keys[xev.xkey.keycode]=true;
			// TODO: need to reenable this again!
			//if (keyCallback) keyCallback->run(&xev.xkey.keycode);
			//if (xev.xkey.keycode==9) request_end();
			break;
		case KeyRelease:
			log[log::debug] << "Key " << do_get_keyname(xev.xkey.keycode) << " (" <<
				xev.xkey.keycode << ") released" <<std::endl;
			keys[xev.xkey.keycode]=false;
			break;
		}
		return true;
	}
	return false;
}

bool GLXWindow::resize(unsigned int w, unsigned int h)
{
	if (w!=(unsigned int)width || h!=(unsigned int)height) {
		log[log::debug] << "Window size changed! " << width << "x" << height <<
			" -> " << w << "x" << h << std::endl;
		width=w;
		height=h;
		return true;
	}
	return false;

}

std::string GLXWindow::get_keyname(int key)
{
#ifdef GLXWINDOW_USING_GLOBAL_MUTEX
	yuri::lock_t l(global_mutex);
#else
	yuri::lock_t l(local_mutex);
#endif
	return do_get_keyname(key);
}
std::string GLXWindow::do_get_keyname(int key)
{
	assert(display);
	assert(win);
	std::string keyname = XKeysymToString(XkbKeycodeToKeysym(display.get(), key, 0, 0));
	//std::string keyname = XKeysymToString(XKeycodeToKeysym(display.get(), key, 0));
	return keyname;
}

bool GLXWindow::check_key(int keysym)
{
	yuri::lock_t l(keys_lock);
	assert(display);
	assert(win);
	if (keys.find(keysym)==keys.end()) return false;
	return keys[keysym];
}

void GLXWindow::exec(core::pCallback c)
{
	yuri::lock_t l(keys_lock);
	assert(display);
	assert(win);
	if (c) c->run(get_this_ptr());
}

bool GLXWindow::have_stereo()
{
	return use_stereo;
}

void GLXWindow::add_used_context(GLXContext ctx,shared_ptr<GLXWindow> win)
{
	yuri::lock_t l(contexts_map_mutex);
	used_contexts[ctx]=win;
}
void GLXWindow::remove_used_context(GLXContext ctx)
{
	yuri::lock_t l(contexts_map_mutex);
	used_contexts.erase(ctx);
}
bool GLXWindow::is_context_used(GLXContext ctx)
{
	yuri::lock_t l(contexts_map_mutex);
	return (bool)(used_contexts.find(ctx) != used_contexts.end());
}

bool GLXWindow::set_vsync(bool state)
{
#ifdef YURI_HAVE_vsync
#ifdef __linux__
#ifdef GLX_SGI_swap_control
	glXSwapIntervalSGI(state?1:0);
	return true;
#else
	return false;
#endif
#else
	return false;
#endif
#else
	(void)state;
	return false;
#endif
}
} // End of graphics
} // End of yuri

//End of File
