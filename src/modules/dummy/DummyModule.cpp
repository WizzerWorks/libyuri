/*!
 * @file 		DummyModule.cpp
 * @author 		Zdenek Travnicek
 * @date 		11.2.2013
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "DummyModule.h"
#include "yuri/config/RegisteredClass.h"
namespace yuri {
namespace dummy_module {

REGISTER("dummy",DummyModule)

IO_THREAD_GENERATOR(DummyModule)

// So we can write log[info] instead of log[log::info]
using namespace yuri::log;

shared_ptr<config::Parameters> DummyModule::configure()
{
	shared_ptr<config::Parameters> p = io::BasicIOThread::configure();
	p->set_description("Dummy module. For testing only.");
	(*p)["size"]["Set size of ....  (ignored ;)"]=666;
	(*p)["name"]["Set name"]=std::string("");
	p->set_max_pipes(1,1);
	return p;
}


DummyModule::DummyModule(log::Log &log_,io::pThreadBase parent,config::Parameters &parameters):
io::BasicIOThread(log_,parent,1,1,std::string("dummy"))
{
	IO_THREAD_INIT("Dummy")
	if (!dummy_name.empty()) log[info] << "Got name " << dummy_name <<"\n";
}

DummyModule::~DummyModule()
{
}

bool DummyModule::step()
{
	io::pBasicFrame frame = in[0]->pop_frame();
	if (frame) {
		yuri::format_t fmt = frame->get_format();
		if (io::BasicPipe::get_format_group(fmt)==YURI_TYPE_VIDEO) {
			push_raw_video_frame(0, frame);
		}
	}
	return true;
}
bool DummyModule::set_param(config::Parameter& param)
{
	if (param.name == "name") {
		dummy_name = param.get<std::string>();
	} else return io::BasicIOThread::set_param(param);
	return true;
}

} /* namespace dummy_module */
} /* namespace yuri */
