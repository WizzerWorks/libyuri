/*
 * RegisteredClass.h
 *
 *  Created on: Jul 24, 2010
 *      Author: worker
 */

#ifndef REGISTEREDCLASS_H_
#define REGISTEREDCLASS_H_

#include <map>
#include <string>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include "yuri/config/config_common.h"
#include "yuri/io/BasicIOThread.h"
#include "yuri/exception/Exception.h"
#include "yuri/config/Parameters.h"
#include "yuri/config/Instance.h"

#ifdef YURI_MODULE_IN_TREE
#define REGISTER(name,classname) namespace { yuri::config::RegisteredClassSpecialized<classname>  __reg__ (name); }
#else
#define REGISTER(name,classname) \
extern "C" { \
std::string yuri_module_get_name() {return name;}\
void 		yuri_module_register() {yuri::config::RegisteredClassSpecialized<classname>  *__reg__ = new yuri::config::RegisteredClassSpecialized<classname>(name);(void)__reg__;}\
}
#endif
namespace yuri {
namespace config {

using namespace std;
using namespace yuri::io;
using namespace yuri::exception;
using namespace yuri::threads;
using boost::mutex;


class Instance;
class RegisteredClass {
public:
	typedef map<string,shared_ptr<RegisteredClass> > register_map_t;
	RegisteredClass(string id) throw (Exception);
	virtual ~RegisteredClass();

	static bool is_registered(string id);
	static shared_ptr<vector<string> > list_registered();
	static shared_ptr<Parameters> get_params(string id) throw (Exception);
	static shared_ptr<Instance> prepare_instance(string id);
	static shared_ptr<Instance> get_converter(long format_in, long format_out);

	static void reload_converters();
	static map<pair<long,long>, vector<shared_ptr<Converter > > > get_all_converters();
	static vector<shared_ptr<Converter > > get_converters(long in, long out);
	static vector<shared_ptr<Converter > > get_converters(pair<long, long> fmts);

	virtual shared_ptr<Instance> _prepare_instance() = 0;
	virtual shared_ptr<Parameters> _get_params() = 0;
	virtual shared_ptr<Instance> _prepare_converter(shared_ptr<Instance> inst,
			long format_in, long format_out) = 0;
	static bool load_module(string path);
protected:
	static register_map_t *registeredClasses;
	static void add_to_register(string id, RegisteredClass*r) throw (Exception);
	static bool do_is_registered(string id);
	static shared_ptr<vector<string> > do_list_registered();
	static shared_ptr<Parameters> do_get_params(string id) throw (Exception);
	static shared_ptr<Instance> do_prepare_instance(string id);
	static void do_add_to_register(string id, RegisteredClass*r) throw (Exception);
	static map<pair<long,long>, vector<shared_ptr<Converter> > > converters;
	static mutex converters_mutex;
	static mutex *reg_mutex;
	static void do_reload_converters();
	static bool conv_sort(shared_ptr<Converter> a, shared_ptr<Converter> b);
	string id;
	shared_ptr<Parameters> params;
	//generator_t generator;
	//configurator_t configurator;

};

template<class T> class RegisteredClassSpecialized:public RegisteredClass {
public:
	RegisteredClassSpecialized(string id) throw (Exception):
		RegisteredClass(id) {}

	typedef T type_id;
protected:
	virtual shared_ptr<Instance> _prepare_instance();
	virtual shared_ptr<Parameters> _get_params();
	virtual shared_ptr<Instance> _prepare_converter(shared_ptr<Instance> inst,
			long format_in, long format_out);
};

template<class T> shared_ptr<Parameters> RegisteredClassSpecialized<T>::_get_params()
{
	if (params) return params;
	return params = type_id::configure();
}

template<class T> shared_ptr<Instance> RegisteredClassSpecialized<T>::_prepare_instance()
{
	if (!_get_params()) throw InitializationFailed("Failed to initialize parameters");
	shared_ptr<Instance> inst (new Instance(id,type_id::generate,params));
	return inst;
}

template<class T> shared_ptr<Instance> RegisteredClassSpecialized<T>::_prepare_converter(shared_ptr<Instance> inst,
		long format_in, long format_out)
{
	if (!type_id::configure_converter(*(inst->params),format_in,format_out))
		throw NotImplemented();
	return inst;
}

}

}

#endif /* REGISTEREDCLASS_H_ */

