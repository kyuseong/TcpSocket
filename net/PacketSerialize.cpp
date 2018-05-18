#include "stdafx.h"
#include "PacketSerialize.h"

#include <assert.h>
#include <vector>

#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>
#include <boost/uuid/uuid.hpp> 
#include <boost/lexical_cast.hpp> 
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "Logger.h"
//#include "Crypt.h"

namespace Idea {
/*
bool is_validate_packet( std::shared_ptr<ns::nspacket> packet )
{
	if (!packet->has_id())
	{
		return false;
	}
		
	if (!packet->has_payload())
	{
		return false;
	}

	int length = packet->ByteSize();
	if (length > ns::MESSAGE_PAYLOAD)
	{
		return false;
	}
	return true;
}*/

/*
bool serialize( const std::shared_ptr<ns::nspacket>& packet, buffer_ptr_t& buffer)
{
	int length = packet->ByteSize();
	if (length > ns::MESSAGE_PAYLOAD)
	{
		return false;
	}

	buffer = boost::make_shared<std::vector<char>>(length);
	if (!packet->SerializeToArray(buffer->data(), length))
	{
		return false;
	}

	return true;
}*/

// packet -> buffer 
bool serialize_header(const buffer_ptr_t& payload_buffer, buffer_ptr_t& buffer )
{
	int payload_length = (int)payload_buffer->size();
	if (payload_length > MESSAGE_PAYLOAD)
	{
		return false;
	}

	int be_length = htonl(payload_length);
	int header_length = sizeof(be_length);

	buffer = std::make_shared<std::vector<char>>(header_length + payload_length);

	memcpy(buffer->data(), &be_length, header_length);
	memcpy(buffer->data() + header_length, payload_buffer->data(), payload_length);

	return true;
}
//
//bool deserialize(const buffer_ptr_t& buffer, std::shared_ptr<ns::nspacket>& packet)
//{
//	buffer_ptr_t plain = boost::make_shared<std::vector<char>>();
//	packet = boost::make_shared<ns::nspacket>();
//	if (!packet->ParseFromArray(buffer->data(), buffer->size()))
//	{
//		return false;
//	}
//
//	return true;
//}

}