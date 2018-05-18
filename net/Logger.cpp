
#include "stdafx.h"
#include "Logger.h"

#include <string>

#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_multifile_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/to_log.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions/predicates/is_in_range.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/log/attributes/current_process_name.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/detail/default_attribute_names.hpp>


namespace Idea
{

Logger logger("");

Logger::Logger(const std::string& filter) : filter_(filter)
{
}


Logger::~Logger(void)
{
	if (nullptr != sink_)
	{
		logging::core::get()->remove_sink(sink_);
		sink_->stop();
		sink_->flush();
		sink_.reset();
	}
}


bool Logger::setup( const std::string& path, logging::trivial::severity_level sevLevel)
{
	boost::filesystem::path logPath;
	if (path.empty())
	{
		logPath = boost::filesystem::current_path();
		logPath /= "/log";
	}
	else
	{
		logPath = path;
	}
	if (true == logPath.has_filename())
	{
		logPath /= "/";
	}
	logging::process_id pid = boost::log::aux::this_process::get_id();

	if (filter_.empty())
	{
		logPath /= (boost::log::aux::get_process_name() + "_%Y-%m-%d_%H-%M-%S.%2N_" + std::to_string(pid.native_id()) + ".log");
	}
	else
	{
		logPath /= filter_ + "_" + (boost::log::aux::get_process_name() + "_%Y-%m-%d_%H-%M-%S.%2N_" + std::to_string(pid.native_id()) + ".log");
	}
	
	boost::shared_ptr< sinks::text_file_backend > backend =
		boost::make_shared< sinks::text_file_backend > (
			keywords::file_name = logPath.string(),
			keywords::rotation_size = 10 * 1024 * 1024,
			keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0)
	);
	backend->auto_flush(true);

	sink_ = boost::make_shared< text_sink >(backend);

	sink_ -> set_formatter
      ( boost::log::expressions::format("%1% %2% %3%\t%4%")
      % boost::log::expressions::format_date_time<boost::posix_time::ptime>
      ( "TimeStamp", "%Y-%m-%d %H:%M:%S.%f" )
      % expr::attr< logging::thread_id>(logging::aux::default_attribute_names::thread_id())
      % logging::trivial::severity
      % boost::log::expressions::message
      );

	if (filter_.empty())
	{
		sink_->set_filter(logging::trivial::severity >= sevLevel && expr::attr<std::string>("prefix") == "global");
	}
	else
	{
		sink_->set_filter(logging::trivial::severity >= sevLevel && expr::attr<std::string>("prefix") == filter_);
	}

	logging::add_common_attributes();
	logging::core::get()->add_global_attribute("Scopes", attrs::named_scope());
	logging::core::get()->add_global_attribute("prefix", attrs::constant< std::string >("global"));
	logging::core::get()->add_sink(sink_);
	return true;
}

}