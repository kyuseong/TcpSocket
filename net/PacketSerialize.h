#pragma once

#include <map>
#include <iostream>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/thread.hpp>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/atomic.hpp>
#include <boost/uuid/random_generator.hpp> 

#include "types.h"
#include "TcpConnection.h"

//#include "protocol/output/nspacket.pb.h"
//#include "protocol/output/config.pb.h"

namespace Idea 
{
	//bool serialize( const std::shared_ptr<ns::nspacket>& packet, buffer_ptr_t& buffer);
	bool serialize_header(const buffer_ptr_t& payload_buffer, buffer_ptr_t& buffer );

	//bool deserialize(const buffer_ptr_t& buffer, std::shared_ptr<ns::nspacket>& packet);
}