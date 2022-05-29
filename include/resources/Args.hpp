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

#include <getopt.h>

namespace abuseipdb_client { namespace resources {

    const inline struct option APP_OPTIONS[] = {
        { "--version",          no_argument, nullptr, 'v'   },
        { "--help",             no_argument, nullptr, 'h'   },
        { "",                   no_argument, nullptr, 0     },
        { nullptr,              no_argument, nullptr, 0     }
    };

} /* namespace resources */ } /* namespace abuseipdb_client */

#endif // ABUSEIPDB_CLIENT_INCLUDE_RESOUCRS_ARGS_HPP