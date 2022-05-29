/**
 * @file AbuseIpDbApi.hpp
 * @author Simon Cahil (simon@simonc.eu)
 * @brief Contains the API client for Abuse IPDB
 * @version 0.1
 * @date 2022-05-28
 * 
 * @copyright Copyright (c) 2022 Simon Cahill
 */

#ifndef ABUSEIPDB_CLIENT_INCLUDE_API_ABUSEIPDBAPI_HPP
#define ABUSEIPDB_CLIENT_INCLUDE_API_ABUSEIPDBAPI_HPP

///////////////////////
//  SYSTEM INCLUDES  //
///////////////////////
// stl
#include <exception>
#include <memory>
#include <string>
#include <vector>

// spdlog / fmt
#include <spdlog/spdlog.h>
#include <spdlog/formatter.h>

// curl
#include <curl/curl.h>

// nlohmann/json
#include <nlohmann/json.hpp>

///////////////////////
//  LOCAL  INCLUDES  //
///////////////////////

namespace abuseipdb_client { namespace api {

    using nlohmann::json;

    using spdlog::formatter;
    using spdlog::logger;

    using std::make_shared;
    using std::shared_ptr;
    using std::string;
    using std::vector;

    /**
     * @brief This class represents the client that communicates with the AbuseIpDb API via libcurl.
     * 
     * For documentation on the api, checkout [this link](https://docs.abuseipdb.com/).
     * This class is a singleton, as only one instance of it is required at any one time.
     * if multiple instances (for multiple API keys) are required for some reason, it is advisable to start a new
     * instance of the application.
     */
    class AbuseIpDbApi {
        public: // +++ Factory +++
            class Factory;

            enum class ReportCategories: uint64_t; //!< Enumeration of possible report categories

            struct BlackListOptions; //!< Contains the options for requesting a blacklist

        public: // +++ Constants +++
            const static size_t MAX_IPS_STANDARD; //!< 10.000
            const static size_t MAX_IPS_BASIC_SUB; //!< 100.000
            const static size_t MAX_IPS_PREMIUM_SUB; //!< 500.000

        public: // +++ Constructor / Destructor +++
            AbuseIpDbApi(const AbuseIpDbApi&) = delete;
            virtual ~AbuseIpDbApi() { curl_easy_cleanup(m_curl); }

        public: // +++ API Endpoints +++
            virtual json    bulkReport(const string& csv)                                      ; //!< Upload a CSV for bulk-reporting
            virtual json    checkBlocked(const string&, const size_t)                          ; //!< Check whether a subnet has reported addresses
            virtual json    checkIpAddress(const string& ipAddress)                            ; //!< Checks if a single IP has been reported before
            virtual json    clearIpAddress(const string& ipAddress) 	                       ; //!< Clears all reports of a given IP from the user account
            virtual json    getBlackList(const BlackListOptions&)                              ; //!< Gets a (more or less) complete blacklist
            virtual json    reportIp(const string&, const ReportCategories, const string& = ""); //!< Reports a single IP

            virtual string  getBlackListPlaintext(const BlackListOptions&)                     ; //!< Gets a (more or less) complete blacklist in plain text

        protected: // +++ Constructor +++
            AbuseIpDbApi(const string& apiKey, shared_ptr<logger> logger):
            m_apiKey(apiKey), m_curl(nullptr), m_isInitialised(false),
            m_logger(logger) {
                initialiseCurl();
            }

        protected: // +++ Initialisation +++
            virtual void    initialiseCurl();

        private:
            bool                        m_isInitialised;

            CURL*                       m_curl;

            shared_ptr<logger>  m_logger;

            string                      m_apiKey;
            string                      m_curlResponse;
            string                      m_curlResponseHeaders;
    };

    /**
     * @brief The factory class for the AbuseIpDbApi class.
     */
    class AbuseIpDbApi::Factory {
        public: // +++ Constructor / Destructor +++
            explicit    Factory(const string& apiKey, shared_ptr<logger> logger): m_apiKey(apiKey), m_logger(logger), m_instance(nullptr) {}
                        Factory(const Factory&) = delete;
            ~           Factory() {}

        public: // +++ Instance Management +++
            shared_ptr<AbuseIpDbApi>    getInstance(const bool override = false) {
                if (!m_instance) {
                    return (m_instance = shared_ptr<AbuseIpDbApi>(new AbuseIpDbApi(m_apiKey, m_logger)));
                }

                if (m_instance->m_apiKey != m_apiKey && override) {
                    m_instance.reset(new AbuseIpDbApi(m_apiKey, m_logger));
                }

                throw std::runtime_error("API key mismatch!");
            }

        private: // +++ Member Variables +++
            shared_ptr<AbuseIpDbApi>    m_instance;
            shared_ptr<logger>  m_logger;
            string                      m_apiKey;
    };

    /**
     * @brief A bit-coded enumeration of categories.
     * Note: the enum values do NOT represent the actual category numbers!
     * These values can be OR'd together to report multiple categories.
     */
    enum class AbuseIpDbApi::ReportCategories: uint64_t {
        DnsCompromise       = 1,
        DnsPoisoning        = 2,
        FraudOrders         = 4,
        DdosAttack          = 8,
        FtpBruteForce       = 16,
        PingOfDeath         = 32,
        Phishing            = 64,
        FraudVoip           = 128,
        OpenProxy           = 256,
        WebSpam             = 512,
        EmailSpam           = 1024,
        BlogSpam            = 2048,
        VpnIp               = 4096,
        PortScan            = 8192,
        Hacking             = 16384,
        SqlInjection        = 32768,
        Spoofing            = 65536,
        BruteForce          = 131072,
        BadWebBot           = 262144,
        ExploitedHost       = 524288,
        WebAppAttack        = 1048576,
        Ssh                 = 2097152,
        IotTargeted         = 4194304
        /* 23/64 bits used */
    };

    inline bool operator &(const AbuseIpDbApi::ReportCategories a, const uint64_t b) {
        return static_cast<uint64_t>(a) & static_cast<uint64_t>(b);
    }

    inline AbuseIpDbApi::ReportCategories operator |(const AbuseIpDbApi::ReportCategories a, const AbuseIpDbApi::ReportCategories b) {
        return (
            static_cast<AbuseIpDbApi::ReportCategories>(
                static_cast<uint64_t>(a) | static_cast<uint64_t>(b)
            )
        );
    }

    /**
     * @brief A struct used as a function parameter to set options for requesting a blacklist.
     */
    struct AbuseIpDbApi::BlackListOptions {
        size_t          limit;              //!< The max no. of entries the list shall contain
        size_t          minimumConfidence;  //!< The minimum required confidence (of abuse) each entry shall have

        vector<string>  onlyCountries;      //!< Only get reports from these countries (use country codes!)
        vector<string>  exceptCountries;    //!< Get reports for all countries except these (use country codes!)

        BlackListOptions():
            limit(AbuseIpDbApi::MAX_IPS_BASIC_SUB),
            minimumConfidence(100), onlyCountries({}), exceptCountries({}) {}
    };

    vector<int> getReportCategories(const AbuseIpDbApi::ReportCategories categories);

} /* namespace api */ } /* abuseipdb_client */

#endif // ABUSEIPDB_CLIENT_INCLUDE_API_ABUSEIPDBAPI_HPP