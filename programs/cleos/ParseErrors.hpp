//
// Created by Roman Brod on 5/11/18.
//////////////////////////////////////////////////////////////////////
#pragma once

namespace eosio {


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
struct Error : public std::runtime_error
{
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
struct ConstructionError : public Error
{
   // Using Error::Error constructors seem to not work on GCC 4.7
   ConstructionError(std::string parent,
                     std::string name,
                     ExitCodes exit_code = ExitCodes::BaseClass,
                     bool print_help = true)
           : Error(parent, name, exit_code, print_help) {}
};

/// Thrown when an option is set to conflicting values (non-vector and multi args, for example)
struct IncorrectConstruction : public ConstructionError
{
   IncorrectConstruction(std::string name)
      : ConstructionError("IncorrectConstruction", name, ExitCodes::IncorrectConstruction)
   {}
};

/// Thrown on construction of a bad name
struct BadNameString : public ConstructionError
{
   BadNameString(std::string name)
           : ConstructionError("BadNameString", name, ExitCodes::BadNameString)
   {}
};

/// Thrown when an option already exists
struct OptionAlreadyAdded : public ConstructionError
{
   OptionAlreadyAdded(std::string name)
           : ConstructionError("OptionAlreadyAdded", name, ExitCodes::OptionAlreadyAdded)
   {}
};

// Parsing errors

/// Anything that can error in Parse
struct ParseError : public Error
{
   ParseError(std::string parent, std::string name,
              ExitCodes exit_code = ExitCodes::BaseClass, bool print_help = true)
           : Error(parent, name, exit_code, print_help)
   {}
};

// Not really "errors"

/// This is a successful completion on parsing, supposed to exit
struct Success : public ParseError
{
   Success() : ParseError("Success", "Successfully completed, should be caught and quit",
                          ExitCodes::Success, false)
   {}
};

/// -h or --help on command line
struct CallForHelp : public ParseError
{
   CallForHelp()
      : ParseError("CallForHelp",
                   "This should be caught in your main function, see examples", ExitCodes::Success)
   {}
};

/// Thrown when parsing an INI file and it is missing
struct FileError : public ParseError
{
   FileError(std::string name) : ParseError("FileError", name, ExitCodes::File)
   {}
};

/// Thrown when conversion call back fails, such as when an int fails to coerse to a string
struct ConversionError : public ParseError
{
   ConversionError(std::string name)
      : ParseError("ConversionError", name, ExitCodes::Conversion)
   {}
};

/// Thrown when validation of results fails
struct ValidationError : public ParseError
{
   ValidationError(std::string name)
           : ParseError("ValidationError", name, ExitCodes::Validation)
   {}
};

/// Thrown when a required option is missing
struct RequiredError : public ParseError
{
   RequiredError(std::string name)
           : ParseError("RequiredError", name, ExitCodes::Required)
   {}
};

/// Thrown when a requires option is missing
struct RequiresError : public ParseError {
   RequiresError(std::string name, std::string subname)
      : ParseError("RequiresError", name + " requires " + subname, ExitCodes::Requires)
   {}
};

/// Thrown when a exludes option is present
struct ExcludesError : public ParseError
{
   ExcludesError(std::string name, std::string subname)
      : ParseError("ExcludesError", name + " excludes " + subname, ExitCodes::Excludes)
   {}
};

/// Thrown when too many positionals or options are found
struct ExtrasError : public ParseError
{
   ExtrasError(std::string name)
           : ParseError("ExtrasError", name, ExitCodes::Extras)
   {}
};

/// Thrown when extra values are found in an INI file
struct ExtrasINIError : public ParseError
{
   ExtrasINIError(std::string name)
           : ParseError("ExtrasINIError", name, ExitCodes::ExtrasINI)
   {}
};

/// Thrown when validation fails before parsing
struct InvalidError : public ParseError
{
   InvalidError(std::string name)
           : ParseError("InvalidError", name, ExitCodes::Invalid)
   {}
};

/// This is just a safety check to verify selection and parsing match
struct HorribleError : public ParseError
{
   HorribleError(std::string name)
      : ParseError("HorribleError",
                   "(You should never see this error) " + name, ExitCodes::Horrible)
   {}
};

// After parsing

/// Thrown when counting a non-existent option
struct OptionNotFound : public Error
{
   OptionNotFound(std::string name)
           : Error("OptionNotFound", name, ExitCodes::OptionNotFound)
   {}
};

/// @}


}




