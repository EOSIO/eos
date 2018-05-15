#pragma once

// Distributed under the 3-Clause BSD License.  See accompanying
// file LICENSE or https://github.com/CLIUtils/CLI11 for details.

// This file was generated using MakeSingleHeader.py in CLI11/scripts
// from: v1.1.0-17-gb88f1f2

// This has the complete CLI library in one file.

#include <sys/types.h>
#include <iostream>
#include <string>
#include <deque>
#include <memory>
#include <algorithm>
#include <tuple>
#include <iomanip>
#include <type_traits>
#include <functional>
#include <numeric>
#include <fstream>
#include <vector>
#include <locale>
#include <set>
#include <stdexcept>
#include <exception>
#include <sstream>
#include <sys/stat.h>
#include <utility>
#include "ParseErrors.hpp"




namespace eosio { namespace aux {

// Type tools

// Copied from C++14
#if __cplusplus < 201402L
template <bool B, class T = void> using enable_if_t = typename std::enable_if<B, T>::type;
#else
// If your compiler supports C++14, you can use that definition instead
using std::enable_if_t;
#endif

template <typename T> struct is_vector { static const bool value = false; };

template <class T, class A> struct is_vector<std::vector<T, A>> { static bool const value = true; };

template <typename T> struct is_bool { static const bool value = false; };

template <> struct is_bool<bool> { static bool const value = true; };



// Based generally on https://rmf.io/cxx11/almost-static-if
/// Simple empty scoped class
enum class enabler {};

/// An instance to use in EnableIf
constexpr enabler dummy = {};

// Type name print

/// Was going to be based on
///  http://stackoverflow.com/questions/1055452/c-get-name-of-type-in-template
/// But this is cleaner and works better in this case

template <typename T,
          enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, enabler> = dummy>
constexpr const char *type_name() {
    return "INT";
}

template <typename T,
          enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, enabler> = dummy>
constexpr const char *type_name() {
    return "UINT";
}

template <typename T, enable_if_t<std::is_floating_point<T>::value, enabler> = dummy>
constexpr const char *type_name() {
    return "FLOAT";
}

/// This one should not be used, since vector types print the internal type
template <typename T, enable_if_t<is_vector<T>::value, enabler> = dummy>
constexpr const char *type_name() {
    return "VECTOR";
}

template <typename T,
        enable_if_t<!std::is_floating_point<T>::value && !std::is_integral<T>::value && !is_vector<T>::value,
                      enabler> = dummy>
constexpr const char *type_name()
{
    return "TEXT";
}

// Lexical cast

/// Integers / enums
template <typename T, enable_if_t<std::is_integral<T>::value
    || std::is_enum<T>::value, enabler> = dummy>
bool lexical_cast(std::string input, T &output)
{
    try {
        output = static_cast<T>(std::stoll(input));
        return true;
    } catch(const std::invalid_argument &) {
        return false;
    } catch(const std::out_of_range &) {
        return false;
    }
}

/// Floats
template <typename T, enable_if_t<std::is_floating_point<T>::value, enabler> = dummy>
bool lexical_cast(std::string input, T &output) {
    try {
        output = static_cast<T>(std::stold(input));
        return true;
    } catch(const std::invalid_argument &) {
        return false;
    } catch(const std::out_of_range &) {
        return false;
    }
}

/// String and similar
template <typename T,
    enable_if_t<!std::is_floating_point<T>::value && !std::is_integral<T>::value
                && !std::is_enum<T>::value, enabler> = dummy>
bool lexical_cast(std::string input, T &output) {
    output = input;
    return true;
}


// Based on http://stackoverflow.com/questions/236129/split-a-string-in-c
/// Split a string by a delim
inline std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    // Check to see if emtpy string, give consistent result
    if(s == "")
        elems.emplace_back("");
    else {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while(std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
    }
    return elems;
}

/// Simple function to join a string
template <typename T> std::string join(const T &v, std::string delim = ",")
{
    std::ostringstream s;
    size_t start = 0;
    for(const auto &i : v) {
        if(start++ > 0)
            s << delim;
        s << i;
    }
    return s.str();
}

/// Join a string in reverse order
template <typename T> std::string rjoin(const T &v, std::string delim = ",")
{
    std::ostringstream s;
    for(size_t start = 0; start < v.size(); start++) {
        if(start > 0)
            s << delim;
        s << v[v.size() - start - 1];
    }
    return s.str();
}

// Based roughly on http://stackoverflow.com/questions/25829143/c-trim-whitespace-from-a-string

/// Trim whitespace from left of string
inline std::string &ltrim(std::string &str) {
    auto it = std::find_if(str.begin(), str.end(),
                  [](char ch) { return !std::isspace<char>(ch, std::locale()); });
    str.erase(str.begin(), it);
    return str;
}

/// Trim anything from left of string
inline std::string &ltrim(std::string &str, const std::string &filter) {
    auto it = std::find_if(str.begin(), str.end(),
                           [&filter](char ch) { return filter.find(ch) == std::string::npos; });
    str.erase(str.begin(), it);
    return str;
}

/// Trim whitespace from right of string
inline std::string &rtrim(std::string &str) {
    auto it = std::find_if(str.rbegin(), str.rend(),
                           [](char ch) { return !std::isspace<char>(ch, std::locale()); });
    str.erase(it.base(), str.end());
    return str;
}

/// Trim anything from right of string
inline std::string &rtrim(std::string &str, const std::string &filter)
{
    auto it =
        std::find_if(str.rbegin(), str.rend(),
                     [&filter](char ch) { return filter.find(ch) == std::string::npos; });
    str.erase(it.base(), str.end());
    return str;
}

/// Trim whitespace from string
inline std::string &trim(std::string &str) { return ltrim(rtrim(str)); }

/// Trim anything from string
inline std::string &trim(std::string &str, const std::string filter)
{
   return ltrim(rtrim(str, filter), filter);
}

/// Make a copy of the string and then trim it
inline std::string trim_copy(const std::string &str) {
    std::string s = str;
    return trim(s);
}

/// Make a copy of the string and then trim it, any filter string can be used (any char in string is filtered)
inline std::string trim_copy(const std::string &str, const std::string &filter)
{
    std::string s = str;
    return trim(s, filter);
}
/// Print a two part "help" string
inline void format_help(std::stringstream &out, std::string name, std::string description, size_t wid)
{
    name = "  " + name;
    out << std::setw(static_cast<int>(wid)) << std::left << name;
    if(description != "") {
        if(name.length() >= wid)
            out << std::endl << std::setw(static_cast<int>(wid)) << "";
        out << description;
    }
    out << std::endl;
}

/// Verify the first character of an option
template <typename T> bool valid_first_char(T c)
{
   return std::isalpha(c, std::locale()) || c == '_';
}

/// Verify following characters of an option
template <typename T> bool valid_later_char(T c) {
    return std::isalnum(c, std::locale()) || c == '_' || c == '.' || c == '-';
}

/// Verify an option name
inline bool valid_name_string(const std::string &str) {
    if(str.empty() || !valid_first_char(str[0]))
        return false;
    for(auto c : str.substr(1))
        if(!valid_later_char(c))
            return false;
    return true;
}

/// Return a lower case version of a string
inline std::string to_lower(std::string str)
{
    std::transform(std::begin(str), std::end(str), std::begin(str),
                   [](const std::string::value_type &x) {
        return std::tolower(x, std::locale()); });
    return str;
}

/// Split a string '"one two" "three"' into 'one two', 'three'
inline std::vector<std::string> split_up(std::string str)
{
    std::vector<char> delims = {'\'', '\"'};
    auto find_ws = [](char ch) { return std::isspace<char>(ch, std::locale()); };
    trim(str);

    std::vector<std::string> output;

    while(!str.empty()) {
        if(str[0] == '\'') {
            auto end = str.find('\'', 1);
            if(end != std::string::npos) {
                output.push_back(str.substr(1, end - 1));
                str = str.substr(end + 1);
            } else {
                output.push_back(str.substr(1));
                str = "";
            }
        } else if(str[0] == '\"') {
            auto end = str.find('\"', 1);
            if(end != std::string::npos) {
                output.push_back(str.substr(1, end - 1));
                str = str.substr(end + 1);
            } else {
                output.push_back(str.substr(1));
                str = "";
            }

        } else {
            auto it = std::find_if(std::begin(str), std::end(str), find_ws);
            if(it != std::end(str)) {
                std::string value = std::string(str.begin(), it);
                output.push_back(value);
                str = std::string(it, str.end());
            } else {
                output.push_back(str);
                str = "";
            }
        }
        trim(str);
    }

    return output;
}


// Returns false if not a short option. Otherwise, sets opt name and rest and returns true
inline bool split_short(const std::string &current, std::string &name, std::string &rest)
{
    if(current.size() > 1 && current[0] == '-' && valid_first_char(current[1])) {
        name = current.substr(1, 1);
        rest = current.substr(2);
        return true;
    } else
        return false;
}

// Returns false if not a long option. Otherwise, sets opt name and other side of = and returns true
inline bool split_long(const std::string &current, std::string &name, std::string &value)
{
    if(current.size() > 2 && current.substr(0, 2) == "--" && valid_first_char(current[2])) {
        auto loc = current.find("=");
        if(loc != std::string::npos) {
            name = current.substr(2, loc - 2);
            value = current.substr(loc + 1);
        } else {
            name = current.substr(2);
            value = "";
        }
        return true;
    } else
        return false;
}

// Splits a string into multiple long and short names
inline std::vector<std::string> split_names(std::string current)
{
    std::vector<std::string> output;
    size_t val;
    while((val = current.find(",")) != std::string::npos) {
        output.push_back(current.substr(0, val));
        current = current.substr(val + 1);
    }
    output.push_back(current);
    return output;
}

/// Get a vector of short names, one of long names, and a single name
inline std::tuple<std::vector<std::string>, std::vector<std::string>, std::string>
get_names(const std::vector<std::string> &input)
{
    std::vector<std::string> short_names;
    std::vector<std::string> long_names;
    std::string pos_name;

    for(std::string name : input) {
        if(name.length() == 0)
            continue;
        else if(name.length() > 1 && name[0] == '-' && name[1] != '-') {
            if(name.length() == 2 && valid_first_char(name[1]))
                short_names.emplace_back(1, name[1]);
            else
                throw BadNameString("Invalid one char name: " + name);
        } else if(name.length() > 2 && name.substr(0, 2) == "--") {
            name = name.substr(2);
            if(valid_name_string(name))
                long_names.push_back(name);
            else
                throw BadNameString("Bad long name: " + name);
        } else if(name == "-" || name == "--") {
            throw BadNameString("Must have a name, not just dashes");
        } else {
            if(pos_name.length() > 0)
                throw BadNameString("Only one positional name allowed, remove: " + name);
            pos_name = name;
        }
    }

    return std::tuple<std::vector<std::string>, std::vector<std::string>, std::string>(
        short_names, long_names, pos_name);
}


inline std::string inijoin(std::vector<std::string> args) {
    std::ostringstream s;
    size_t start = 0;
    for(const auto &arg : args) {
        if(start++ > 0)
            s << " ";

        auto it = std::find_if(arg.begin(), arg.end(),
                               [](char ch) { return std::isspace<char>(ch, std::locale()); });
        if(it == arg.end())
            s << arg;
        else if(arg.find(R"(")") == std::string::npos)
            s << R"(")" << arg << R"(")";
        else
            s << R"(')" << arg << R"(')";
    }

    return s.str();
}

struct ini_ret_t
{
    /// This is the full name with dots
    std::string fullname;

    /// Listing of inputs
    std::vector<std::string> inputs;

    /// Current parent level
    size_t level = 0;

    /// Return parent or empty string, based on level
    ///
    /// Level 0, a.b.c would return a
    /// Level 1, a.b.c could return b
    std::string parent() const
    {
        std::vector<std::string> plist = split(fullname, '.');
        if(plist.size() > (level + 1))
            return plist[level];
        else
            return "";
    }

    /// Return name
    std::string name() const {
        std::vector<std::string> plist = split(fullname, '.');
        return plist.at(plist.size() - 1);
    }
};

/// Internal parsing function
inline std::vector<ini_ret_t> parse_ini(std::istream &input)
{
    std::string name, line;
    std::string section = "default";

    std::vector<ini_ret_t> output;

    while(getline(input, line)) {
        std::vector<std::string> items;

        trim(line);
        size_t len = line.length();
        if(len > 1 && line[0] == '[' && line[len - 1] == ']') {
            section = line.substr(1, len - 2);
        } else if(len > 0 && line[0] != ';') {
            output.emplace_back();
            ini_ret_t &out = output.back();

            // Find = in string, split and recombine
            auto pos = line.find("=");
            if(pos != std::string::npos) {
                name = trim_copy(line.substr(0, pos));
                std::string item = trim_copy(line.substr(pos + 1));
                items = split_up(item);
            } else {
                name = trim_copy(line);
                items = {"ON"};
            }

            if(to_lower(section) == "default")
                out.fullname = name;
            else
                out.fullname = section + "." + name;

            out.inputs.insert(std::end(out.inputs), std::begin(items), std::end(items));
        }
    }
    return output;
}

/// Parse an INI file, throw an error (ParseError:INIParseError or FileError) on failure
inline std::vector<ini_ret_t> parse_ini(const std::string &name) {

    std::ifstream input{name};
    if(!input.good())
        throw FileError(name);

    return parse_ini(input);
}

/// @defgroup validator_group Validators
/// @brief Some validators that are provided
///
/// These are simple `bool(std::string)` validators that are useful.
/// @{

/// Check for an existing file
inline bool ExistingFile(std::string filename)
{
    struct stat buffer;
    bool exist = stat(filename.c_str(), &buffer) == 0;
    bool is_dir = (buffer.st_mode & S_IFDIR) != 0;
    if(!exist) {
        std::cerr << "File does not exist: " << filename << std::endl;
        return false;
    } else if(is_dir) {
        std::cerr << "File is actually a directory: " << filename << std::endl;
        return false;
    } else {
        return true;
    }
}

/// Check for an existing directory
inline bool ExistingDirectory(std::string filename)
{
    struct stat buffer;
    bool exist = stat(filename.c_str(), &buffer) == 0;
    bool is_dir = (buffer.st_mode & S_IFDIR) != 0;
    if(!exist) {
        std::cerr << "Directory does not exist: " << filename << std::endl;
        return false;
    } else if(is_dir) {
        return true;
    } else {
        std::cerr << "Directory is actually a file: " << filename << std::endl;
        return false;
    }
}

/// Check for a non-existing path
inline bool NonexistentPath(std::string filename)
{
    struct stat buffer;
    bool exist = stat(filename.c_str(), &buffer) == 0;
    if (!exist)
    {
        return true;
    } else {
        std::cerr << "Path exists: " << filename << std::endl;
        return false;
    }
}

/// Produce a range validator function
template <typename T> std::function<bool(std::string)> Range(T min, T max)
{
    return [min, max](std::string input) { T val;
        lexical_cast(input, val);
        return val >= min && val <= max; };
}

/// Range of one value is 0 to value
template <typename T> std::function<bool(std::string)> Range(T max)
{
   return Range(static_cast<T>(0), max);
}

/// @}

}

} // namespace eosio
