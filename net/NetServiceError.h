#pragma once

#include <boost/system/error_code.hpp>
#include <boost/type_traits.hpp>

#include "NetServiceDelegate.h"

namespace Idea 
{

enum netservice_error {
	netservice_error_success = 0,
	netservice_error_system = 1,
	netservice_error_receive_timeout = 2,
	netservice_error_too_big = 3,
	netservice_error_serialzie_failed = 4,
	netservice_error_deserialzie_failed = 5,
	netservice_error_over_send_que = 6,
	netservice_error_invalid_seq_number = 7
};

class socket_error_category : public boost::system::error_category 
{
public:
	virtual const char* name() const noexcept;
	virtual std::string message(int ev) const;
	virtual boost::system::error_condition default_error_condition (int ev) const noexcept;
};

boost::system::error_condition make_error_condition (netservice_error e);
boost::system::error_code make_error_code(netservice_error e );


}