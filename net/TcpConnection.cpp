#include "stdafx.h"
#include "TcpConnection.h"

#include <boost/bind.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/detail/endian.hpp>
#include <boost/make_shared.hpp>

#include "Logger.h"
#include "PacketSerialize.h"
#include "Crypt.h"
#include "NetServiceError.h"

//#include "..\protocol\output\error.pb.h"


namespace Idea
{

TcpConnection::TcpConnection( boost::asio::io_service& io_service, NetServiceDelegate* event_listener/*, MessageHandler* message_handler */, const server_config& config)
:	io_service_(io_service),
	socket_(io_service), 
	event_listener_(event_listener), 
	timer_(io_service), 
	config_(config),
	strand_(io_service),
	id_(0),
	receive_payload_length_(0),
	receive_payload_buffer_( std::make_shared<std::vector<char>>(nspacket_define::MESSAGE_PAYLOAD) ),
	send_seq_(1),
	receive_seq_(1)
{
	LIB_LOG(lv::trace) << "[TcpConnection::TcpConnection] - id : " << id();
}

TcpConnection::~TcpConnection()
{
	LIB_LOG(lv::trace) << "[TcpConnection::~TcpConnection] - id : " << id();
}

bool TcpConnection::connect()
{
	boost::asio::ip::tcp::endpoint endpoint;
	endpoint.address(boost::asio::ip::address::from_string(config_.ip()));
	endpoint.port((uint16)config_.port());
	socket_.async_connect( endpoint, strand_.wrap(boost::bind(&TcpConnection::completed_connected, shared_from_this(), boost::asio::placeholders::error)));

	LIB_LOG(lv::trace) << "[TcpConnection::connect] - id : " << id();

	return true;
}

void TcpConnection::close(bool gracefully)
{
	LIB_LOG(lv::trace) << "[TcpConnection::close] - id : " << id();

	timer_.cancel();
	boost::system::error_code error;
	if (true == gracefully)
	{
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, error);
		if (error)
		{
			LIB_LOG(lv::trace) << "[TcpConnection::close] - shutdown failed - id : " << id() << ",error : " << error;
		}
	}
	socket_.close(error);
	if (error)
	{
		LIB_LOG(lv::trace) << "[TcpConnection::close] - close failed - id : " << id() << ",error : " << error;
		return;
	}
}

void TcpConnection::id(int64 id)
{
	id_ = id;
}

int64 TcpConnection::id()
{
	return id_;
}

std::size_t TcpConnection::send_que_size()
{
	std::size_t size = send_que.size();

	return size;
}

std::string TcpConnection::remote_address()
{
	LIB_LOG( lv::trace) << "[TcpConnection::remote_address] - id : " << id();

	std::string remote_address;
	try
	{
		boost::asio::ip::tcp::endpoint remoteEndPoint = socket_.remote_endpoint();
		remote_address = remoteEndPoint.address().to_string();
	}
	catch (std::exception&)
	{
		remote_address = "";
	}
	return remote_address;
}

boost::asio::ip::tcp::socket& TcpConnection::socket()
{
	return socket_;
}
//
//void TcpConnection::send_packet(std::shared_ptr<ns::nspacket> packet_ptr)
//{
//	LIB_LOG( lv::trace) << "[TcpConnection::send_packet] - id : " << id();
//
//	strand_.post( boost::bind(&TcpConnection::send_packet_lock, shared_from_this(), packet_ptr) );
//}

void TcpConnection::send_buffer( buffer_ptr_t buffer_ptr )
{
	LIB_LOG( lv::trace) << "[TcpConnection::send_buffer] - id : " << id();

	strand_.post( boost::bind(&TcpConnection::send_buffer_lock, shared_from_this(), buffer_ptr) );
}

//
//void TcpConnection::send_message(const ::google::protobuf::Message& message)
//{
//	LIB_LOG( lv::trace) << "[TcpConnection::send_message] - id : " << id();
//	std::shared_ptr<ns::nspacket> packet = boost::make_shared<ns::nspacket>();
//	
//	const google::protobuf::Reflection* reflection = message.GetReflection();
//	const google::protobuf::Descriptor* descriptor = message.GetDescriptor();	
//	const google::protobuf::FieldDescriptor* id_field_descriptor = descriptor->FindFieldByName("id");
//	if( id_field_descriptor == 0)
//		return;
//
//	int id = 0;
//	if( id_field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_ENUM )
//	{
//		id = reflection->GetEnum(message, id_field_descriptor)->number();
//	}
//	else
//	{
//		id = reflection->GetInt32( message, id_field_descriptor );
//	}
//
//	packet->set_id(id);
//	message.AppendToString( packet->mutable_payload() );
//
//	send_packet( packet );
//}
//
//void TcpConnection::send_packet_lock(std::shared_ptr<ns::nspacket> packet_ptr)
//{
//	LIB_LOG( lv::trace) << "[TcpConnection::send_packet_lock] - id : " << id();
//
//	packet_ptr->set_seq( send_seq_++ );
//
//	buffer_ptr_t buffer_ptr;
//	if (!serialize(packet_ptr, buffer_ptr) )
//	{
//		LIB_LOG( lv::error) << "[TcpConnection::send_packet_lock] - serialize failed. - id:" << id();
//		set_last_error( make_error_code( ns::error::ns_error_serialzie_failed ) );
//		close(false);
//		return;
//	}
//	
//	send_buffer_lock(buffer_ptr);
//}

