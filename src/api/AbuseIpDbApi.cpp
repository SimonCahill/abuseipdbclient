/**
 * @file AbuseIpDbApi.cpp
 * @author Simon Cahill (simon@simonc.eu)
 * @brief Contains the implementation of the AbuseIpDbApi class.
 * @version 0.1
 * @date 2022-05-28
 * 
 * @copyright Copyright (c) 2022 Simon Cahill
 */

///////////////////////
//  SYSTEM INCLUDES  //
///////////////////////
// stl
#include <bitset>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

// curl
#include <curl/curl.h>

// nlohmann/json
#include <nlohmann/json.hpp>

// spdlog / fmt
#include <spdlog/formatter.h>

///////////////////////
//  LOCAL  INCLUDES  //
///////////////////////
#include "api/AbuseIpDbApi.hpp"

namespace abuseipdb_client { namespace api {

    using nlohmann::json;

    using spdlog::fmt_lib::format;

    using std::bitset;
    using std::error_code;
    using std::make_shared;
    using std::map;
    using std::shared_ptr;
    using std::string;
    using std::vector;

    namespace fs = std::filesystem;

    const size_t AbuseIpDbApi::MAX_IPS_STANDARD = 10'000;
    const size_t AbuseIpDbApi::MAX_IPS_BASIC_SUB = 100'000;
    const size_t AbuseIpDbApi::MAX_IPS_PREMIUM_SUB = 500'000;

    /**
     * @brief Escapes a string so it only contains legal URL chars.
     * 
     * @param unescapedString The unescaped URL/HTTP string
     * @param curl The curl instance
     * 
     * @return string The escaped string.
     */
    static string getEscapedString(const string& unescapedString, CURL* curl) {
        char* newString = curl_easy_escape(curl, unescapedString.c_str(), unescapedString.size());
        string escapedString(newString);
        
        return escapedString;
    }

    /**
     * @brief CURL write callback; writes data from the incoming stream to a given string output.
     * 
     * @param data The data received by CURL
     * @param dataLength Is always 1; the length of a byte?
     * @param memBufSize The size of the memory buffer
     * @param output A string* pointing to the output buffer.
     * 
     * @return size_t The total amount of bytes written.
     */
    static size_t handleCurlWrite(void* data, size_t dataLength, size_t memBufSize, string* output) {
        auto size = dataLength * memBufSize;

        auto str = string(reinterpret_cast<const char*>(data), size);

        if (str.empty() || std::all_of(str.begin(), str.end(), [&](const char x) { return std::isspace(x) != 0; })) {
            goto End;
        }

        output->append(str);

        End:
        return size;
    }

    /**
     * @brief Applies any standard or custom headers to the curl instance.
     * 
     * @param curl The CURL instance.
     * @param apiKey The API key for abuseipdb
     * @param otherHeaders A map of other headers to be applied.
     * 
     * @return struct curl_slist* A pointer to the newly constructed header list. Must be freed!
     */
    static struct curl_slist* setHeaders(CURL* curl, const string& apiKey, map<string, string> otherHeaders = {}) {
        struct curl_slist* headers = nullptr;

        headers = curl_slist_append(headers, ("Key: " + apiKey).c_str());
        headers = curl_slist_append(headers, "accept: application/json");

        if (otherHeaders.size() > 0) {
            std::for_each(otherHeaders.begin(), otherHeaders.end(), [&](const auto x) {
                auto header = format("{:s}: {:s}", x.first, x.second);
                headers = curl_slist_append(headers, header.c_str());
            });
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        return headers;
    }

    /**
     * @brief Uploads a compatible CSV to AbuseIPDB
     * 
     * @param csv The canonical path to the CSV file.
     * 
     * @return json The value returned from AbuseIPDB's API.
     */
    json AbuseIpDbApi::bulkReport(const string& csv) {
        initialiseCurl();

        const static string API_URL = "https://api.abuseipdb.com/api/v2/bulk-report";
        struct curl_slist* headers = setHeaders(m_curl, m_apiKey);

        error_code err;
        if (!fs::exists(csv, err) || !fs::is_regular_file(csv, err)) {
            throw fs::filesystem_error("Csv must be a valid file!", err);
        }

        size_t fileSize = fs::file_size(csv);

        FILE* fd = fopen(csv.c_str(), "rb");
        if (!fd) {
            err = error_code(errno, std::system_category());
            throw fs::filesystem_error("Failed to open file", fs::path(csv), err);
        }

        curl_easy_setopt(m_curl, CURLOPT_URL, API_URL.c_str());
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "POST");

