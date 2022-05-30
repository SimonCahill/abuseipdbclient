/**
 * @file ConfigManager.hpp
 * @author Simon Cahill (simon@simonc.eu)
 * @brief Contains the declaration of the config manager for the application. Only applicable when running as a service.
 * @version 0.1
 * @date 2022-05-29
 * 
 * @copyright Copyright (c) 2022 Simon Cahill
 */

#ifndef ABUSEIPDB_INCLUDE_CFG_CONFIGMANAGER_HPP
#define ABUSEIPDB_INCLUDE_CFG_CONFIGMANAGER_HPP

///////////////////////
//  SYSTEM INCLUDES  //
///////////////////////
// stl
#include <exception>
#include <memory>
#include <string>

// json
#include <nlohmann/json.hpp>

// spdlog
#include <spdlog/spdlog.h>

///////////////////////
//  LOCAL  INCLUDES  //
///////////////////////
#include "util/Utilities.hpp"

namespace abuseipdb_client { namespace cfg {

    using nlohmann::json;

    using spdlog::logger;

    using std::exception;
    using std::shared_ptr;
    using std::string;

    class ConfigException: exception;

    /**
     * @brief Simple class providing basic functionality for a working config.
     */
    class ConfigManager {
        public: // +++ Static +++
            const static string             DEFAULT_CONFIG_LOCATION; //!< Platform-dependant value!
            const static string             CONFIG_PATTERN; //!< = R"(([A-z0-9_-]+\.?)+)";

            static shared_ptr<ConfigManager> getInstance(); ///!< Gets the instance of this class.

        public: // +++ Constructor / Destructor +++
                                            ConfigManager(const ConfigManager&) = delete;
            virtual ~                       ConfigManager() {}

        public: // +++ Setter / Setter +++
            virtual string                  getConfigPath() const { return m_cfgPath; }

            virtual void                    setConfigPath(const string& val) { m_cfgPath = val; }
            virtual void                    setLogger(shared_ptr<logger> val) { if (m_logger) { return; } m_logger = val; }

        public: // +++ Config Management +++
            virtual void                    loadConfigs();

        public: // +++ Config Getters / Setters +++
            virtual bool                    hasConfig(const string& config) const;

            template<class T>
            T                               getConfig(const string& path) const {
                return getConfig(m_configObj, path);
            }

        protected: // +++ Constructor +++
                                            ConfigManager();

        protected: // +++ Protected API +++
            virtual bool                    hasConfig(const json& container, const string& path) const;

            template<class T>
            T                               getConfig(const json& container, const string& path) const {
                if (!hasConfig(container, path)) {
                    throw ConfigException("Attempt to retrieve non-existing config!", path);
                }

                if (utils::regexMatch(path, REGEX_PATTERN)) {
                    const auto tokens = utils::splitString(path, ".");
                    
                }

                return container.at(path);
            }

        private:
            json                            m_configObj;

            shared_ptr<logger>  	        m_logger;

            string                          m_cfgPath;

        
    };

    class ConfigException: exception {
        public: // +++ Constructor / Destructor +++
            ConfigException(const string& configObj, const string& error):
                m_config(configObj), m_error(error) {}
            ~ConfigException() {}

        public: // +++ Exposed API +++
            const char* what() { spdlog::fmt_lib::format("{0:s}; Config object {1:s}", m_error, m_config).c_str(); }

        private: // +++ Private API +++
            const string    m_config;
            const string    m_error;
    };

} /* namespace cfg */ } /* namespace abuseipdb_client */

#endif // ABUSEIPDB_INCLUDE_CFG_CONFIGMANAGER_HPP
