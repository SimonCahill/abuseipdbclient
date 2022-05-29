/**
 * @file Main.cpp
 * @author Simon Cahill (simon@simonc.eu)
 * @brief Contains the entry point of this application.
 * @version 0.1
 * @date 2022-05-28
 * 
 * @copyright Copyright (c) 2022 Simon Cahill
 */

///////////////////////
//  SYSTEM INCLUDES  //
///////////////////////
// stl
#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

// curl
#include <curl/curl.h>

// C
#include <signal.h>

// json
#include <nlohmann/json.hpp>

// spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>

///////////////////////
//  LOCAL  INCLUDES  //
///////////////////////
#include "resources/Args.hpp"
#include "api/AbuseIpDbApi.hpp"

using spdlog::level::level_enum;
using spdlog::logger;

using std::chrono::milliseconds;
using std::error_code;
using std::exception;
using std::for_each;
using std::ifstream;
using std::make_shared;
using std::ofstream;
using std::shared_ptr;
using std::string;
using std::this_thread::sleep_for;
using std::vector;

///////////////////////
//     CONSTANTS     //
///////////////////////
#ifdef abuseipdb_DEBUG
    const static level_enum LOG_LEVEL = level_enum::trace;
    #warning "Running in debug mode!"
#else
    const static level_enum LOG_LEVEL = level_enum::info;
#endif // ipreporter_DEBUG

///////////////////////
//  GLOBAL VARIABLES //
///////////////////////
static shared_ptr<logger>   g_logger;

bool parseArgs(const int32_t, const char**);
void setupLogging();
void setupSignals();

int32_t main(const int32_t argc, const char** argv) {

    using abuseipdb_client::api::AbuseIpDbApi;
    using cat_t = abuseipdb_client::api::AbuseIpDbApi::ReportCategories;

    setupLogging();

    g_logger->debug("Setting up API connector...");
    auto apiConnector = AbuseIpDbApi::Factory("38dd2247858867262086cc640c76550bd9c9f5a2ba427eca7b4ca6f9c7e8164c559cd158451b1af3", g_logger)
                        .getInstance();
    

    return 0;
}

bool parseArgs(const int32_t argc, const char** argv) {

    int32_t arg = 0;

    do {
        int32_t argvIndex = optind ? optind : 1;
        int32_t optionIndex = 0;

    } while (arg >= 0);

    return true; // keep alive
}

void setupLogging() {
    int32_t syslogOptions = LOG_PID;
    vector<spdlog::sink_ptr> sinks = {
        make_shared<spdlog::sinks::syslog_sink_st>("abuseipdb", syslogOptions, LOG_PID, true),
        make_shared<spdlog::sinks::stdout_color_sink_st>(spdlog::color_mode::always)
    };

    // set log level for each sink
    for_each(sinks.begin(), sinks.end(), [&](spdlog::sink_ptr x) {
        x->set_level(LOG_LEVEL);
        x->set_pattern("[%Y-%m-%d] [%H:%M:%S] [%^%l%$] %v");
    });
    g_logger = make_shared<spdlog::logger>("ipdbreporter", sinks.begin(), sinks.end());
    g_logger->set_level(LOG_LEVEL);
}
