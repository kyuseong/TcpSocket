#include "stdafx.h"
#include "NetService.h"

#include <assert.h>
#include <vector>

#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>
#include <boost/uuid/uuid.hpp> 
#include <boost/lexical_cast.hpp> 
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string/case_conv.hpp>


#include "TcpConnection.h"
#include "Logger.h"


namespace Idea {

NetService::NetService(const server_config& config, NetServiceDelegate* listener) :
	config_(config), 
	io_service_(config.thread_pool_count()),
	acceptor_(io_service_), 
	strand_(io_service_), 
	timer_(io_service_),
	event_listener_( listener ),
	top_id_(1)
{
	// listen endpoint
	endpoint_.address(boost::asio::ip::address::from_string(config_.ip()));
	endpoint_.port((uint16)config_.port());
}

NetService::~NetService(void)
{
}

bool NetService::init()
{
	LIB_LOG(lv::info) << "[NetService::init]";

	post_timer();

	return true;
}

void NetService::run_io_worker()
{
	LIB_LOG(lv::info) << "[NetService::run_io_worker]";
	
	work_.reset(new boost::asio::io_service::work(io_service_));
	auto worker = boost::bind(&boost::asio::io_service::run, &io_service_);

	for (int i = 0; i < config_.thread_pool_count(); i++)
	{
		io_thread_pool_.create_thread( worker );
	}
}

void NetService::poll()
{
	io_service_.poll();
}


void NetService::term()
{
	LIB_LOG(lv::info) << "[NetService::term]";

	stop_acceptor();

	boost::system::error_code error;
	timer_.cancel(error);

	close_all();

	work_.reset();

	io_thread_pool_.join_all();
}

bool NetService::close(int64 id)
{
	LIB_LOG(lv::debug) << "[NetService::close] - id:" << id;

	std::lock_guard<std::mutex> guard(mtx_);

	auto itr = connection_map_.find( id );
	if( itr == connection_map_.end() )
	{
		return false;
	}
	auto connection = itr->second.lock();
	if( connection )
	{
		connection->close();
		return true;
	}

	return false;
}

void NetService::close_all()
{
	LIB_LOG(lv::info) << "[NetService::close_all]";

	std::lock_guard<std::mutex> guard(mtx_);

	for (auto& kv : connection_map_)
	{
		if (TcpConnectionPtr connection = kv.second.lock())
		{
			connection->close();
		}
	}
	connection_map_.clear();
}


bool NetService::start_acceptor()
{
	LIB_LOG(lv::info) << "[NetService::start_acceptor]";

	acceptor_.open(endpoint_.protocol());
	boost::asio::socket_base::reuse_address option(true);
	acceptor_.set_option(option);
	acceptor_.bind(endpoint_);
	acceptor_.listen();

	post_accept();

	return true;
}

void NetService::stop_acceptor()
{
	LIB_LOG(lv::info) << "[NetService::stop_acceptor]";

	boost::system::error_code error;
	acceptor_.cancel(error);
	acceptor_.close(error);
}

bool NetService::connect(int64& id)
{
	id = top_id_++;

	LIB_LOG(lv::info) << "[NetService::connect] - id:" << id;

	TcpConnectionPtr connection = std::make_shared<TcpConnection>(io_service_, event_listener_,/* &message_handler_,*/ config_);
	
	connection->id(id);
	TcpConnectionWeakPtr client_wptr(connection);
	bool ret = true;
	{
		std::lock_guard<std::mutex> guard(mtx_);
		auto result = connection_map_.insert(std::make_pair(id, client_wptr));
		ret = result.second;
	}
	
	connection->connect();

	return ret;
}

bool NetService::is_connected(int64 id)
{
	TcpConnectionPtr ptr = get_connection(id);
	return ( ptr != nullptr);
}

TcpConnectionPtr NetService::get_connection(int64 id)
{
	std::lock_guard<std::mutex> guard(mtx_);
	
	auto& itr = connection_map_.find(id);
	if( itr == connection_map_.end() )
		return nullptr;

	return itr->second.lock();
}

//void NetService::send_message(int64 id, const ::google::protobuf::Message& message)
//{
//	TcpConnectionPtr ptr = get_connection(id);
//	if( ptr )
//	{
//		ptr->send_message( message);
//	}
//}
//
//void NetService::send_packet(int64 id, std::shared_ptr<ns::nspacket> packet)
//{
//	TcpConnectionPtr ptr = get_connection(id);
//	if( ptr )
//	{
//		ptr->send_packet( packet);
//	}
//}

// buffer 에 해당하는 내용을 전송한다.
void NetService::send_buffer(int64 id, buffer_ptr_t buffer)
{
	TcpConnectionPtr ptr = get_connection(id);
	if( ptr )
	{
		ptr->send_buffer( buffer );
	}

	return;
}

// accept을 요청함
void NetService::post_accept()
{
	LIB_LOG(lv::trace) << "[NetService::post_accept] - start";

	TcpConnectionPtr connection = std::make_shared<TcpConnection>(io_service_, event_listener_,/* &message_handler_,*/ config_);
	
	int64 id = top_id_++;
	connection->id(id);

	acceptor_.async_accept(connection->socket(), boost::bind(&NetService::completed_accept, this, connection, boost::asio::placeholders::error));
}

// accept 됨
void NetService::completed_accept(TcpConnectionPtr connection, const boost::system::error_code& error)
{
	LIB_LOG(lv::trace) << "[NetService::completed_accept] - start - id:" << connection->id() << ",error:" << error;

	if (error != boost::system::errc::operation_canceled)
	{
		post_accept();
	}
	else if (error)
	{
		LIB_LOG(lv::error) << "[NetService::completed_accept] - error - id:" << connection->id() << ",error:" << error;
		return;
	}

	int64 id = connection->id();
	TcpConnectionWeakPtr connection_wptr(connection);
	bool inserted = true;
	{
		std::lock_guard<std::mutex> guard(mtx_);

		auto result = connection_map_.insert(std::make_pair(id, connection_wptr));
		inserted = result.second;
	}
	if (!inserted)
	{
		LIB_LOG(lv::error) << "[NetService::completed_accept] - insert failed - id: " << id << ",remote address : " << connection->remote_address();
		connection->close(false);
		return;
	}

	connection->accpeted(error);
}

// 주기적으로 처리해야하는 작업을 처리한다.
void NetService::completed_timer(const boost::system::error_code& error)
{
	LIB_LOG(lv::trace) << "[NetService::completed_timer] - start";

	if (error)
	{
		LIB_LOG(lv::error) << "[NetService::completed_timer] - error - code:" << error;
		return;
	}

	post_timer();

	check_zombie_client();
}

void NetService::post_timer()
{
	LIB_LOG(lv::trace) << "[NetService::post_timer] - start";

	timer_.expires_from_now(boost::chrono::seconds(config_.zombie_intval()));
	timer_.async_wait(strand_.wrap(boost::bind(&NetService::completed_timer, this, boost::asio::placeholders::error)));
}

// 이미 접속이 끊어진 클라이언트들을 정리한다.
// map안에는 이미 key/value(connection) value가 null임
void NetService::check_zombie_client()
{
	LIB_LOG(lv::trace) << "[NetService::check_zombie_client] - begin";

	std::lock_guard<std::mutex> guard(mtx_);
	std::vector<int64> eraseList;
	for (auto& kv : connection_map_)
	{
		TcpConnectionPtr connection = kv.second.lock();
		if (!connection)
		{
			eraseList.push_back(kv.first);
			continue;
		}
	}
	
	for (auto& sessionID : eraseList)
	{
		connection_map_.erase(sessionID);
	}

	LIB_LOG(lv::trace) << "[NetService::check_zombie_client] - end - erase:" << eraseList.size();

}

}