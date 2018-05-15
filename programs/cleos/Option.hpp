//
// Created by Roman Brod on 5/11/18.
//////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <set>
#include <sstream>
#include "ParseErrors.hpp"


namespace eosio
{


class CmdPart;

using results_t = std::vector<std::string>;
using callback_t = std::function<bool(results_t)>;

class Option
{
   friend class CmdPart;

protected:
   /// @name Names
   ///@{

   /// A list of the short names (`-a`) without the leading dashes
   std::vector <std::string> snames_;

   /// A list of the long names (`--a`) without the leading dashes
   std::vector <std::string> lnames_;

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

   /// A human readable type value, set when CmdPart creates this
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
   std::vector <std::function<bool(std::string)>> validators_;

   /// A list of options that are required with this option
   std::set<Option *> requires_;

   /// A list of options that are excluded with this option
   std::set<Option *> excludes_;

   ///@}
   /// @name Other
   ///@{

   /// The owner of the Option
   CmdPart *parent_;

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

public:
   /// Making an option by hand is not defined, it must be made by the CmdPart class
   Option(std::string name,
          std::string description = "",
          std::function<bool(results_t)> callback = [](results_t) { return true; },
          bool default_ = true,
          CmdPart *owner = nullptr);
   /// @name Basic
   ///@{

   /// Count the total number of times an option was passed
   size_t count() const
   { return results_.size(); }

   /// This class is true if option is passed.
   operator bool() const
   {
      return count() > 0;
   }

   /// Clear the parsed results (mostly for testing)
   void clear()
   {
      results_.clear();
   }

   ///@}
   /// @name Setting options
   ///@{

   /// Set the option as required
   Option *required(bool value = true)
   {
      required_ = value;
      return this;
   }

   /// Support Plubmum term
   Option *mandatory(bool value = true)
   { return required(value); }

   /// Set the number of expected arguments (Flags bypass this)
   Option *expected(int value);

   /// Adds a validator
   Option *check(std::function<bool(std::string)> validator)
   {

      validators_.push_back(validator);
      return this;
   }

   /// Changes the group membership
   Option *group(std::string name)
   {
      group_ = name;
      return this;
   }

   /// Sets required options
   Option *requires(Option *opt)
   {
      auto tup = requires_.insert(opt);
      if (!tup.second)
         throw OptionAlreadyAdded(get_name() + " requires " + opt->get_name());
      return this;
   }

   /// Find a dependent Option in string if needed
   template<typename T = CmdPart>
   Option *requires(std::string opt_name)
   {
      for (Option *opt : dynamic_cast<T *>(parent_)->options_)
         if (opt != this && opt->check_name(opt_name))
            return requires(opt);
      throw IncorrectConstruction("Option " + opt_name + " is not defined");
   }

   /// Any number supported, any mix of string and Opt
   template<typename A, typename B, typename... ARG>
   Option *requires(A opt, B opt1, ARG... args)
   {
      requires(opt);
      return requires(opt1, args...);
   }

   /// Sets excluded options
   Option *excludes(Option *opt)
   {
      auto tup = excludes_.insert(opt);
      if (!tup.second)
         throw OptionAlreadyAdded(get_name() + " excludes " + opt->get_name());
      return this;
   }

   /// Can find a string if needed
   template<typename T = CmdPart>
   Option *excludes(std::string opt_name)
   {
      for (Option* opt : dynamic_cast<T *>(parent_)->options_)
         if (opt != this && opt->check_name(opt_name))
            return excludes(opt);
      throw IncorrectConstruction("Option " + opt_name + " is not defined");
   }

   /// Any number supported, any mix of string and Opt
   template<typename A, typename B, typename... ARG>
   Option *excludes(A opt, B opt1, ARG... args)
   {
      excludes(opt);
      return excludes(opt1, args...);
   }

   /// Sets environment variable to read if no option given
   Option *envname(std::string name)
   {
      envname_ = name;
      return this;
   }

   /// Ignore case
   ///
   /// The template hides the fact that we don't have the definition of CmdPart yet.
   /// You are never expected to add an argument to the template here.
   template<typename T = CmdPart>
   Option *ignore_case(bool value = true)
   {
      ignore_case_ = value;
      for (const Option* opt : dynamic_cast<T *>(parent_)->options_)
         if (opt != this && *opt == *this)
            throw OptionAlreadyAdded(opt->get_name());
      return this;
   }

   ///@}
   /// @name Accessors
   ///@{

   /// True if this is a required option
   bool get_required() const
   { return required_; }

   /// The number of arguments the option expects
   int get_expected() const
   { return expected_; }

   /// True if this has a default value
   int get_default() const
   { return default_; }

   /// True if the argument can be given directly
   bool get_positional() const
   { return pname_.length() > 0; }

   /// True if option has at least one non-positional name
   bool nonpositional() const
   { return (snames_.size() + lnames_.size()) > 0; }

   /// True if option has description
   bool has_description() const
   { return description_.length() > 0; }

   /// Get the group of this option
   const std::string &get_group() const
   { return group_; }

   /// Get the description
   const std::string &get_description() const
   { return description_; }

   // Just the pname
   std::string get_pname() const
   { return pname_; }

   ///@}
   /// @name Help tools
   ///@{

   /// Gets a , sep list of names. Does not include the positional name if opt_only=true.
   std::string get_name(bool opt_only = false) const;

   /// The name and any extras needed for positionals
   std::string help_positional() const;


   /// The first half of the help print, name plus default, etc
   std::string help_name() const
   {
      std::stringstream out;
      out << get_name(true) << help_aftername();
      return out.str();
   }

   /// pname with type info
   std::string help_pname() const
   {
      std::stringstream out;
      out << get_pname() << help_aftername();
      return out.str();
   }

   /// This is the part after the name is printed but before the description
   std::string help_aftername() const;

   ///@}
   /// @name Parser tools
   ///@{

   /// Process the callback
   void run_callback() const;

   /// If options share any of the same names, they are equal (not counting positional)
   bool operator==(const Option &other) const;

   /// Check a name. Requires "-" or "--" for short / long, supports positional name
   bool check_name(std::string name) const;

   /// Requires "-" to be removed from string
   bool check_sname(std::string name) const;

   /// Requires "--" to be removed from string
   bool check_lname(std::string name) const;

   /// Puts a result at position r
   void add_result(std::string s)
   {
      results_.push_back(s);
      callback_run_ = false;
   }

   /// Get a copy of the results
   std::vector <std::string> results() const
   { return results_; }

   /// See if the callback has been run already
   bool get_callback_run() const
   { return callback_run_; }

   ///@}
   /// @name Custom options
   ///@{

   /// Set a custom option, typestring, expected, and changeable
   void set_custom_option(std::string typeval, int expected = 1, bool changeable = false)
   {
      typeval_ = typeval;
      expected_ = expected;
      changeable_ = changeable;
   }

   /// Set the default value string representation
   void set_default_val(std::string val)
   { defaultval_ = val; }


   /// Set the type name displayed on this option
   void set_type_name(std::string val)
   { typeval_ = val; }

   ///@}

protected:
   /// @name CmdPart Helpers
   ///@{
   /// Can print positional name detailed option if true
   bool _has_help_positional() const
   {
      return get_positional() && (has_description() || !requires_.empty() || !excludes_.empty());
   }
   ///@}
};

}