        curl_mime* form = curl_mime_init(m_curl);
        curl_mimepart* field = curl_mime_addpart(form);

        // add csv
        curl_mime_name(field, "csv");
        curl_mime_filedata(field, csv.c_str());

        // add submit, just in case
        field = curl_mime_addpart(form);
        curl_mime_name(field, "submit");
        curl_mime_data(field, "send", CURL_ZERO_TERMINATED);

        curl_easy_setopt(m_curl, CURLOPT_MIMEPOST, form);

        auto retCode = curl_easy_perform(m_curl);

        if (retCode != CURLcode::CURLE_OK) {
            m_logger->error("CURL failed: {:s} ({:d})", curl_easy_strerror(retCode), retCode);
            return json();
        }
        
        try {
            return json::parse(m_curlResponse);
        } catch (...) {
            m_logger->error("Failed to parse JSON!");
            m_logger->trace("Erronious output: {:s}", m_curlResponse);
            return json();
        }
    }

    /**
     * @brief Checks whether a network address (CIDR notation) has any reported IPs
     * 
     * @param networkAddress The network address. E.g. 193.41.200.0
     * @param subnetSize The netmask (CIDR). E.g. 24
     * 
     * @return json THe AbuseIPDB response
     */
    json AbuseIpDbApi::checkBlocked(const string& networkAddress, const size_t subnetSize) {
        initialiseCurl();

        const static string API_URL = "https://api.abuseipdb.com/api/v2/check-block";
        struct curl_slist* headers = setHeaders(m_curl, m_apiKey);
        
        auto getParam = "network=" + getEscapedString(format("{:s}/{:d}", networkAddress, subnetSize), m_curl);
        
        auto url = format("{:s}?{:s}", API_URL, getParam);
        m_logger->debug("Connecting to {:s}", url);
        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        
        auto retCode = curl_easy_perform(m_curl);
        
        curl_slist_free_all(headers);
        curl_easy_reset(m_curl);

        if (retCode != CURLcode::CURLE_OK) {
            m_logger->error("CURL failed: {:s} ({:d})", curl_easy_strerror(retCode), retCode);
            return json();
        }
        
        try {
            return json::parse(m_curlResponse);
        } catch (...) {
            m_logger->error("Failed to parse JSON!");
            m_logger->trace("Erronious output: {:s}", m_curlResponse);
            return json();
        }
    }

    /**
     * @brief Checks whether a given IP address has been reported before.
     * 
     * @param ipAddress The IP address to check
     * 
     * @return json The response value.
     */
    json AbuseIpDbApi::checkIpAddress(const string& ipAddress) {
        initialiseCurl();

        const static string API_URL = "https://api.abuseipdb.com/api/v2/check";
        struct curl_slist* headers = setHeaders(m_curl, m_apiKey);
        
        auto ipParam = "ipAddress=" + getEscapedString(ipAddress, m_curl);
        
        auto url = format("{:s}?{:s}&verbose", API_URL, ipParam);
        m_logger->debug("Connecting to {:s}", url);
        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        
        auto retCode = curl_easy_perform(m_curl);
        
        curl_slist_free_all(headers);
        curl_easy_reset(m_curl);

        if (retCode != CURLcode::CURLE_OK) {
            m_logger->error("CURL failed: {:s} ({:d})", curl_easy_strerror(retCode), retCode);
            return json();
        }
        
        try {
            return json::parse(m_curlResponse);
        } catch (...) {
            m_logger->error("Failed to parse JSON!");
            m_logger->trace("Erronious output: {:s}", m_curlResponse);
            return json();
        }
    }

    /**
     * @brief Clears all reports of the passed IP address from the user account associated with the API key.
     * 
     * @param ipAddress The IP address to clear.
     * 
     * @return json The response value.
     */
    json AbuseIpDbApi::clearIpAddress(const string& ipAddress) {
        initialiseCurl();

        const static string API_URL = "https://api.abuseipdb.com/api/v2/clear-address";
        struct curl_slist* headers = setHeaders(m_curl, m_apiKey);
        
        auto ipParam = "ipAddress=" + getEscapedString(ipAddress, m_curl);
        
        auto url = format("{:s}?{:s}&verbose", API_URL, ipParam);
        m_logger->debug("Connecting to {:s}", url);
        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        
        auto retCode = curl_easy_perform(m_curl);
        
        curl_slist_free_all(headers);
        curl_easy_reset(m_curl);

        if (retCode != CURLcode::CURLE_OK) {
            m_logger->error("CURL failed: {:s} ({:d})", curl_easy_strerror(retCode), retCode);
            return json();
        }
        
        try {
            return json::parse(m_curlResponse);
        } catch (...) {
            m_logger->error("Failed to parse JSON!");
            m_logger->trace("Erronious output: {:s}", m_curlResponse);
            return json();
        }
    }

