#pragma once
#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/filesystem.hpp>
#include <boost/preprocessor.hpp>
#include <boost/vmd/vmd.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace fs = boost::filesystem;

extern boost::log::sources::severity_logger< boost::log::trivial::severity_level > g_lg;
#define PY_TRACE BOOST_LOG_SEV(g_lg, boost::log::trivial::trace)
#define PY_DEBUG BOOST_LOG_SEV(g_lg, boost::log::trivial::debug)
#define PY_INFO  BOOST_LOG_SEV(g_lg, boost::log::trivial::info)
#define PY_WARN  BOOST_LOG_SEV(g_lg, boost::log::trivial::warning)
#define PY_ERROR BOOST_LOG_SEV(g_lg, boost::log::trivial::error)
#define PY_FATAL BOOST_LOG_SEV(g_lg, boost::log::trivial::fatal)
#define DATA(r, symbol, elem) symbol elem
#define BOOST_FORMAT(f, ...) boost::format(f) BOOST_PP_SEQ_FOR_EACH(DATA, % , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define LOGT(f, ...) \
PY_TRACE << BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__), f, BOOST_FORMAT(f, __VA_ARGS__)) ;
#define LOGD(f, ...) \
PY_DEBUG << BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__), f, BOOST_FORMAT(f, __VA_ARGS__)) ;
#define LOGI(f, ...) \
PY_INFO << BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__), f, BOOST_FORMAT(f, __VA_ARGS__)) ;
#define LOGW(f, ...) \
PY_WARN << BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__), f, BOOST_FORMAT(f, __VA_ARGS__)) ;
#define LOGE(f, ...) \
PY_ERROR << BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__), f, BOOST_FORMAT(f, __VA_ARGS__)) ;
#define LOGF(f, ...) \
PY_FATAL << BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__), f, BOOST_FORMAT(f, __VA_ARGS__)) ;

#define LOG(f, ...) \
std::cout << BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__), f, BOOST_FORMAT(f, __VA_ARGS__)) << std::endl;

struct Util
{
    Util(std::string arg0 = "");
    std::string byte2str(const uint8_t *bytes, int size);
private:
    void init_log();
    fs::path log_path_;
};
 