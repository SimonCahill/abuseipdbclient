/**
 * @file ConfigManager.cpp
 * @author Simon Cahill (simon@simonc.eu)
 * @brief Contains the implementation of the ConfigManager class.
 * @version 0.1
 * @date 2022-05-29
 * 
 * @copyright Copyright (c) 2022 Simon Cahill
 */

///////////////////////
//  SYSTEM INCLUDES  //
///////////////////////
// stl
#include <filesystem>
#include <memory>
#include <string>

// json
#include <nlohmann/json.hpp>

///////////////////////
//  LOCAL  INCLUDES  //
///////////////////////
#include "cfg/ConfigManager.hpp"
#include "resources/Resources.hpp"
#include "util/Utilities.hpp"

namespace abuseipdb_client { namespace cfg {

    using nlohmann::json;

    using spdlog::fmt_lib::format;

    using std::error_code;
    using std::exception;
    using std::string;

    namespace fs = std::filesystem;

    #ifdef abuseipdb_CONFIG_LOCATION
        const string ConfigManager::DEFAULT_CONFIG_LOCATION = abuseipdb_CONFIG_LOCATION;
    #elif defined(__linux__)
        const string ConfigManager::DEFAULT_CONFIG_LOCATION = "/etc/abusipdb_client/config.json";
    #elif defined(WIN32)
        const string ConfigManager::DEFAULT_CONFIG_LOCATION = R"(C:\abuseipdb_client\config.json)";
    #endif
    const string ConfigManager::CONFIG_PATTERN = R"(([A-z0-9_-]+\.?)+)";

    /**
     * @brief Gets the current instance or returns a new instance of ConfigManager.
     * 
     * @return shared_ptr<ConfigManager> A shared_ptr object pointing to the instance of ConfigManager.
     */
    shared_ptr<ConfigManager> ConfigManager::getInstance() {
        static shared_ptr<ConfigManager> instance;

        if (!instance) {
            instance = shared_ptr<ConfigManager>(new ConfigManager());
        }

        return instance;
    }

    ConfigManager::ConfigManager(): m_logger(nullptr), m_cfgPath(DEFAULT_CONFIG_LOCATION) {}

    bool ConfigManager::hasConfig(const string& config) const { return hasConfig(m_configObj, config); }

    bool ConfigManager::hasConfig(const json& container, const string& path) const {
        if (utils::regexMatch(path, CONFIG_PATTERN)) {
            const auto tokens = utils::splitString(path, ".");
            return hasConfig(tokens.front()) ?
                hasConfig(m_configObj.at(tokens.front()), utils::replaceString(path, tokens.front() + ".", ""))
                : false;
        }

        return container.contains(path);
    }

    void ConfigManager::loadConfigs() {
        error_code err;

        string configString;

        if (!fs::exists(m_cfgPath, err) || !fs::is_regular_file(m_cfgPath, err) || !utils::readFile(m_cfgPath, configString)) {
            m_logger->error("Couldn't open config file. Does it exist? Will load defaults! Some features may not work as expected!");
            m_logger->error("This information might help: {:s}", err.value());
            configString = resources::getDefaultConfig();
        }

        try {
            m_configObj = json::parse(configString, nullptr, true, true);
        } catch (const exception& ex) {
            m_logger->critical("Failed to parse configuration! Error: {:s}", ex.what());
            m_logger->critical("Some or all application functions may be impaired!");
        }
    }



} /* namespace cfg */ } /* namespace abuseipdb_client */