    /**
     * @brief Gets a blacklist from AbuseIPDB with the passed options.
     * 
     * @param options A struct containing possible options for the blacklist. Supplying an empty instance will apply defaults.
     * 
     * @return json The blacklist in JSON form.
     */
    json AbuseIpDbApi::getBlackList(const BlackListOptions& options) {
        initialiseCurl();

        const static string API_URL = "https://api.abuseipdb.com/api/v2/blacklist";
        struct curl_slist* headers = setHeaders(m_curl, m_apiKey);
        
        auto confidenceMinimum  = "confidenceMinimum=" + getEscapedString(std::to_string(options.minimumConfidence), m_curl);
        auto limit              = "limit=" + getEscapedString(std::to_string(options.limit), m_curl);
        auto countryList        = options.onlyCountries.size() > 0 ?
                                  "onlyCountries=" + getEscapedString(
                                    std::accumulate(options.onlyCountries.begin(), options.onlyCountries.end(), string{}), m_curl
                                  )
                                  :
                                  "exceptCountries=" + getEscapedString(
                                    std::accumulate(options.exceptCountries.begin(), options.exceptCountries.end(), string{}), m_curl
                                  );
        
        auto url = format("{:s}?{:s}&{:s}&{:s}", API_URL, confidenceMinimum, limit, countryList);
        m_logger->debug("Connecting to {:s}", url);
        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        
        auto retCode = curl_easy_perform(m_curl);
        
        curl_slist_free_all(headers);
        curl_easy_reset(m_curl);

        if (retCode != CURLcode::CURLE_OK) {
            m_logger->error("CURL failed: {:s} ({:d})", curl_easy_strerror(retCode), retCode);
            return json();
        }
        
        try {
            return json::parse(m_curlResponse);
        } catch (...) {
            m_logger->error("Failed to parse JSON!");
            m_logger->trace("Erronious output: {:s}", m_curlResponse);
            return json();
        }
    }

    /**
     * @brief Reports the passed IP address.
     * 
     * @param ipAddress The IP address to report.
     * @param categories The categories to apply to the report.
     * @param comment The comment for the report. (Don't forget to strip your personal information!)
     * 
     * @return json The response value.
     */
    json AbuseIpDbApi::reportIp(const string& ipAddress, const ReportCategories categories, const string& comment) {
        initialiseCurl();

        const static string API_URL = "https://api.abuseipdb.com/api/v2/report";
        struct curl_slist* headers = setHeaders(m_curl, m_apiKey);

        if (categories == static_cast<ReportCategories>(0)) {
            throw std::invalid_argument("categories must be a valid category!");
        }

        auto categoryList = getReportCategories(categories);

        if (categoryList.size() == 0) {
            throw std::runtime_error("Failed to parse categories!");
        }

        auto ip             = "ip=" + getEscapedString(ipAddress, m_curl);
        auto categoryParam  = "categories=" + getEscapedString(
            std::accumulate(
                std::next(categoryList.begin()), categoryList.end(), std::to_string(categoryList[0]),
                [&](string a, int64_t b) {
                    return std::move(a) + "," + std::to_string(b);
                }
            ), m_curl
        );
        auto commentParam    = "comment=" + getEscapedString(comment, m_curl);
        
        auto postParams = format("{:s}&{:s}&{:s}", ip, categoryParam, commentParam);
        m_logger->debug("Connecting to {:s}", API_URL);
        m_logger->debug("Post fields: {:s}", postParams);
        curl_easy_setopt(m_curl, CURLOPT_URL, API_URL.c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postParams.c_str());
        
        auto retCode = curl_easy_perform(m_curl);
        
        curl_slist_free_all(headers);
        curl_easy_reset(m_curl);

        if (retCode != CURLcode::CURLE_OK) {
            m_logger->error("CURL failed: {:s} ({:d})", curl_easy_strerror(retCode), retCode);
            return json();
        }
        
        try {
            return json::parse(m_curlResponse);
        } catch (...) {
            m_logger->error("Failed to parse JSON!");
            m_logger->trace("Erronious output: {:s}", m_curlResponse);
            return json();
        }
    }

