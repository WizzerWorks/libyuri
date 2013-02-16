/*!
 * @file 		config_common.h
 * @author 		Zdenek Travnicek
 * @date 		25.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2010 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */
/*
 * config_common.h
 *
 *  Created on: Jul 25, 2010
 *      Author: neneko
 */

#ifndef CONFIG_COMMON_H_
#define CONFIG_COMMON_H_
#include "yuri/io/types.h"
#include <yuri/config/Parameters.h>
#include <yuri/io/BasicIOThread.h>

#include <map>
#include <string>

namespace yuri {
typedef shared_ptr<yuri::io::BasicIOThread> (* generator_t)(yuri::log::Log&,yuri::threads::pThreadBase,yuri::config::Parameters& parameters);
typedef shared_ptr<yuri::config::Parameters> (* configurator_t)();
typedef shared_ptr<yuri::io::BasicIOThread> (* converter_t)(yuri::log::Log&,yuri::threads::pThreadBase,long format_in, long format_out, yuri::config::Parameters& parameters);

}
#endif /* CONFIG_COMMON_H_ */
