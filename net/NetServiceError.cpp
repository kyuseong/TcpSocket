#include "stdafx.h"
#include "NetServiceError.h"

namespace Idea
{


const char* socket_error_category::name() const noexcept
{ 
	return "Ns"; 
}

boost::system::error_condition socket_error_category::default_error_condition (int ev) const noexcept
{
	if( ev == netservice_error_success)
	{
		return boost::system::errc::success;;
	}
	return boost::system::error_condition(ev, *this);
}
  
std::string socket_error_category::message(int ev) const
{
	return std::to_string(ev);
}

socket_error_category error_catg;


boost::system::error_code make_error_code(netservice_error e )
{
	return boost::system::error_code(static_cast<int>(e), error_catg);
}

boost::system::error_condition make_error_condition (netservice_error e)
{
    return boost::system::error_condition(static_cast<int>(e), error_catg);
}

}