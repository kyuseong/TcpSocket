#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/bounded_fifo_queue.hpp>
#include <boost/log/sinks/drop_on_overflow.hpp>

namespace Idea
{

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace lv = boost::log::trivial;
namespace attrs = boost::log::attributes;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

class Logger : public src::severity_logger<logging::trivial::severity_level>
{
	typedef sinks::asynchronous_sink< sinks::text_file_backend, sinks::bounded_fifo_queue<100000, sinks::drop_on_overflow> > text_sink;
	boost::shared_ptr< text_sink > sink_;
	std::string filter_;
public:
	Logger(const std::string& filter);
	~Logger();

	bool setup(const std::string& path, logging::trivial::severity_level sevLevel);
};

extern Logger logger;

#define LOG_INIT(sv) logger.setup("nplib_log", sv )
#define LIB_LOG(sv) BOOST_LOG_SEV(logger, sv)	

//#define LIB_LOG(sv) __noop


}