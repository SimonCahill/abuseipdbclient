/**
 * @file Utilities.hpp
 * @author Simon Cahill (simon@simonc.eu)
 * @brief Contains useful utility and extension functions.
 * @version 0.1
 * @date 2022-05-29
 * 
 * @copyright Copyright (c) 2022 Simon Cahill
 */

#ifndef ABUSEIPDB_INCLUDE_UTIL_UTILITIES_HPP
#define ABUSEIPDB_INCLUDE_UTIL_UTILITIES_HPP

#include <fstream>
#include <regex>
#include <string>
#include <vector>

namespace abuseipdb_client { namespace utils {

    using std::ifstream;
    using std::regex;
    using std::string;
    using std::vector;

    namespace reg = std::regex_constants;

    inline bool readFile(const string& path, string &data) {
        string line;
        ifstream fileInput(path.c_str());
        int peekChar;
        bool returnVal = true;
        if (fileInput.is_open()) {
            while ((peekChar = fileInput.peek()) != EOF) {
                if (getline(fileInput, line, '\n')) {
                    data.append(line);
                    data.append("\n");
                } else {
                    // Getline failed. Find out what the error was
                    if (fileInput.bad() || fileInput.fail()) {
                        returnVal = false;
                    } else {
                        returnVal = true;
                    }
                }
            }
            fileInput.close();
        } else { returnVal = false; }

        return returnVal;
    }

    inline bool regexMatch(const string& haystack, const string& pattern) {
        const regex regPattern(pattern);
        return std::regex_match(haystack, regPattern, reg::ECMAScript);
    }

    inline bool splitString(const string& str, const string& delimiters, vector<string>& tokens, size_t maxLen = SIZE_MAX) {
        // Skip delimiters at beginning.
        size_t lastPos = str.find_first_not_of(delimiters, 0);
        // Find first "non-delimiter".
        size_t pos = str.find_first_of(delimiters, lastPos);

        while ((string::npos != pos || string::npos != lastPos) && maxLen < tokens.size()) {
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            // Skip delimiters.
            lastPos = string::npos == pos ? string::npos : pos + 1;
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiters, lastPos);
        }

        return tokens.size() > 0;
    }

    inline vector<string> splitString(const string& str, const string& delimiters, size_t maxLen = SIZE_MAX) {
        vector<string> tokens{};

        splitString(str, delimiters, tokens, maxLen);

        return tokens;
    }

    inline string replaceString(const string& haystack, const string& needle, const string& replacement, const int32_t maxCount = -1) {
        size_t needleLocation = 0;
        int32_t currentCount = 0;

        while ((needleLocation = haystack.find(needle)) != string::npos) {
            haystack.replace(needleLocation, needle.size(), replacement);
            if (currentCount++ >= maxCount) { break; }
        }

        return haystack;
    }

} /* namespace utils */ } /* namespace abuseipdb_client */

#endif // ABUSEIPDB_INCLUDE_UTIL_UTILITIES_HPP