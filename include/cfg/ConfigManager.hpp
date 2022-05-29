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
#include <memory>
#include <string>

// json
#include <nlohmann/json>

///////////////////////
//  LOCAL  INCLUDES  //
///////////////////////

namespace abuseipdb_client { namespace cfg {

    using nlohmann::json;

    using std::shared_ptr;
    using std::string;

    /**
     * @brief Simple class providing basic functionality for a working config.
     */
    class ConfigManager {
        public: // +++ Static +++
            const static string             DEFAULT_CONFIG_LOCATION; //!< Platform-dependant value!

            static shared_ptr<ConfigManager> getInstance(); ///!< Gets the instance of this class.

        public: // +++ Constructor / Destructor +++
                                            ConfigManager(const ConfigManager&) = delete;
            virtual ~                       ConfigManager() {}

        public: // +++ Setter / Setter +++
            virtual string                  getConfigPath() const { return m_cfgPath; }

            virtual void                    setConfigPath(const string& val) { m_cfgPath = val; }

        protected: // +++ Constructor +++
                                            ConfigManager();

        private:
            string                          m_cfgPath;

        
    };

} /* namespace cfg */ } /* namespace abuseipdb_client */

#endif // ABUSEIPDB_INCLUDE_CFG_CONFIGMANAGER_HPP