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

// From CLI/Error.hpp

namespace CLI {

/// These codes are part of every error in CLI. They can be obtained from e using e.exit_code or as a quick shortcut,
/// int values from e.get_error_code().
enum class ExitCodes {
    Success = 0,
    IncorrectConstruction = 100,
    BadNameString,
    OptionAlreadyAdded,
    File,
    Conversion,
    Validation,
    Required,
    Requires,
    Excludes,
    Extras,
    ExtrasINI,
    Invalid,
    Horrible,
    OptionNotFound,
    BaseClass = 255
};

// Error definitions

/// @defgroup error_group Errors
/// @brief Errors thrown by CLI11
///
/// These are the errors that can be thrown. Some of them, like CLI::Success, are not really errors.
/// @{

/// All errors derive from this one
struct Error : public std::runtime_error {
    int exit_code;
    bool print_help;
    int get_exit_code() const { return exit_code; }
    Error(std::string parent, std::string name, ExitCodes exit_code = ExitCodes::BaseClass, bool print_help = true)
        : runtime_error(parent + ": " + name), exit_code(static_cast<int>(exit_code)), print_help(print_help) {}
    Error(std::string parent,
          std::string name,
          int exit_code = static_cast<int>(ExitCodes::BaseClass),
          bool print_help = true)
        : runtime_error(parent + ": " + name), exit_code(exit_code), print_help(print_help) {}
};

/// Construction errors (not in parsing)
struct ConstructionError : public Error {
    // Using Error::Error constructors seem to not work on GCC 4.7
    ConstructionError(std::string parent,
                      std::string name,
                      ExitCodes exit_code = ExitCodes::BaseClass,
                      bool print_help = true)
        : Error(parent, name, exit_code, print_help) {}
};

/// Thrown when an option is set to conflicting values (non-vector and multi args, for example)
struct IncorrectConstruction : public ConstructionError {
    IncorrectConstruction(std::string name)
        : ConstructionError("IncorrectConstruction", name, ExitCodes::IncorrectConstruction) {}
};

/// Thrown on construction of a bad name
struct BadNameString : public ConstructionError {
    BadNameString(std::string name) : ConstructionError("BadNameString", name, ExitCodes::BadNameString) {}
};

/// Thrown when an option already exists
struct OptionAlreadyAdded : public ConstructionError {
    OptionAlreadyAdded(std::string name)
        : ConstructionError("OptionAlreadyAdded", name, ExitCodes::OptionAlreadyAdded) {}
};

// Parsing errors

/// Anything that can error in Parse
struct ParseError : public Error {
    ParseError(std::string parent, std::string name, ExitCodes exit_code = ExitCodes::BaseClass, bool print_help = true)
        : Error(parent, name, exit_code, print_help) {}
};

// Not really "errors"

/// This is a successful completion on parsing, supposed to exit
struct Success : public ParseError {
    Success() : ParseError("Success", "Successfully completed, should be caught and quit", ExitCodes::Success, false) {}
};

/// -h or --help on command line
struct CallForHelp : public ParseError {
    CallForHelp()
        : ParseError("CallForHelp", "This should be caught in your main function, see examples", ExitCodes::Success) {}
};

/// Thrown when parsing an INI file and it is missing
struct FileError : public ParseError {
    FileError(std::string name) : ParseError("FileError", name, ExitCodes::File) {}
};

/// Thrown when conversion call back fails, such as when an int fails to coerse to a string
struct ConversionError : public ParseError {
    ConversionError(std::string name) : ParseError("ConversionError", name, ExitCodes::Conversion) {}
};

/// Thrown when validation of results fails
struct ValidationError : public ParseError {
    ValidationError(std::string name) : ParseError("ValidationError", name, ExitCodes::Validation) {}
};

/// Thrown when a required option is missing
struct RequiredError : public ParseError {
    RequiredError(std::string name) : ParseError("RequiredError", name, ExitCodes::Required) {}
};

/// Thrown when a requires option is missing
struct RequiresError : public ParseError {
    RequiresError(std::string name, std::string subname)
        : ParseError("RequiresError", name + " requires " + subname, ExitCodes::Requires) {}
};

/// Thrown when a exludes option is present
struct ExcludesError : public ParseError {
    ExcludesError(std::string name, std::string subname)
        : ParseError("ExcludesError", name + " excludes " + subname, ExitCodes::Excludes) {}
};

/// Thrown when too many positionals or options are found
struct ExtrasError : public ParseError {
    ExtrasError(std::string name) : ParseError("ExtrasError", name, ExitCodes::Extras) {}
};

/// Thrown when extra values are found in an INI file
struct ExtrasINIError : public ParseError {
    ExtrasINIError(std::string name) : ParseError("ExtrasINIError", name, ExitCodes::ExtrasINI) {}
};

/// Thrown when validation fails before parsing
struct InvalidError : public ParseError {
    InvalidError(std::string name) : ParseError("InvalidError", name, ExitCodes::Invalid) {}
};

/// This is just a safety check to verify selection and parsing match
struct HorribleError : public ParseError {
    HorribleError(std::string name)
        : ParseError("HorribleError", "(You should never see this error) " + name, ExitCodes::Horrible) {}
};

// After parsing

/// Thrown when counting a non-existent option
struct OptionNotFound : public Error {
    OptionNotFound(std::string name) : Error("OptionNotFound", name, ExitCodes::OptionNotFound) {}
};

/// @}

} // namespace CLI

// From CLI/TypeTools.hpp

