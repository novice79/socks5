
#include "util.h"

boost::log::sources::severity_logger< boost::log::trivial::severity_level > g_lg;
using namespace std;
Util::Util(std::string arg0)
{
	if( arg0.empty() )
	{
		log_path_ = "log";
	}
	else
	{
		auto path = fs::canonical(fs::path(arg0).remove_filename());
		log_path_ = fs::system_complete(path) / "log";
	}
	if( !fs::exists(log_path_) ) fs::create_directory(log_path_);
	init_log();
}
void Util::init_log()
{
	namespace logging = boost::log;
	namespace src = boost::log::sources;
	namespace expr = boost::log::expressions;
	namespace sinks = boost::log::sinks;
	namespace keywords = boost::log::keywords;
	logging::add_file_log
    (
        keywords::file_name = log_path_ / "socks5_%Y-%m-%d.log",
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		keywords::auto_flush = true,
        keywords::format = "[%TimeStamp%]:<%Severity%> %Message%",
        keywords::max_files = 5
    );
    // add console sink
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();
    sink->locked_backend()->add_stream(
        boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter()) );

    // flush
    sink->locked_backend()->auto_flush(true);

    // format sink
    sink->set_formatter
    (
        expr::format("[%1%]: <%2%> %3%")
            % expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
            % logging::trivial::severity
            % expr::smessage
        // expr::stream
        //     << "["
        //     << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
        //     << "]: <" << logging::trivial::severity
        //     << "> " << expr::smessage
    );
    // register sink
    logging::core::get()->add_sink(sink);
	// enum severity_level
	// {
	// 	trace,
	// 	debug,
	// 	info,
	// 	warning,
	// 	error,
	// 	fatal
	// };
	static map<string, logging::trivial::severity_level> log_level_map = {
		{ "trace", logging::trivial::trace },
		{ "debug", logging::trivial::debug },
		{ "info", logging::trivial::info },
		{ "warning", logging::trivial::warning },
		{ "error", logging::trivial::error },
		{ "fatal", logging::trivial::fatal }
	};
	logging::trivial::severity_level ll = logging::trivial::warning;
	auto log_level = std::getenv("LOG");
	if(log_level)
	{
		auto it = log_level_map.find(log_level);
		if(it != log_level_map.end())
		{
			ll = it->second;
		}
	}
	logging::core::get()->set_filter
	(
		logging::trivial::severity >= ll
	);
	logging::add_common_attributes();
}
std::string Util::byte2str(const uint8_t *bytes, int size)
{
	static char const hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	std::string str;
	for (int i = 0; i < size; ++i)
	{
		const char ch = bytes[i];
		str.append(&hex[(ch & 0xF0) >> 4], 1);
		str.append(&hex[ch & 0xF], 1);
	}
	return str;
}
