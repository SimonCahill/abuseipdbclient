######################################################
##  Simple CMake script that reads the default      ##
##  configuration into a CMake variable, so it may  ##
##  be used in configure_files.                     ##
##                  © Simon Cahill                  ##
######################################################

set(abuseipdb_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cfg/config.json")
file(READ "${abuseipdb_CONFIG_FILE}" abuseipdb_DEFAULT_CONFIG)