    /**
     * @brief Gets a blacklist from AbuseIPDB with certain options in plaintext.
     * 
     * @param options The options to apply to the blacklist. Supply an empty object to use defaults.
     * 
     * @return string The blacklist in plaintext.
     */
    string AbuseIpDbApi::getBlackListPlaintext(const BlackListOptions& options) {
        initialiseCurl();

        const static string API_URL = "https://api.abuseipdb.com/api/v2/blacklist";
        struct curl_slist* headers = setHeaders(m_curl, m_apiKey, map<string, string>{ { "Accept", "text/plain" } });
        
        auto confidenceMinimum  = "confidenceMinimum=" + getEscapedString(std::to_string(options.minimumConfidence), m_curl);
        auto limit              = "limit=" + getEscapedString(std::to_string(options.limit), m_curl);
        auto countryList        = options.onlyCountries.size() > 0 ?
                                  "onlyCountries=" + getEscapedString(
                                    std::accumulate(options.onlyCountries.begin(), options.onlyCountries.end(), string{}), m_curl
                                  )
                                  :
                                  "exceptCountries=" + getEscapedString(
                                    std::accumulate(options.exceptCountries.begin(), options.exceptCountries.end(), string{}), m_curl
                                  );
        
        auto url = format("{:s}?{:s}&{:s}&{:s}&plaintext", API_URL, confidenceMinimum, limit, countryList);
        m_logger->debug("Connecting to {:s}", url);
        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        
        auto retCode = curl_easy_perform(m_curl);
        
        curl_slist_free_all(headers);
        curl_easy_reset(m_curl);

        if (retCode != CURLcode::CURLE_OK) {
            m_logger->error("CURL failed: {:s} ({:d})", curl_easy_strerror(retCode), retCode);
            return json();
        }
        
        try {
            return json::parse(m_curlResponse).dump(2);
        } catch (...) {
            return m_curlResponse;
        }
    }

    /**
     * @brief Initialises the CURL library
     * 
     */
    void AbuseIpDbApi::initialiseCurl() {
        if (!m_isInitialised) {
            m_curl = curl_easy_init();
            m_isInitialised = true;
        }

        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, handleCurlWrite);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_curlResponse);
        curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &m_curlResponseHeaders);
        curl_easy_setopt(m_curl, CURLOPT_DNS_LOCAL_IP4, 1);
        curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, 0);

        #ifdef abuseipdb_DEBUG
        // curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);
        #endif

        m_curlResponse.clear();
        m_curlResponseHeaders.clear();
    }
    
    /**
     * @brief Extracts all the AbuseIPDB categories for a bulk report from a single enum value.
     * 
     * @param values The value containing combined categories.
     * @return vector<int> A vector containing integers with the categories.
     */
    vector<int> getReportCategories(const AbuseIpDbApi::ReportCategories values) {
        vector<int> categories{};

        auto val = static_cast<uint64_t>(values);

        // Well, this is a bit ugly, but it'll work for now.
        // There's probably some super elegant solution to this
        if (val & 1      ) { categories.push_back(1);  }
        if (val & 2      ) { categories.push_back(2);  }
        if (val & 4      ) { categories.push_back(3);  }
        if (val & 8      ) { categories.push_back(4);  }
        if (val & 16     ) { categories.push_back(5);  }
        if (val & 32     ) { categories.push_back(6);  }
        if (val & 64     ) { categories.push_back(7);  }
        if (val & 128    ) { categories.push_back(8);  }
        if (val & 256    ) { categories.push_back(9);  }
        if (val & 512    ) { categories.push_back(10); }
        if (val & 1024   ) { categories.push_back(11); }
        if (val & 2048   ) { categories.push_back(12); }
        if (val & 4096   ) { categories.push_back(13); }
        if (val & 8192   ) { categories.push_back(14); }
        if (val & 16384  ) { categories.push_back(15); }
        if (val & 32768  ) { categories.push_back(16); }
        if (val & 65536  ) { categories.push_back(17); }
        if (val & 131072 ) { categories.push_back(18); }
        if (val & 262144 ) { categories.push_back(19); }
        if (val & 524288 ) { categories.push_back(20); }
        if (val & 1048576) { categories.push_back(21); }
        if (val & 2097152) { categories.push_back(22); }
        if (val & 4194304) { categories.push_back(23); }

        return categories;
    }

} /* namespace api */ } /* abuseipdb_client */