/*!
 * @file 		ThreadChild.cpp
 * @author 		Zdenek Travnicek
 * @date 		24.7.2010
 * @date		16.2.2013
 * @copyright	Institute of Intermedia, 2010 - 2013
 * 				Distributed under GNU Public License 3.0
 *
 */

#include "ThreadChild.h"

namespace yuri
{
namespace threads
{
	 
ThreadChild::ThreadChild(shared_ptr<boost::thread> thread,
		shared_ptr<ThreadBase> child,bool spawned)
	:thread_ptr(thread),thread(child),finished(false),spawned(spawned)
{
}

ThreadChild::~ThreadChild()
{
}

}
}

// End of file