namespace CLI {

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

namespace detail {
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
          enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, detail::enabler> = detail::dummy>
constexpr const char *type_name() {
    return "INT";
}

template <typename T,
          enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, detail::enabler> = detail::dummy>
constexpr const char *type_name() {
    return "UINT";
}

template <typename T, enable_if_t<std::is_floating_point<T>::value, detail::enabler> = detail::dummy>
constexpr const char *type_name() {
    return "FLOAT";
}

/// This one should not be used, since vector types print the internal type
template <typename T, enable_if_t<is_vector<T>::value, detail::enabler> = detail::dummy>
constexpr const char *type_name() {
    return "VECTOR";
}

template <typename T,
          enable_if_t<!std::is_floating_point<T>::value && !std::is_integral<T>::value && !is_vector<T>::value,
                      detail::enabler> = detail::dummy>
constexpr const char *type_name() {
    return "TEXT";
}

// Lexical cast

/// Integers / enums
template <typename T, enable_if_t<std::is_integral<T>::value
    || std::is_enum<T>::value
    , detail::enabler> = detail::dummy>
bool lexical_cast(std::string input, T &output) {
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
template <typename T, enable_if_t<std::is_floating_point<T>::value, detail::enabler> = detail::dummy>
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
template <
    typename T,
    enable_if_t<!std::is_floating_point<T>::value
                && !std::is_integral<T>::value
                && !std::is_enum<T>::value, detail::enabler> = detail::dummy>
bool lexical_cast(std::string input, T &output) {
    output = input;
    return true;
}

} // namespace detail
} // namespace CLI

// From CLI/StringTools.hpp

namespace CLI {
namespace detail {

// Based on http://stackoverflow.com/questions/236129/split-a-string-in-c
/// Split a string by a delim
inline std::vector<std::string> split(const std::string &s, char delim) {
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
template <typename T> std::string join(const T &v, std::string delim = ",") {
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
template <typename T> std::string rjoin(const T &v, std::string delim = ",") {
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
    auto it = std::find_if(str.begin(), str.end(), [](char ch) { return !std::isspace<char>(ch, std::locale()); });
    str.erase(str.begin(), it);
    return str;
}

/// Trim anything from left of string
inline std::string &ltrim(std::string &str, const std::string &filter) {
    auto it = std::find_if(str.begin(), str.end(), [&filter](char ch) { return filter.find(ch) == std::string::npos; });
    str.erase(str.begin(), it);
    return str;
}

/// Trim whitespace from right of string
inline std::string &rtrim(std::string &str) {
    auto it = std::find_if(str.rbegin(), str.rend(), [](char ch) { return !std::isspace<char>(ch, std::locale()); });
    str.erase(it.base(), str.end());
    return str;
}

/// Trim anything from right of string
inline std::string &rtrim(std::string &str, const std::string &filter) {
    auto it =
        std::find_if(str.rbegin(), str.rend(), [&filter](char ch) { return filter.find(ch) == std::string::npos; });
    str.erase(it.base(), str.end());
    return str;
}

/// Trim whitespace from string
inline std::string &trim(std::string &str) { return ltrim(rtrim(str)); }

/// Trim anything from string
inline std::string &trim(std::string &str, const std::string filter) { return ltrim(rtrim(str, filter), filter); }

/// Make a copy of the string and then trim it
inline std::string trim_copy(const std::string &str) {
    std::string s = str;
    return trim(s);
}

/// Make a copy of the string and then trim it, any filter string can be used (any char in string is filtered)
inline std::string trim_copy(const std::string &str, const std::string &filter) {
    std::string s = str;
    return trim(s, filter);
}
/// Print a two part "help" string
inline void format_help(std::stringstream &out, std::string name, std::string description, size_t wid) {
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
template <typename T> bool valid_first_char(T c) { return std::isalpha(c, std::locale()) || c == '_'; }

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
inline std::string to_lower(std::string str) {
    std::transform(std::begin(str), std::end(str), std::begin(str), [](const std::string::value_type &x) {
        return std::tolower(x, std::locale());
    });
    return str;
}

/// Split a string '"one two" "three"' into 'one two', 'three'
inline std::vector<std::string> split_up(std::string str) {

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

} // namespace detail
} // namespace CLI

// From CLI/Split.hpp

namespace CLI {
namespace detail {

// Returns false if not a short option. Otherwise, sets opt name and rest and returns true
inline bool split_short(const std::string &current, std::string &name, std::string &rest) {
    if(current.size() > 1 && current[0] == '-' && valid_first_char(current[1])) {
        name = current.substr(1, 1);
        rest = current.substr(2);
        return true;
    } else
        return false;
}

// Returns false if not a long option. Otherwise, sets opt name and other side of = and returns true
inline bool split_long(const std::string &current, std::string &name, std::string &value) {
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
inline std::vector<std::string> split_names(std::string current) {
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
get_names(const std::vector<std::string> &input) {

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

} // namespace detail
} // namespace CLI

// From CLI/Ini.hpp

namespace CLI {
namespace detail {

inline std::string inijoin(std::vector<std::string> args) {
    std::ostringstream s;
    size_t start = 0;
    for(const auto &arg : args) {
        if(start++ > 0)
            s << " ";

        auto it = std::find_if(arg.begin(), arg.end(), [](char ch) { return std::isspace<char>(ch, std::locale()); });
        if(it == arg.end())
            s << arg;
        else if(arg.find(R"(")") == std::string::npos)
            s << R"(")" << arg << R"(")";
        else
            s << R"(')" << arg << R"(')";
    }

    return s.str();
}

struct ini_ret_t {
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
    std::string parent() const {
        std::vector<std::string> plist = detail::split(fullname, '.');
        if(plist.size() > (level + 1))
            return plist[level];
        else
            return "";
    }

    /// Return name
    std::string name() const {
        std::vector<std::string> plist = detail::split(fullname, '.');
        return plist.at(plist.size() - 1);
    }
};

/// Internal parsing function
inline std::vector<ini_ret_t> parse_ini(std::istream &input) {
    std::string name, line;
    std::string section = "default";

    std::vector<ini_ret_t> output;

    while(getline(input, line)) {
        std::vector<std::string> items;

        detail::trim(line);
        size_t len = line.length();
        if(len > 1 && line[0] == '[' && line[len - 1] == ']') {
            section = line.substr(1, len - 2);
        } else if(len > 0 && line[0] != ';') {
            output.emplace_back();
            ini_ret_t &out = output.back();

            // Find = in string, split and recombine
            auto pos = line.find("=");
            if(pos != std::string::npos) {
                name = detail::trim_copy(line.substr(0, pos));
                std::string item = detail::trim_copy(line.substr(pos + 1));
                items = detail::split_up(item);
            } else {
                name = detail::trim_copy(line);
                items = {"ON"};
            }

            if(detail::to_lower(section) == "default")
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

} // namespace detail
} // namespace CLI

// From CLI/Validators.hpp

namespace CLI {

/// @defgroup validator_group Validators
/// @brief Some validators that are provided
///
/// These are simple `bool(std::string)` validators that are useful.
/// @{

/// Check for an existing file
inline bool ExistingFile(std::string filename) {
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
inline bool ExistingDirectory(std::string filename) {
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
inline bool NonexistentPath(std::string filename) {
    struct stat buffer;
    bool exist = stat(filename.c_str(), &buffer) == 0;
    if(!exist) {
        return true;
    } else {
        std::cerr << "Path exists: " << filename << std::endl;
        return false;
    }
}

/// Produce a range validator function
template <typename T> std::function<bool(std::string)> Range(T min, T max) {
    return [min, max](std::string input) {
        T val;
        detail::lexical_cast(input, val);
        return val >= min && val <= max;
    };
}

/// Range of one value is 0 to value
template <typename T> std::function<bool(std::string)> Range(T max) { return Range(static_cast<T>(0), max); }

/// @}

} // namespace CLI

// From CLI/Option.hpp

namespace CLI {

using results_t = std::vector<std::string>;
using callback_t = std::function<bool(results_t)>;

class Option;
class App;

using Option_p = std::unique_ptr<Option>;

class Option {
    friend App;

  protected:
    /// @name Names
    ///@{

    /// A list of the short names (`-a`) without the leading dashes
    std::vector<std::string> snames_;

    /// A list of the long names (`--a`) without the leading dashes
    std::vector<std::string> lnames_;

    /// A positional name
    std::string pname_;

    /// If given, check the environment for this option
    std::string envname_;

    ///@}
    /// @name Help
    ///@{

    /// The description for help strings
    std::string description_;

    /// A human readable default value, usually only set if default is true in creation
    std::string defaultval_;

    /// A human readable type value, set when App creates this
    std::string typeval_;

    /// The group membership
    std::string group_{"Options"};

    /// True if this option has a default
    bool default_{false};

    ///@}
    /// @name Configuration
    ///@{

    /// True if this is a required option
    bool required_{false};

    /// The number of expected values, 0 for flag, -1 for unlimited vector
    int expected_{1};

    /// A private setting to allow args to not be able to accept incorrect expected values
    bool changeable_{false};

    /// Ignore the case when matching (option, not value)
    bool ignore_case_{false};

    /// A list of validators to run on each value parsed
    std::vector<std::function<bool(std::string)>> validators_;

    /// A list of options that are required with this option
    std::set<Option *> requires_;

    /// A list of options that are excluded with this option
    std::set<Option *> excludes_;

    ///@}
    /// @name Other
    ///@{

    /// Remember the parent app
    App *parent_;

    /// Options store a callback to do all the work
    callback_t callback_;

    ///@}
    /// @name Parsing results
    ///@{

    /// Results of parsing
    results_t results_;

    /// Whether the callback has run (needed for INI parsing)
    bool callback_run_{false};

    ///@}

    /// Making an option by hand is not defined, it must be made by the App class
    Option(std::string name,
           std::string description = "",
           std::function<bool(results_t)> callback = [](results_t) { return true; },
           bool default_ = true,
           App *parent = nullptr)
        : description_(std::move(description)), default_(default_), parent_(parent), callback_(std::move(callback)) {
        std::tie(snames_, lnames_, pname_) = detail::get_names(detail::split_names(name));
    }

  public:
    /// @name Basic
    ///@{

    /// Count the total number of times an option was passed
    size_t count() const { return results_.size(); }

    /// This class is true if option is passed.
    operator bool() const { return count() > 0; }

    /// Clear the parsed results (mostly for testing)
    void clear() { results_.clear(); }

    ///@}
    /// @name Setting options
    ///@{

    /// Set the option as required
    Option *required() {
        if( !required_ ) {
            description_ += " (required)";
        }
        required_ = true;
        return this;
    }

    /// Support Plubmum term
    Option *mandatory() { return required(); }

    /// Set the number of expected arguments (Flags bypass this)
    Option *expected(int value) {
        if(value == 0)
            throw IncorrectConstruction("Cannot set 0 expected, use a flag instead");
        else if(expected_ == 0)
            throw IncorrectConstruction("Cannot make a flag take arguments!");
        else if(!changeable_)
            throw IncorrectConstruction("You can only change the expected arguments for vectors");
        expected_ = value;
        return this;
    }

    /// Adds a validator
    Option *check(std::function<bool(std::string)> validator) {

        validators_.push_back(validator);
        return this;
    }

    /// Changes the group membership
    Option *group(std::string name) {
        group_ = name;
        return this;
    }

    /// Sets required options
    Option *requires(Option *opt) {
        auto tup = requires_.insert(opt);
        if(!tup.second)
            throw OptionAlreadyAdded(get_name() + " requires " + opt->get_name());
        return this;
    }

    /// Can find a string if needed
    template <typename T = App> Option *requires(std::string opt_name) {
        for(const Option_p &opt : dynamic_cast<T *>(parent_)->options_)
            if(opt.get() != this && opt->check_name(opt_name))
                return requires(opt.get());
        throw IncorrectConstruction("Option " + opt_name + " is not defined");
    }

    /// Any number supported, any mix of string and Opt
    template <typename A, typename B, typename... ARG> Option *requires(A opt, B opt1, ARG... args) {
        requires(opt);
        return requires(opt1, args...);
    }

    /// Sets excluded options
    Option *excludes(Option *opt) {
        auto tup = excludes_.insert(opt);
        if(!tup.second)
            throw OptionAlreadyAdded(get_name() + " excludes " + opt->get_name());
        return this;
    }

    /// Can find a string if needed
    template <typename T = App> Option *excludes(std::string opt_name) {
        for(const Option_p &opt : dynamic_cast<T *>(parent_)->options_)
            if(opt.get() != this && opt->check_name(opt_name))
                return excludes(opt.get());
        throw IncorrectConstruction("Option " + opt_name + " is not defined");
    }
    /// Any number supported, any mix of string and Opt
    template <typename A, typename B, typename... ARG> Option *excludes(A opt, B opt1, ARG... args) {
        excludes(opt);
        return excludes(opt1, args...);
    }

    /// Sets environment variable to read if no option given
    Option *envname(std::string name) {
        envname_ = name;
        return this;
    }

    /// Ignore case
    ///
    /// The template hides the fact that we don't have the definition of App yet.
    /// You are never expected to add an argument to the template here.
    template <typename T = App> Option *ignore_case(bool value = true) {
        ignore_case_ = value;
        for(const Option_p &opt : dynamic_cast<T *>(parent_)->options_)
            if(opt.get() != this && *opt == *this)
                throw OptionAlreadyAdded(opt->get_name());
        return this;
    }

    ///@}
    /// @name Accessors
    ///@{

    /// True if this is a required option
    bool get_required() const { return required_; }

    /// The number of arguments the option expects
    int get_expected() const { return expected_; }

    /// True if this has a default value
    int get_default() const { return default_; }

    /// True if the argument can be given directly
    bool get_positional() const { return pname_.length() > 0; }

    /// True if option has at least one non-positional name
    bool nonpositional() const { return (snames_.size() + lnames_.size()) > 0; }

    /// True if option has description
    bool has_description() const { return description_.length() > 0; }

    /// Get the group of this option
    const std::string &get_group() const { return group_; }

    /// Get the description
    const std::string &get_description() const { return description_; }

    // Just the pname
    std::string get_pname() const { return pname_; }

    ///@}
    /// @name Help tools
    ///@{

    /// Gets a , sep list of names. Does not include the positional name if opt_only=true.
    std::string get_name(bool opt_only = false) const {
        std::vector<std::string> name_list;
        if(!opt_only && pname_.length() > 0)
            name_list.push_back(pname_);
        for(const std::string &sname : snames_)
            name_list.push_back("-" + sname);
        for(const std::string &lname : lnames_)
            name_list.push_back("--" + lname);
        return detail::join(name_list);
    }

    /// The name and any extras needed for positionals
    std::string help_positional() const {
        std::string out = pname_;
        if(get_expected() > 1)
            out = out + "(" + std::to_string(get_expected()) + "x)";
        else if(get_expected() == -1)
            out = out + "...";
        out = get_required() ? out : "[" + out + "]";
        return out;
    }

    /// The first half of the help print, name plus default, etc
    std::string help_name() const {
        std::stringstream out;
        out << get_name(true) << help_aftername();
        return out.str();
    }

    /// pname with type info
    std::string help_pname() const {
        std::stringstream out;
        out << get_pname() << help_aftername();
        return out.str();
    }

    /// This is the part after the name is printed but before the description
    std::string help_aftername() const {
        std::stringstream out;

        if(get_expected() != 0) {
            if(typeval_ != "")
                out << " " << typeval_;
            if(defaultval_ != "")
                out << "=" << defaultval_;
            if(get_expected() > 1)
                out << " x " << get_expected();
            if(get_expected() == -1)
                out << " ...";
        }
        if(envname_ != "")
            out << " (env:" << envname_ << ")";
        if(!requires_.empty()) {
            out << " Requires:";
            for(const Option *opt : requires_)
                out << " " << opt->get_name();
        }
        if(!excludes_.empty()) {
            out << " Excludes:";
            for(const Option *opt : excludes_)
                out << " " << opt->get_name();
        }
        return out.str();
    }

    ///@}
    /// @name Parser tools
    ///@{

    /// Process the callback
    void run_callback() const {
        if(!callback_(results_))
            throw ConversionError(get_name() + "=" + detail::join(results_));
        if(!validators_.empty()) {
            for(const std::string &result : results_)
                for(const std::function<bool(std::string)> &vali : validators_)
                    if(!vali(result))
                        throw ValidationError(get_name() + "=" + result);
        }
    }

    /// If options share any of the same names, they are equal (not counting positional)
    bool operator==(const Option &other) const {
        for(const std::string &sname : snames_)
            if(other.check_sname(sname))
                return true;
        for(const std::string &lname : lnames_)
            if(other.check_lname(lname))
                return true;
        // We need to do the inverse, just in case we are ignore_case
        for(const std::string &sname : other.snames_)
            if(check_sname(sname))
                return true;
        for(const std::string &lname : other.lnames_)
            if(check_lname(lname))
                return true;
        return false;
    }

    /// Check a name. Requires "-" or "--" for short / long, supports positional name
    bool check_name(std::string name) const {

        if(name.length() > 2 && name.substr(0, 2) == "--")
            return check_lname(name.substr(2));
        else if(name.length() > 1 && name.substr(0, 1) == "-")
            return check_sname(name.substr(1));
        else {
            std::string local_pname = pname_;
            if(ignore_case_) {
                local_pname = detail::to_lower(local_pname);
                name = detail::to_lower(name);
            }
            return name == local_pname;
        }
    }

    /// Requires "-" to be removed from string
    bool check_sname(std::string name) const {
        if(ignore_case_) {
            name = detail::to_lower(name);
            return std::find_if(std::begin(snames_), std::end(snames_), [&name](std::string local_sname) {
                       return detail::to_lower(local_sname) == name;
                   }) != std::end(snames_);
        } else
            return std::find(std::begin(snames_), std::end(snames_), name) != std::end(snames_);
    }

    /// Requires "--" to be removed from string
    bool check_lname(std::string name) const {
        if(ignore_case_) {
            name = detail::to_lower(name);
            return std::find_if(std::begin(lnames_), std::end(lnames_), [&name](std::string local_sname) {
                       return detail::to_lower(local_sname) == name;
                   }) != std::end(lnames_);
        } else
            return std::find(std::begin(lnames_), std::end(lnames_), name) != std::end(lnames_);
    }

    /// Puts a result at position r
    void add_result(std::string s) {
        results_.push_back(s);
        callback_run_ = false;
    }

    /// Get a copy of the results
    std::vector<std::string> results() const { return results_; }

    /// See if the callback has been run already
    bool get_callback_run() const { return callback_run_; }

    ///@}
    /// @name Custom options
    ///@{

    /// Set a custom option, typestring, expected, and changeable
    void set_custom_option(std::string typeval, int expected = 1, bool changeable = false) {
        typeval_ = typeval;
        expected_ = expected;
        changeable_ = changeable;
    }

    /// Set the default value string representation
    void set_default_val(std::string val) { defaultval_ = val; }


    /// Set the type name displayed on this option
    void set_type_name(std::string val) {typeval_ = val;}

    ///@}

  protected:
    /// @name App Helpers
    ///@{
    /// Can print positional name detailed option if true
    bool _has_help_positional() const {
        return get_positional() && (has_description() || !requires_.empty() || !excludes_.empty());
    }
    ///@}
};

} // namespace CLI

// From CLI/App.hpp

namespace CLI {

namespace detail {
enum class Classifer { NONE, POSITIONAL_MARK, SHORT, LONG, SUBCOMMAND };
struct AppFriend;
} // namespace detail

class App;

using App_p = std::unique_ptr<App>;

/// Creates a command line program, with very few defaults.
/** To use, create a new `Program()` instance with `argc`, `argv`, and a help description. The templated
*  add_option methods make it easy to prepare options. Remember to call `.start` before starting your
* program, so that the options can be evaluated and the help option doesn't accidentally run your program. */
class App final {
    friend Option;
    friend detail::AppFriend;

  protected:
    // This library follows the Google style guide for member names ending in underscores

    /// @name Basics
    ///@{

    /// Subcommand name or program name (from parser)
    std::string name_{"program"};

    /// Description of the current program/subcommand
    std::string description_;

    /// If true, allow extra arguments (ie, don't throw an error).
    bool allow_extras_{false};

    ///  If true, return immediatly on an unrecognised option (implies allow_extras)
    bool prefix_command_{false};
    
    /// This is a function that runs when complete. Great for subcommands. Can throw.
    std::function<void()> callback_;

    ///@}
    /// @name Options
    ///@{

    /// The list of options, stored locally
    std::vector<Option_p> options_;

    /// A pointer to the help flag if there is one
    Option *help_ptr_{nullptr};

    ///@}
    /// @name Parsing
    ///@{

    using missing_t = std::vector<std::pair<detail::Classifer, std::string>>;

    /// Pair of classifier, string for missing options. (extra detail is removed on returning from parse)
    ///
    /// This is faster and cleaner than storing just a list of strings and reparsing. This may contain the -- separator.
    missing_t missing_;

    /// This is a list of pointers to options with the orignal parse order
    std::vector<Option *> parse_order_;

    ///@}
    /// @name Subcommands
    ///@{

    /// Storage for subcommand list
    std::vector<App_p> subcommands_;

    /// If true, the program name is not case sensitive
    bool ignore_case_{false};

    /// Allow subcommand fallthrough, so that parent commands can collect commands after subcommand.
    bool fallthrough_{false};

    /// A pointer to the parent if this is a subcommand
    App *parent_{nullptr};

    /// True if this command/subcommand was parsed
    bool parsed_{false};

    /// -1 for 1 or more, 0 for not required, # for exact number required
    int require_subcommand_ = 0;

    ///@}
    /// @name Config
    ///@{

    /// The name of the connected config file
    std::string config_name_;

    /// True if ini is required (throws if not present), if false simply keep going.
    bool config_required_{false};

    /// Pointer to the config option
    Option *config_ptr_{nullptr};

    ///@}

    /// Special private constructor for subcommand
    App(std::string description_, bool help, detail::enabler) : description_(std::move(description_)) {

        if(help)
            help_ptr_ = add_flag("-h,--help", "Print this help message and exit");
    }

  public:
    /// @name Basic
    ///@{

    /// Create a new program. Pass in the same arguments as main(), along with a help string.
    App(std::string description_ = "", bool help = true) : App(description_, help, detail::dummy) {}

    /// Set a callback for the end of parsing.
    ///
    /// Due to a bug in c++11,
    /// it is not possible to overload on std::function (fixed in c++14
    /// and backported to c++11 on newer compilers). Use capture by reference
    /// to get a pointer to App if needed.
    App *set_callback(std::function<void()> callback) {
        callback_ = callback;
        return this;
    }

    /// Remove the error when extras are left over on the command line.
    App *allow_extras(bool allow = true) {
        allow_extras_ = allow;
        return this;
    }

    /// Do not parse anything after the first unrecongnised option and return
    App *prefix_command(bool allow = true) {
        prefix_command_ = allow;
        return this;
    }
    
    /// Ignore case. Subcommand inherit value.
    App *ignore_case(bool value = true) {
        ignore_case_ = value;
        if(parent_ != nullptr) {
            for(const auto &subc : parent_->subcommands_) {
                if(subc.get() != this && (this->check_name(subc->name_) || subc->check_name(this->name_)))
                    throw OptionAlreadyAdded(subc->name_);
            }
        }
        return this;
    }

    /// Check to see if this subcommand was parsed, true only if received on command line.
    bool parsed() const { return parsed_; }

    /// Check to see if this subcommand was parsed, true only if received on command line.
    /// This allows the subcommand to be directly checked.
    operator bool() const { return parsed_; }

    /// Require a subcommand to be given (does not affect help call)
    /// Does not return a pointer since it is supposed to be called on the main App.
    App *require_subcommand(int value = -1) {
        require_subcommand_ = value;
        return this;
    }

    /// Stop subcommand fallthrough, so that parent commands cannot collect commands after subcommand.
    /// Default from parent, usually set on parent.
    App *fallthrough(bool value = true) {
        fallthrough_ = value;
        return this;
    }

    ///@}
    /// @name Adding options
    ///@{

    /// Add an option, will automatically understand the type for common types.
    ///
    /// To use, create a variable with the expected type, and pass it in after the name.
    /// After start is called, you can use count to see if the value was passed, and
    /// the value will be initialized properly. Numbers, vectors, and strings are supported.
    ///
    /// ->required(), ->default, and the validators are options,
    /// The positional options take an optional number of arguments.
    ///
    /// For example,
    ///
    ///     std::string filename;
    ///     program.add_option("filename", filename, "description of filename");
    ///
    Option *add_option(std::string name, callback_t callback, std::string description = "", bool defaulted = false) {
        Option myopt{name, description, callback, defaulted, this};

        if(std::find_if(std::begin(options_), std::end(options_), [&myopt](const Option_p &v) {
               return *v == myopt;
           }) == std::end(options_)) {
            options_.emplace_back();
            Option_p &option = options_.back();
            option.reset(new Option(name, description, callback, defaulted, this));
            return option.get();
        } else
            throw OptionAlreadyAdded(myopt.get_name());
    }
    
    

    /// Add option for non-vectors (duplicate copy needed without defaulted to avoid `iostream << value`)
    template <typename T, enable_if_t<!is_vector<T>::value, detail::enabler> = detail::dummy>
    Option *add_option(std::string name,
                       T &variable, ///< The variable to set
                       std::string description = "") {

        CLI::callback_t fun = [&variable](CLI::results_t res) {
            if(res.size() != 1)
                return false;
            return detail::lexical_cast(res[0], variable);
        };

        Option *opt = add_option(name, fun, description, false);
        opt->set_custom_option(detail::type_name<T>());
        return opt;
    }

    /// Add option for non-vectors with a default print
    template <typename T, enable_if_t<!is_vector<T>::value, detail::enabler> = detail::dummy>
    Option *add_option(std::string name,
                       T &variable, ///< The variable to set
                       std::string description,
                       bool defaulted) {
        
        CLI::callback_t fun = [&variable](CLI::results_t res) {
            if(res.size() != 1)
                return false;
            return detail::lexical_cast(res[0], variable);
        };
        
        Option *opt = add_option(name, fun, description, defaulted);
        opt->set_custom_option(detail::type_name<T>());
        if(defaulted) {
            std::stringstream out;
            out << variable;
            opt->set_default_val(out.str());
        }
        return opt;
    }
    
    /// Add option for vectors (no default)
    template <typename T>
    Option *add_option(std::string name,
                       std::vector<T> &variable, ///< The variable vector to set
                       std::string description = "") {
        
        CLI::callback_t fun = [&variable](CLI::results_t res) {
            bool retval = true;
            variable.clear();
            for(const auto &a : res) {
                variable.emplace_back();
                retval &= detail::lexical_cast(a, variable.back());
            }
            return (!variable.empty()) && retval;
        };
        
        Option *opt = add_option(name, fun, description, false);
        opt->set_custom_option(detail::type_name<T>(), -1, true);
        return opt;
    }
    
    /// Add option for vectors
    template <typename T>
    Option *add_option(std::string name,
                       std::vector<T> &variable, ///< The variable vector to set
                       std::string description,
                       bool defaulted) {

        CLI::callback_t fun = [&variable](CLI::results_t res) {
            bool retval = true;
            variable.clear();
            for(const auto &a : res) {
                variable.emplace_back();
                retval &= detail::lexical_cast(a, variable.back());
            }
            return (!variable.empty()) && retval;
        };

        Option *opt = add_option(name, fun, description, defaulted);
        opt->set_custom_option(detail::type_name<T>(), -1, true);
        if(defaulted)
            opt->set_default_val("[" + detail::join(variable) + "]");
        return opt;
    }

    /// Add option for flag
    Option *add_flag(std::string name, std::string description = "") {
        CLI::callback_t fun = [](CLI::results_t) { return true; };

        Option *opt = add_option(name, fun, description, false);
        if(opt->get_positional())
            throw IncorrectConstruction("Flags cannot be positional");
        opt->set_custom_option("", 0);
        return opt;
    }

    /// Add option for flag integer
    template <typename T,
              enable_if_t<std::is_integral<T>::value && !is_bool<T>::value, detail::enabler> = detail::dummy>
    Option *add_flag(std::string name,
                     T &count, ///< A varaible holding the count
                     std::string description = "") {

        count = 0;
        CLI::callback_t fun = [&count](CLI::results_t res) {
            count = static_cast<T>(res.size());
            return true;
        };

        Option *opt = add_option(name, fun, description, false);
        if(opt->get_positional())
            throw IncorrectConstruction("Flags cannot be positional");
        opt->set_custom_option("", 0);
        return opt;
    }

    /// Bool version only allows the flag once
    template <typename T, enable_if_t<is_bool<T>::value, detail::enabler> = detail::dummy>
    Option *add_flag(std::string name,
                     T &count, ///< A varaible holding true if passed
                     std::string description = "") {

        count = false;
        CLI::callback_t fun = [&count](CLI::results_t res) {
            count = true;
            return res.size() == 1;
        };

        Option *opt = add_option(name, fun, description, false);
        if(opt->get_positional())
            throw IncorrectConstruction("Flags cannot be positional");
        opt->set_custom_option("", 0);
        return opt;
    }

    /// Add set of options (No default)
    template <typename T>
    Option *add_set(std::string name,
                    T &member,           ///< The selected member of the set
                    std::set<T> options, ///< The set of posibilities
                    std::string description = "") {

        CLI::callback_t fun = [&member, options](CLI::results_t res) {
            if(res.size() != 1) {
                return false;
            }
            bool retval = detail::lexical_cast(res[0], member);
            if(!retval)
                return false;
            return std::find(std::begin(options), std::end(options), member) != std::end(options);
        };

        Option *opt = add_option(name, fun, description, false);
        std::string typeval = detail::type_name<T>();
        typeval += " in {" + detail::join(options) + "}";
        opt->set_custom_option(typeval);
        return opt;
    }
    
    /// Add set of options
    template <typename T>
    Option *add_set(std::string name,
                    T &member,           ///< The selected member of the set
                    std::set<T> options, ///< The set of posibilities
                    std::string description,
                    bool defaulted) {
        
        CLI::callback_t fun = [&member, options](CLI::results_t res) {
            if(res.size() != 1) {
                return false;
            }
            bool retval = detail::lexical_cast(res[0], member);
            if(!retval)
                return false;
            return std::find(std::begin(options), std::end(options), member) != std::end(options);
        };
        
        Option *opt = add_option(name, fun, description, defaulted);
        std::string typeval = detail::type_name<T>();
        typeval += " in {" + detail::join(options) + "}";
        opt->set_custom_option(typeval);
        if(defaulted) {
            std::stringstream out;
            out << member;
            opt->set_default_val(out.str());
        }
        return opt;
    }

    /// Add set of options, string only, ignore case (no default)
    Option *add_set_ignore_case(std::string name,
                                std::string &member,           ///< The selected member of the set
                                std::set<std::string> options, ///< The set of posibilities
                                std::string description = "") {

        CLI::callback_t fun = [&member, options](CLI::results_t res) {
            if(res.size() != 1) {
                return false;
            }
            member = detail::to_lower(res[0]);
            auto iter = std::find_if(std::begin(options), std::end(options), [&member](std::string val) {
                return detail::to_lower(val) == member;
            });
            if(iter == std::end(options))
                return false;
            else {
                member = *iter;
                return true;
            }
        };

        Option *opt = add_option(name, fun, description, false);
        std::string typeval = detail::type_name<std::string>();
        typeval += " in {" + detail::join(options) + "}";
        opt->set_custom_option(typeval);
        
        return opt;
    }
    
    /// Add set of options, string only, ignore case
    Option *add_set_ignore_case(std::string name,
                                std::string &member,           ///< The selected member of the set
                                std::set<std::string> options, ///< The set of posibilities
                                std::string description,
                                bool defaulted) {
        
        CLI::callback_t fun = [&member, options](CLI::results_t res) {
            if(res.size() != 1) {
                return false;
            }
            member = detail::to_lower(res[0]);
            auto iter = std::find_if(std::begin(options), std::end(options), [&member](std::string val) {
                return detail::to_lower(val) == member;
            });
            if(iter == std::end(options))
                return false;
            else {
                member = *iter;
                return true;
            }
        };
        
        Option *opt = add_option(name, fun, description, defaulted);
        std::string typeval = detail::type_name<std::string>();
        typeval += " in {" + detail::join(options) + "}";
        opt->set_custom_option(typeval);
        if(defaulted) {
            opt->set_default_val(member);
        }
        return opt;
    }

    /// Add a complex number
    template <typename T>
    Option *add_complex(std::string name,
                        T &variable,
                        std::string description = "",
                        bool defaulted = false,
                        std::string label = "COMPLEX") {
        CLI::callback_t fun = [&variable](results_t res) {
            if(res.size() != 2)
                return false;
            double x, y;
            bool worked = detail::lexical_cast(res[0], x) && detail::lexical_cast(res[1], y);
            if(worked)
                variable = T(x, y);
            return worked;
        };

        CLI::Option *opt = add_option(name, fun, description, defaulted);
        opt->set_custom_option(label, 2);
        if(defaulted) {
            std::stringstream out;
            out << variable;
            opt->set_default_val(out.str());
        }
        return opt;
    }

    /// Add a configuration ini file option
    Option *add_config(std::string name = "--config",
                       std::string default_filename = "",
                       std::string help = "Read an ini file",
                       bool required = false) {

        // Remove existing config if present
        if(config_ptr_ != nullptr)
            remove_option(config_ptr_);
        config_name_ = default_filename;
        config_required_ = required;
        config_ptr_ = add_option(name, config_name_, help, default_filename != "");
        return config_ptr_;
    }

    /// Removes an option from the App. Takes an option pointer. Returns true if found and removed.
    bool remove_option(Option *opt) {
        auto iterator =
            std::find_if(std::begin(options_), std::end(options_), [opt](const Option_p &v) { return v.get() == opt; });
        if(iterator != std::end(options_)) {
            options_.erase(iterator);
            return true;
        }
        return false;
    }

    ///@}
    /// @name Subcommmands
    ///@{

    /// Add a subcommand. Like the constructor, you can override the help message addition by setting help=false
    App *add_subcommand(std::string name, std::string description = "", bool help = true) {
        subcommands_.emplace_back(new App(description, help, detail::dummy));
        subcommands_.back()->name_ = name;
        subcommands_.back()->allow_extras();
        subcommands_.back()->parent_ = this;
        subcommands_.back()->ignore_case_ = ignore_case_;
        subcommands_.back()->fallthrough_ = fallthrough_;
        for(const auto &subc : subcommands_)
            if(subc.get() != subcommands_.back().get())
                if(subc->check_name(subcommands_.back()->name_) || subcommands_.back()->check_name(subc->name_))
                    throw OptionAlreadyAdded(subc->name_);
        return subcommands_.back().get();
    }

    /// Check to see if a subcommand is part of this command (doesn't have to be in command line)
    App *get_subcommand(App *subcom) const {
        for(const App_p &subcomptr : subcommands_)
            if(subcomptr.get() == subcom)
                return subcom;
        throw CLI::OptionNotFound(subcom->get_name());
    }

    /// Check to see if a subcommand is part of this command (text version)
    App *get_subcommand(std::string subcom) const {
        for(const App_p &subcomptr : subcommands_)
            if(subcomptr->check_name(subcom))
                return subcomptr.get();
        throw CLI::OptionNotFound(subcom);
    }

    ///@}
    /// @name Extras for subclassing
    ///@{

    /// This allows subclasses to inject code before callbacks but after parse.
    ///
    /// This does not run if any errors or help is thrown.
    virtual void pre_callback() {}

    ///@}
    /// @name Parsing
    ///@{

    /// Parses the command line - throws errors
    /// This must be called after the options are in but before the rest of the program.
    std::vector<std::string> parse(int argc, char **argv) {
        name_ = argv[0];
        std::vector<std::string> args;
        for(int i = argc - 1; i > 0; i--)
            args.emplace_back(argv[i]);
        return parse(args);
    }

    /// The real work is done here. Expects a reversed vector.
    /// Changes the vector to the remaining options.
    std::vector<std::string> &parse(std::vector<std::string> &args) {
        _validate();
        _parse(args);
        run_callback();
        return args;
    }

    /// Print a nice error message and return the exit code
    int exit(const Error &e) const {
        if(e.exit_code != static_cast<int>(ExitCodes::Success)) {
            std::cerr << "ERROR: ";
            std::cerr << e.what() << std::endl;
            if(e.print_help)
                std::cerr << help();
        } else {
            if(e.print_help)
                std::cout << help();
        }
        return e.get_exit_code();
    }

    /// Reset the parsed data
    void reset() {

        parsed_ = false;
        missing_.clear();

        for(const Option_p &opt : options_) {
            opt->clear();
        }
        for(const App_p &app : subcommands_) {
            app->reset();
        }
    }

    ///@}
    /// @name Post parsing
    ///@{

    /// Counts the number of times the given option was passed.
    size_t count(std::string name) const {
        for(const Option_p &opt : options_) {
            if(opt->check_name(name)) {
                return opt->count();
            }
        }
        throw OptionNotFound(name);
    }

    /// Get a subcommand pointer list to the currently selected subcommands (after parsing)
    std::vector<App *> get_subcommands() const {
        std::vector<App *> subcomms;
        for(const App_p &subcomptr : subcommands_)
            if(subcomptr->parsed_)
                subcomms.push_back(subcomptr.get());
        return subcomms;
    }

    /// Check to see if given subcommand was selected
    bool got_subcommand(App *subcom) const {
        // get subcom needed to verify that this was a real subcommand
        return get_subcommand(subcom)->parsed_;
    }

    /// Check with name instead of pointer to see if subcommand was selected
    bool got_subcommand(std::string name) const { return get_subcommand(name)->parsed_; }

    ///@}
    /// @name Help
    ///@{

    /// Produce a string that could be read in as a config of the current values of the App. Set default_also to include
    /// default arguments. Prefix will add a string to the beginning of each option.
    std::string config_to_str(bool default_also = false, std::string prefix = "") const {
        std::stringstream out;
        for(const Option_p &opt : options_) {

            // Only process option with a long-name
            if(!opt->lnames_.empty()) {
                std::string name = prefix + opt->lnames_[0];

                // Non-flags
                if(opt->get_expected() != 0) {

                    // If the option was found on command line
                    if(opt->count() > 0)
                        out << name << "=" << detail::inijoin(opt->results()) << std::endl;

                    // If the option has a default and is requested by optional argument
                    else if(default_also && opt->defaultval_ != "")
                        out << name << "=" << opt->defaultval_ << std::endl;
                    // Flag, one passed
                } else if(opt->count() == 1) {
                    out << name << "=true" << std::endl;

                    // Flag, multiple passed
                } else if(opt->count() > 1) {
                    out << name << "=" << opt->count() << std::endl;

                    // Flag, not present
                } else if(opt->count() == 0 && default_also && opt.get() != get_help_ptr()) {
                    out << name << "=false" << std::endl;
                }
            }
        }
        for(const App_p &subcom : subcommands_)
            out << subcom->config_to_str(default_also, prefix + subcom->name_ + ".");
        return out.str();
    }

    /// Makes a help message, with a column wid for column 1
    std::string help(size_t wid = 30, std::string prev = "") const {
        // Delegate to subcommand if needed
        if(prev == "")
            prev = name_;
        else
            prev += " " + name_;

        auto selected_subcommands = get_subcommands();
        if(!selected_subcommands.empty())
            return selected_subcommands.at(0)->help(wid, prev);

        std::stringstream out;
        out << description_ << std::endl;
        out << "Usage: " << prev;

        // Check for options_
        bool npos = false;
        std::set<std::string> groups;
        for(const Option_p &opt : options_) {
            if(opt->nonpositional()) {
                npos = true;
                groups.insert(opt->get_group());
            }
        }

        if(npos)
            out << " [OPTIONS]";

        // Positionals
        bool pos = false;
        for(const Option_p &opt : options_)
            if(opt->get_positional()) {
                // A hidden positional should still show up in the usage statement
                // if(detail::to_lower(opt->get_group()) == "hidden")
                //    continue;
                out << " " << opt->help_positional();
                if(opt->_has_help_positional())
                    pos = true;
            }

        if(!subcommands_.empty()) {
            if(require_subcommand_ != 0)
                out << " SUBCOMMAND";
            else
                out << " [SUBCOMMAND]";
        }

        out << std::endl << std::endl;

        // Positional descriptions
        if(pos) {
            out << "Positionals:" << std::endl;
            for(const Option_p &opt : options_) {
                if(detail::to_lower(opt->get_group()) == "hidden")
                    continue;
                if(opt->_has_help_positional())
                    detail::format_help(out, opt->help_pname(), opt->get_description(), wid);
            }
            out << std::endl;
        }

        // Options
        if(npos) {
            for(const std::string &group : groups) {
                if(detail::to_lower(group) == "hidden")
                    continue;
                out << group << ":" << std::endl;
                for(const Option_p &opt : options_) {
                    if(opt->nonpositional() && opt->get_group() == group)
                        detail::format_help(out, opt->help_name(), opt->get_description(), wid);
                }
                out << std::endl;
            }
        }

        // Subcommands
        if(!subcommands_.empty()) {
            out << "Subcommands:" << std::endl;
            for(const App_p &com : subcommands_)
                detail::format_help(out, com->get_name(), com->description_, wid);
        }
        return out.str();
    }

    ///@}
    /// @name Getters
    ///@{

    /// Get a pointer to the help flag.
    Option *get_help_ptr() { return help_ptr_; }

    /// Get a pointer to the help flag. (const)
    const Option *get_help_ptr() const { return help_ptr_; }

    /// Get a pointer to the config option.
    Option *get_config_ptr() { return config_ptr_; }

    /// Get a pointer to the config option. (const)
    const Option *get_config_ptr() const { return config_ptr_; }
    /// Get the name of the current app
    std::string get_name() const { return name_; }

    /// Check the name, case insensitive if set
    bool check_name(std::string name_to_check) const {
        std::string local_name = name_;
        if(ignore_case_) {
            local_name = detail::to_lower(name_);
            name_to_check = detail::to_lower(name_to_check);
        }

        return local_name == name_to_check;
    }

    /// This gets a vector of pointers with the original parse order
    const std::vector<Option *> &parse_order() const { return parse_order_; }

    ///@}

  protected:
    /// Check the options to make sure there are no conficts.
    ///
    /// Currenly checks to see if mutiple positionals exist with -1 args
    void _validate() const {
        auto count = std::count_if(std::begin(options_), std::end(options_), [](const Option_p &opt) {
            return opt->get_expected() == -1 && opt->get_positional();
        });
        if(count > 1)
            throw InvalidError(name_ + ": Too many positional arguments with unlimited expected args");
        for(const App_p &app : subcommands_)
            app->_validate();
    }

    /// Return missing from the master
    missing_t *missing() {
        if(parent_ != nullptr)
            return parent_->missing();
        return &missing_;
    }

    /// Internal function to run (App) callback, top down
    void run_callback() {
        pre_callback();
        if(callback_)
            callback_();
        for(App *subc : get_subcommands()) {
            subc->run_callback();
        }
    }

    bool _valid_subcommand(const std::string &current) const {
        for(const App_p &com : subcommands_)
            if(com->check_name(current) && !*com)
                return true;
        if(parent_ != nullptr)
            return parent_->_valid_subcommand(current);
        else
            return false;
    }

    /// Selects a Classifer enum based on the type of the current argument
    detail::Classifer _recognize(const std::string &current) const {
        std::string dummy1, dummy2;

        if(current == "--")
            return detail::Classifer::POSITIONAL_MARK;
        if(_valid_subcommand(current))
            return detail::Classifer::SUBCOMMAND;
        if(detail::split_long(current, dummy1, dummy2))
            return detail::Classifer::LONG;
        if(detail::split_short(current, dummy1, dummy2))
            return detail::Classifer::SHORT;
        return detail::Classifer::NONE;
    }

    /// Internal parse function
    void _parse(std::vector<std::string> &args) {
        parsed_ = true;
        bool positional_only = false;

        while(!args.empty()) {
            _parse_single(args, positional_only);
        }

        if(help_ptr_ != nullptr && help_ptr_->count() > 0) {
            throw CallForHelp();
        }

        // Process an INI file
        if(config_ptr_ != nullptr && config_name_ != "") {
            try {
                std::vector<detail::ini_ret_t> values = detail::parse_ini(config_name_);
                while(!values.empty()) {
                    if(!_parse_ini(values)) {
                        throw ExtrasINIError(values.back().fullname);
                    }
                }
            } catch(const FileError &) {
                if(config_required_)
                    throw;
            }
        }

        // Get envname options if not yet passed
        for(const Option_p &opt : options_) {
            if(opt->count() == 0 && opt->envname_ != "") {
                char *buffer = nullptr;
                std::string ename_string;

                #ifdef _MSC_VER
                // Windows version
                size_t sz = 0;
                if(_dupenv_s(&buffer, &sz, opt->envname_.c_str()) == 0 && buffer != nullptr) {
                    ename_string = std::string(buffer);
                    free(buffer);
                }
                #else
                // This also works on Windows, but gives a warning
                buffer = std::getenv(opt->envname_.c_str());
                if(buffer != nullptr)
                    ename_string = std::string(buffer);
                #endif

                if(!ename_string.empty()) {
                    opt->add_result(ename_string);
                }
            }
        }

        // Process callbacks
        for(const Option_p &opt : options_) {
            if(opt->count() > 0 && !opt->get_callback_run()) {
                opt->run_callback();
            }
        }

        // Verify required options
        for(const Option_p &opt : options_) {
            // Required
            if(opt->get_required() && (static_cast<int>(opt->count()) < opt->get_expected() || opt->count() == 0))
                throw RequiredError(opt->get_name());
            // Requires
            for(const Option *opt_req : opt->requires_)
                if(opt->count() > 0 && opt_req->count() == 0)
                    throw RequiresError(opt->get_name(), opt_req->get_name());
            // Excludes
            for(const Option *opt_ex : opt->excludes_)
                if(opt->count() > 0 && opt_ex->count() != 0)
                    throw ExcludesError(opt->get_name(), opt_ex->get_name());
        }

        auto selected_subcommands = get_subcommands();
        if(require_subcommand_ < 0 && selected_subcommands.empty())
            throw RequiredError("Subcommand required");
        else if(require_subcommand_ > 0 && static_cast<int>(selected_subcommands.size()) != require_subcommand_)
            throw RequiredError(std::to_string(require_subcommand_) + " subcommand(s) required");

        // Convert missing (pairs) to extras (string only)
        if(parent_ == nullptr) {
            args.resize(missing()->size());
            std::transform(std::begin(*missing()),
                           std::end(*missing()),
                           std::begin(args),
                           [](const std::pair<detail::Classifer, std::string> &val) { return val.second; });
            std::reverse(std::begin(args), std::end(args));

            size_t num_left_over = std::count_if(
                std::begin(*missing()), std::end(*missing()), [](std::pair<detail::Classifer, std::string> &val) {
                    return val.first != detail::Classifer::POSITIONAL_MARK;
                });

            if(num_left_over > 0 && !(allow_extras_ || prefix_command_))
                throw ExtrasError("[" + detail::rjoin(args, " ") + "]");
        }
    }

    /// Parse one ini param, return false if not found in any subcommand, remove if it is
    ///
    /// If this has more than one dot.separated.name, go into the subcommand matching it
    /// Returns true if it managed to find the option, if false you'll need to remove the arg manully.
    bool _parse_ini(std::vector<detail::ini_ret_t> &args) {
        detail::ini_ret_t &current = args.back();
        std::string parent = current.parent(); // respects curent.level
        std::string name = current.name();

        // If a parent is listed, go to a subcommand
        if(parent != "") {
            current.level++;
            for(const App_p &com : subcommands_)
                if(com->check_name(parent))
                    return com->_parse_ini(args);
            return false;
        }

        auto op_ptr = std::find_if(
            std::begin(options_), std::end(options_), [name](const Option_p &v) { return v->check_lname(name); });

        if(op_ptr == std::end(options_))
            return false;

        // Let's not go crazy with pointer syntax
        Option_p &op = *op_ptr;

        if(op->results_.empty()) {
            // Flag parsing
            if(op->get_expected() == 0) {
                if(current.inputs.size() == 1) {
                    std::string val = current.inputs.at(0);
                    val = detail::to_lower(val);
                    if(val == "true" || val == "on" || val == "yes")
                        op->results_ = {""};
                    else if(val == "false" || val == "off" || val == "no")
                        ;
                    else
                        try {
                            size_t ui = std::stoul(val);
                            for(size_t i = 0; i < ui; i++)
                                op->results_.emplace_back("");
                        } catch(const std::invalid_argument &) {
                            throw ConversionError(current.fullname + ": Should be true/false or a number");
                        }
                } else
                    throw ConversionError(current.fullname + ": too many inputs for a flag");
            } else {
                op->results_ = current.inputs;
                op->run_callback();
            }
        }

        args.pop_back();
        return true;
    }

    /// Parse "one" argument (some may eat more than one), delegate to parent if fails, add to missing if missing from
    /// master
    void _parse_single(std::vector<std::string> &args, bool &positional_only) {

        detail::Classifer classifer = positional_only ? detail::Classifer::NONE : _recognize(args.back());
        switch(classifer) {
        case detail::Classifer::POSITIONAL_MARK:
            missing()->emplace_back(classifer, args.back());
            args.pop_back();
            positional_only = true;
            break;
        case detail::Classifer::SUBCOMMAND:
            _parse_subcommand(args);
            break;
        case detail::Classifer::LONG:
            // If already parsed a subcommand, don't accept options_
            _parse_long(args);
            break;
        case detail::Classifer::SHORT:
            // If already parsed a subcommand, don't accept options_
            _parse_short(args);
            break;
        case detail::Classifer::NONE:
            // Probably a positional or something for a parent (sub)command
            _parse_positional(args);
        }
    }

    /// Count the required remaining positional arguments
    size_t _count_remaining_required_positionals() const {
        size_t retval = 0;
        for(const Option_p &opt : options_)
            if(opt->get_positional()
               && opt->get_required()
               && opt->get_expected() > 0
               && static_cast<int>(opt->count()) < opt->get_expected())
                retval = static_cast<size_t>(opt->get_expected()) - opt->count();

        return retval;
    }

    /// Parse a positional, go up the tree to check
    void _parse_positional(std::vector<std::string> &args) {

        std::string positional = args.back();
        for(const Option_p &opt : options_) {
            // Eat options, one by one, until done
            if(opt->get_positional() &&
               (static_cast<int>(opt->count()) < opt->get_expected() || opt->get_expected() < 0)) {

                opt->add_result(positional);
                parse_order_.push_back(opt.get());
                args.pop_back();
                return;
            }
        }

        if(parent_ != nullptr && fallthrough_)
            return parent_->_parse_positional(args);
        else {
            args.pop_back();
            missing()->emplace_back(detail::Classifer::NONE, positional);
            
            if(prefix_command_) {
                while(!args.empty()) {
                    missing()->emplace_back(detail::Classifer::NONE, args.back());
                    args.pop_back();
                }
            }
        }
        
    }

    /// Parse a subcommand, modify args and continue
    ///
    /// Unlike the others, this one will always allow fallthrough
    void _parse_subcommand(std::vector<std::string> &args) {
        if(_count_remaining_required_positionals() > 0)
            return _parse_positional(args);
        for(const App_p &com : subcommands_) {
            if(com->check_name(args.back())) {
                args.pop_back();
                com->_parse(args);
                return;
            }
        }
        if(parent_ != nullptr)
            return parent_->_parse_subcommand(args);
        else
            throw HorribleError("Subcommand " + args.back() + " missing");
    }

    /// Parse a short argument, must be at the top of the list
    void _parse_short(std::vector<std::string> &args) {
        std::string current = args.back();

        std::string name;
        std::string rest;
        if(!detail::split_short(current, name, rest))
            throw HorribleError("Short");

        auto op_ptr = std::find_if(
            std::begin(options_), std::end(options_), [name](const Option_p &opt) { return opt->check_sname(name); });

        // Option not found
        if(op_ptr == std::end(options_)) {
            // If a subcommand, try the master command
            if(parent_ != nullptr && fallthrough_)
                return parent_->_parse_short(args);
            // Otherwise, add to missing
            else {
                args.pop_back();
                missing()->emplace_back(detail::Classifer::SHORT, current);
                return;
            }
        }

        args.pop_back();

        // Get a reference to the pointer to make syntax bearable
        Option_p &op = *op_ptr;

        int num = op->get_expected();

        if(num == 0) {
            op->add_result("");
            parse_order_.push_back(op.get());
        } else if(rest != "") {
            if(num > 0)
                num--;
            op->add_result(rest);
            parse_order_.push_back(op.get());
            rest = "";
        }

        if(num == -1) {
            while(!args.empty() && _recognize(args.back()) == detail::Classifer::NONE) {
                op->add_result(args.back());
                parse_order_.push_back(op.get());
                args.pop_back();
            }
        } else
            while(num > 0 && !args.empty()) {
                num--;
                std::string current_ = args.back();
                args.pop_back();
                op->add_result(current_);
                parse_order_.push_back(op.get());
            }

        if(rest != "") {
            rest = "-" + rest;
            args.push_back(rest);
        }
    }

    /// Parse a long argument, must be at the top of the list
    void _parse_long(std::vector<std::string> &args) {
        std::string current = args.back();

        std::string name;
        std::string value;
        if(!detail::split_long(current, name, value))
            throw HorribleError("Long:" + args.back());

        auto op_ptr = std::find_if(
            std::begin(options_), std::end(options_), [name](const Option_p &v) { return v->check_lname(name); });

        // Option not found
        if(op_ptr == std::end(options_)) {
            // If a subcommand, try the master command
            if(parent_ != nullptr && fallthrough_)
                return parent_->_parse_long(args);
            // Otherwise, add to missing
            else {
                args.pop_back();
                missing()->emplace_back(detail::Classifer::LONG, current);
                return;
            }
        }

        args.pop_back();

        // Get a reference to the pointer to make syntax bearable
        Option_p &op = *op_ptr;

        int num = op->get_expected();

        if(value != "") {
            if(num != -1)
                num--;
            op->add_result(value);
            parse_order_.push_back(op.get());
        } else if(num == 0) {
            op->add_result("");
            parse_order_.push_back(op.get());
        }

        if(num == -1) {
            while(!args.empty() && _recognize(args.back()) == detail::Classifer::NONE) {
                op->add_result(args.back());
                parse_order_.push_back(op.get());
                args.pop_back();
            }
        } else
            while(num > 0 && !args.empty()) {
                num--;
                op->add_result(args.back());
                parse_order_.push_back(op.get());
                args.pop_back();
            }
        return;
    }
};

namespace detail {
/// This class is simply to allow tests access to App's protected functions
struct AppFriend {

    /// Wrap _parse_short, perfectly forward arguments and return
    template <typename... Args>
    static auto parse_short(App *app, Args &&... args) ->
        typename std::result_of<decltype (&App::_parse_short)(App, Args...)>::type {
        return app->_parse_short(std::forward<Args>(args)...);
    }

    /// Wrap _parse_long, perfectly forward arguments and return
    template <typename... Args>
    static auto parse_long(App *app, Args &&... args) ->
        typename std::result_of<decltype (&App::_parse_long)(App, Args...)>::type {
        return app->_parse_long(std::forward<Args>(args)...);
    }

    /// Wrap _parse_subcommand, perfectly forward arguments and return
    template <typename... Args>
    static auto parse_subcommand(App *app, Args &&... args) ->
        typename std::result_of<decltype (&App::_parse_subcommand)(App, Args...)>::type {
        return app->_parse_subcommand(std::forward<Args>(args)...);
    }
};
} // namespace detail

} // namespace CLI
