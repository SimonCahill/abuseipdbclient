/**
 * @file Args.hpp
 * @author Simon Cahill (simon@simonc.eu)
 * @brief Contains the arguments used by this application
 * @version 0.1
 * @date 2022-05-28
 * 
 * @copyright Copyright (c) 2022 Simon Cahill
 */

#ifndef ABUSEIPDB_CLIENT_INCLUDE_RESOUCRS_ARGS_HPP
#define ABUSEIPDB_CLIENT_INCLUDE_RESOUCRS_ARGS_HPP

#include <string>

#include <getopt.h>

#include "cfg/ConfigManager.hpp"
#include "resources/Version.hpp"

namespace abuseipdb_client { namespace resources {

    using cfg::ConfigManager;

    using std::string_view;

    const inline struct option* getApplicationArgs() {
        const static struct option args[] = {
            { "--version",          no_argument,        nullptr, 'v'   },
            { "--help",             no_argument,        nullptr, 'h'   },
            { "--config",           required_argument,  nullptr, 'c'   },
            { "--daemon",           no_argument,        nullptr, 'd'   },
            { "--api-key",          required_argument,  nullptr, 'a'   },
            { nullptr,              no_argument,        nullptr, 0     }
        };

        return args;
    }

    constexpr inline string_view getApplicationArgsString() { return R"(vhdc:a:d)"; }

    inline string getHelpText(const string& argv0) {
        return fmt::format(R"(
{0:s} v{1:s}
Written by Simon Cahill

Usage: {0:s} [--daemon] [-c <"/path/to/conf">] [-a <api_key>]
       {0:s} --help

Switches:
    --version,  -v                  Displays the application version and exits
    --help,     -h                  Displays this menu and exits
    --daemon,   -d                  Run as a daemon (in service mode)

Arguments:
    --config,   -c <config_path>    Override the default config path ({2:s})
    --api-key,  -a <api_key>        Override the API key (provided by config)

)", argv0, applicationVersion(), ConfigManager::DEFAULT_CONFIG_LOCATION);
    }

} /* namespace resources */ } /* namespace abuseipdb_client */

#endif // ABUSEIPDB_CLIENT_INCLUDE_RESOUCRS_ARGS_HPP