void TcpConnection::send_buffer_lock( buffer_ptr_t plain_ptr )
{
	buffer_ptr_t cipher_payload_ptr;
	if( !encrypt( plain_ptr, cipher_payload_ptr ) )
	{
		LIB_LOG( lv::error) << "[TcpConnection::send_packet_lock] - encrypt failed. - id:" << id();
		set_last_error( make_error_code(netservice_error_serialzie_failed ) );
		close(false);
		return;
	}

	buffer_ptr_t buffer_ptr;
	if(!serialize_header( cipher_payload_ptr, buffer_ptr ) )
	{
		LIB_LOG( lv::error) << "[TcpConnection::send_packet_lock] - serialize_header failed. - id:" << id();
		set_last_error( make_error_code(netservice_error_serialzie_failed ) );
		close(false);
		return;
	}

	LIB_LOG( lv::trace) << "[TcpConnection::send_buffer_lock] - id " << id() << ",length:" << buffer_ptr->size();
	send_que.push_back(buffer_ptr);

	// no send in progress
	if ( send_que.size() > 1 )
		return;

	post_send();
}

void TcpConnection::post_send()
{
	LIB_LOG( lv::trace) << "[TcpConnection::post_send] - id:" << id() << ",size:" << send_que.size();
	if ( send_que.empty() )
		return;

	std::vector<boost::asio::const_buffer> asio_buffer;
	size_t total_buffer_size= 0;
	for(auto& v : send_que)
	{
		// check payload size
		size_t buffer_size = v->size();
		if ( ( total_buffer_size + buffer_size ) > MESSAGE_PAYLOAD )
			break;

		total_buffer_size += buffer_size;
		asio_buffer.push_back(boost::asio::buffer(v->data(), v->size()));
	}

	LIB_LOG(lv::trace) << "[TcpConnection::post_send] - post send -  id:" << id() << ",total_buffer_size:" << total_buffer_size;

	size_t posted_count = asio_buffer.size();
	boost::asio::async_write(socket_,
		asio_buffer,
		strand_.wrap(boost::bind(&TcpConnection::completed_sent, shared_from_this(), posted_count, boost::asio::placeholders::error)));
}

void TcpConnection::completed_sent(size_t sent, const boost::system::error_code& error)
{
	LIB_LOG(lv::trace) << "[TcpConnection::completed_sent] - id:" << id() << ",sent:" << sent;
	if (error)
	{
		return;
	}
	
	for(int i=0;i<sent;++i)
	{
		send_que.pop_front();
	}

	post_send();
}

void TcpConnection::post_receive()
{
	LIB_LOG(lv::trace) << "[TcpConnection::post_receive] - id : " << id();

	boost::asio::async_read(socket_,
		boost::asio::buffer(&receive_payload_length_, sizeof(receive_payload_length_)),
		strand_.wrap(boost::bind(&TcpConnection::completed_received_header, shared_from_this(), boost::asio::placeholders::error)));
}

void TcpConnection::completed_received_header( const boost::system::error_code& error )
{
	LIB_LOG( lv::trace) << "[TcpConnection::completed_received_header] - begin - id:" << id() << ", error:" << error;
	if (error)
	{
		LIB_LOG( lv::error ) << "[TcpConnection::completed_received_header] - error  - id:" << id() << ", error : " << error;
		completed_closed(error);
		return;
	}
	receive_payload_length_ = ntohl(receive_payload_length_);
	if (receive_payload_length_ > MESSAGE_PAYLOAD)
	{
		LIB_LOG( lv::error ) << "[TcpConnection::completed_received_header] - too big length - id:" << id() << ", error : " << error;
		close(false);
		completed_closed(error);
		return;
	}

	reset_last_receive_time();
	boost::asio::async_read(socket_,
		boost::asio::buffer(&receive_payload_buffer_->front(), receive_payload_length_),
		strand_.wrap(boost::bind(&TcpConnection::completed_received_payload, shared_from_this(), boost::asio::placeholders::error)));
}

void TcpConnection::completed_received_payload( const boost::system::error_code& error )
{
	LIB_LOG( lv::trace) << "[TcpConnection::completed_received_payload] - id:" << id() << ", error : " << error;
	if (error)
	{
		LIB_LOG( lv::trace) << "[TcpConnection::completed_received_payload] - error - id:" << id() << ", error : " << error;
		completed_closed(error);
		return;
	}
	
	bool invoke_handle_packet = true;

	buffer_ptr_t plain;
	if (!decrypt(receive_payload_buffer_, receive_payload_length_, plain))
	{
		LIB_LOG(lv::error) << "[TcpConnection::completed_received_payload] - deserialize_packet failed - id:" << id() << ", error : ";
		close(false);
		completed_closed(make_error_code(netservice_error_deserialzie_failed));
		return;
	}

	if( event_listener_ != NULL ) {
		if( event_listener_->handle_receive( id(), plain, plain->size()) )
		{
			invoke_handle_packet = false;
		}
	}
/*
	if( invoke_handle_packet )
	{
		
		std::shared_ptr<ns::nspacket> packet;
		if( !deserialize( plain, packet ) )
		{
			LIB_LOG( lv::error) << "[TcpConnection::completed_received_payload] - deserialize_packet failed - id:" << id() << ", error : ";
			close(false);
			completed_closed( make_error_code( ns::error::ns_error_deserialzie_failed) );
			return;
		}
		
		if( packet->seq() != receive_seq_++ )
		{
			LIB_LOG( lv::error) << "[TcpConnection::completed_received_payload] - deserialize_packet failed - id:" << id() << ", error : ";
			close(false);
			completed_closed( make_error_code( ns::error::ns_error_invalid_seq_number) );
		}
		
		bool invoke_handle_message = true;
		if( event_listener_ != NULL ) {
			if( event_listener_->handle_receive_packet( id(), packet) )
				invoke_handle_message = false;
		}

		if( invoke_handle_message )
		{
			message_handler_->process(id(), packet);
		}
	}*/

	reset_last_receive_time();

	post_receive();
}

void TcpConnection::completed_closed( const boost::system::error_code& error )
{
	LIB_LOG( lv::trace) << "[TcpConnection::completed_closed] - id : " << id() << ", error_code : " << error;
	set_last_error(error);	

	timer_.cancel();
	if( event_listener_ != NULL )
		event_listener_->handle_closed( id(), last_error_);
}

void TcpConnection::accpeted(const boost::system::error_code& error)
{
	LIB_LOG( lv::trace) << "[TcpConnection::accpeted] - id : " << id() << ", error : " << error;

	completed_connected(error);
}

void TcpConnection::completed_connected(const boost::system::error_code& error)
{
	LIB_LOG( lv::trace) << "[TcpConnection::completed_connected] - id : " << id() << ", error : " << error;
	event_listener_->handle_connected( id(), remote_address(),  error);
	if (error)
	{
		return;
	}

	reset_last_receive_time();
	post_timer();
	post_receive();
}

void TcpConnection::completed_timer(const boost::system::error_code& error)
{
	LIB_LOG( lv::trace) << "[TcpConnection::completed_timer] - id : " << id() << ", error:" << error;
	if (error)
	{
		LIB_LOG( lv::trace) << "[TcpConnection::completed_timer] - error - id: " << id() << ", error : " << error;
		return;
	}
	if (!socket_.is_open())
	{
		return;
	}

	post_timer();
	check_receive_timeout();
}

bool TcpConnection::check_receive_timeout()
{
	// receive timeout
	std::chrono::steady_clock::time_point now =  std::chrono::steady_clock::now();
	std::chrono::duration<double> receive_left_time = now - last_receive_time();
	std::chrono::duration<double> timeoutsec = std::chrono::seconds(config_.receive_timeout());
	if (receive_left_time > timeoutsec)
	{
		LIB_LOG( lv::warning) << "[TcpConnection::check_receive_timeout] - receive timeout connection - id:" << id() << ", receive_left_time: " << receive_left_time.count();
		set_last_error( make_error_code(netservice_error_receive_timeout) );
		close(false);
		return true;
	}

	if (send_que_size() > config_.max_send_queue_size() )
	{
		LIB_LOG( lv::warning) << "[TcpConnection::check_receive_timeout] - over send queue size - id : " << id() << ", size: " << send_que_size();
		set_last_error( make_error_code(netservice_error_over_send_que) );
		close(false);
		return true;
	}
	return false;
}


void TcpConnection::reset_last_receive_time()
{
	last_receive_time_ = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point TcpConnection::last_receive_time()
{
	return last_receive_time_;
}

void TcpConnection::post_timer()
{
	LIB_LOG( lv::trace) << "[TcpConnection::post_timer] - id : " << id();

	timer_.expires_from_now(std::chrono::seconds( config_.receive_timeout_intval() ));
	timer_.async_wait(strand_.wrap(boost::bind(&TcpConnection::completed_timer, shared_from_this(), boost::asio::placeholders::error)));	
}

void TcpConnection::set_last_error(boost::system::error_code error)
{
	if( last_error_ )
		return;

	last_error_ = error;
}

}