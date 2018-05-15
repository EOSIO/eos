//
// Created by Roman Brod on 5/11/18.
//////////////////////////////////////////////////////////////////////
#pragma once
#include "Option.hpp"
#include "CLI11.hpp"


namespace eosio {

/// Creates one node of a tree of nodes representing command line program.
/// Each path (from a leaf to the root) represents different functionality
/// of a program. Each instance in that tree combines a number of options
/// and flags that can be used to configure the execution of a command.
class CmdPart
{
   friend class AppFriend;

   enum class Classifer
   {
      NONE, POSITIONAL_MARK, SHORT, LONG, SUBCOMMAND
   };

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
   std::vector<Option*> options_;

   /// A pointer to the help flag if there is one
   Option *help_ptr_{nullptr};

   ///@}
   /// @name Parsing
   ///@{

   using missing_t = std::vector<std::pair<Classifer, std::string>>;

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
   std::vector<CmdPart*> subcommands_;

   /// If true, the program name is not case sensitive
   bool ignore_case_{false};

   /// Allow subcommand fallthrough, so that parent commands can collect commands after subcommand.
   bool fallthrough_{false};

   /// A pointer to the parent if this is a subcommand
   CmdPart* parent_{nullptr};

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
   CmdPart(std::string description_, bool help, aux::enabler) : description_(std::move(description_))
   {
      if (help)
         help_ptr_ = add_flag("-h,--help", "Print this help message and exit");
   }

public:
   /// @name Basic
   ///@{

   /// Create a new program. Pass in the same arguments as main(), along with a help string.
   CmdPart(std::string description_ = "", bool help = true)
           : CmdPart(description_, help, aux::dummy)
   {}

   /// Set a callback for the end of parsing.
   ///
   /// Due to a bug in c++11,
   /// it is not possible to overload on std::function (fixed in c++14
   /// and backported to c++11 on newer compilers). Use capture by reference
   /// to get a pointer to CmdPart if needed.
   CmdPart* set_callback(std::function<void()> callback) {
      callback_ = callback;
      return this;
   }

   /// Remove the error when extras are left over on the command .
   CmdPart* allow_extras(bool allow = true)
   {
      allow_extras_ = allow;
      return this;
   }

   /// Do not parse anything after the first unrecongnised option and return
   CmdPart* prefix_command(bool allow = true)
   {
      prefix_command_ = allow;
      return this;
   }

   /// Ignore case. Subcommand inherit value.
   CmdPart* ignore_case(bool value = true)
   {
      ignore_case_ = value;
      if(parent_ != nullptr)
      {
         for(const auto subc : parent_->subcommands_)
         {
            if(subc != this && (check_name(subc->name_) || subc->check_name(name_)))
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
   /// Does not return a pointer since it is supposed to be called on the main CmdPart.
   CmdPart* require_subcommand(int value = -1) {
      require_subcommand_ = value;
      return this;
   }

   /// Stop subcommand fallthrough, so that parent commands cannot collect commands after subcommand.
   /// Default from parent, usually set on parent.
   CmdPart* fallthrough(bool value = true) {
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
  Option *add_option(std::string name, callback_t callback, std::string description = "", bool defaulted = false);



   /// Add option for non-vectors (duplicate copy needed without defaulted to avoid `iostream << value`)
   template <typename T, aux::enable_if_t<!aux::is_vector<T>::value, aux::enabler> = aux::dummy>
   Option *add_option(std::string name,
                      T &variable, ///< The variable to set
                      std::string description = "") {

      callback_t fun = [&variable](results_t res) {
         if(res.size() != 1)
            return false;
         return aux::lexical_cast(res[0], variable);
      };

      Option *opt = add_option(name, fun, description, false);
      opt->set_custom_option(aux::type_name<T>());
      return opt;
   }

   /// Add option for non-vectors with a default print
   template <typename T, aux::enable_if_t<!aux::is_vector<T>::value, aux::enabler> = aux::dummy>
   Option *add_option(std::string name,
                      T &variable, ///< The variable to set
                      std::string description,
                      bool defaulted) {

      callback_t fun = [&variable](results_t res) {
         if(res.size() != 1)
            return false;
         return aux::lexical_cast(res[0], variable);
      };

      Option *opt = add_option(name, fun, description, defaulted);
      opt->set_custom_option(aux::type_name<T>());
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

      callback_t fun = [&variable](results_t res) {
         bool retval = true;
         variable.clear();
         for(const auto &a : res) {
            variable.emplace_back();
            retval &= aux::lexical_cast(a, variable.back());
         }
         return (!variable.empty()) && retval;
      };

      Option *opt = add_option(name, fun, description, false);
      opt->set_custom_option(aux::type_name<T>(), -1, true);
      return opt;
   }

   /// Add option for vectors
   template <typename T>
   Option *add_option(std::string name,
                      std::vector<T> &variable, ///< The variable vector to set
                      std::string description,
                      bool defaulted) {

      callback_t fun = [&variable](results_t res) {
         bool retval = true;
         variable.clear();
         for(const auto &a : res) {
            variable.emplace_back();
            retval &= aux::lexical_cast(a, variable.back());
         }
         return (!variable.empty()) && retval;
      };

      Option *opt = add_option(name, fun, description, defaulted);
      opt->set_custom_option(aux::type_name<T>(), -1, true);
      if(defaulted)
         opt->set_default_val("[" + aux::join(variable) + "]");
      return opt;
   }

   /// Add option for flag
   Option *add_flag(std::string name, std::string description = "") {
      callback_t fun = [](results_t) { return true; };

      Option *opt = add_option(name, fun, description, false);
      if(opt->get_positional())
         throw IncorrectConstruction("Flags cannot be positional");
      opt->set_custom_option("", 0);
      return opt;
   }

   /// Add option for flag integer
   template <typename T,
           aux::enable_if_t<std::is_integral<T>::value && !aux::is_bool<T>::value, aux::enabler> = aux::dummy>
   Option *add_flag(std::string name,
                    T &count, ///< A varaible holding the count
                    std::string description = "") {

      count = 0;
      callback_t fun = [&count](results_t res) {
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
   template <typename T, aux::enable_if_t<aux::is_bool<T>::value, aux::enabler> = aux::dummy>
   Option* add_flag(std::string name,
                    T &count, ///< A varaible holding true if passed
                    std::string description = "") {

      count = false;
      callback_t fun = [&count](results_t res) {
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
   Option* add_set(std::string name,
                   T &member,           ///< The selected member of the set
                   std::set<T> options, ///< The set of posibilities
                   std::string description = "") {

      callback_t fun = [&member, options](results_t res) {
         if(res.size() != 1) {
            return false;
         }
         bool retval = aux::lexical_cast(res[0], member);
         if(!retval)
            return false;
         return std::find(std::begin(options), std::end(options), member) != std::end(options);
      };

      Option *opt = add_option(name, fun, description, false);
      std::string typeval = aux::type_name<T>();
      typeval += " in {" + aux::join(options) + "}";
      opt->set_custom_option(typeval);
      return opt;
   }

   /// Add set of options
   template <typename T>
   Option* add_set(std::string name,
                   T &member,           ///< The selected member of the set
                   std::set<T> options, ///< The set of posibilities
                   std::string description,
                   bool defaulted) {

      callback_t fun = [&member, options](results_t res) {
         if(res.size() != 1) {
            return false;
         }
         bool retval = aux::lexical_cast(res[0], member);
         if(!retval)
            return false;
         return std::find(std::begin(options), std::end(options), member) != std::end(options);
      };

      Option *opt = add_option(name, fun, description, defaulted);
      std::string typeval = aux::type_name<T>();
      typeval += " in {" + aux::join(options) + "}";
      opt->set_custom_option(typeval);
      if(defaulted) {
         std::stringstream out;
         out << member;
         opt->set_default_val(out.str());
      }
      return opt;
   }

   /// Add set of options, string only, ignore case (no default)
   Option* add_set_ignore_case(std::string name,
                               std::string &member,           ///< The selected member of the set
                               std::set<std::string> options, ///< The set of posibilities
                               std::string description = "");

   /// Add set of options, string only, ignore case
   Option *add_set_ignore_case(std::string name,
                               std::string &member,           ///< The selected member of the set
                               std::set<std::string> options, ///< The set of posibilities
                               std::string description,
                               bool defaulted) {
      callback_t fun = [&member, options](results_t res) {
         if(res.size() != 1) {
            return false;
         }
         member = aux::to_lower(res[0]);
         auto iter = std::find_if(std::begin(options), std::end(options), [&member](std::string val) {
            return aux::to_lower(val) == member;
         });
         if(iter == std::end(options))
            return false;
         else {
            member = *iter;
            return true;
         }
      };

      Option *opt = add_option(name, fun, description, defaulted);
      std::string typeval = aux::type_name<std::string>();
      typeval += " in {" + aux::join(options) + "}";
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
      callback_t fun = [&variable](results_t res) {
         if(res.size() != 2)
            return false;
         double x, y;
         bool worked = aux::lexical_cast(res[0], x) && aux::lexical_cast(res[1], y);
         if(worked)
            variable = T(x, y);
         return worked;
      };

      Option *opt = add_option(name, fun, description, defaulted);
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

   /// Removes an option from the CmdPart. Takes an option pointer. Returns true if found and removed.
   bool remove_option(Option *opt) {
      auto iterator =
              std::find_if(std::begin(options_), std::end(options_), [opt](const Option* v) { return v == opt; });
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
   CmdPart* add_subcommand(std::string name, std::string description = "", bool help = true);

   /// Check to see if a subcommand is part of this command (doesn't have to be in command line)
   const CmdPart* get_subcommand(CmdPart* cmd) const
   {
      for(const CmdPart* subcmd : subcommands_)
         if(subcmd == cmd)
            return cmd;
      throw OptionNotFound(cmd->get_name());
   }

   /// Check to see if a subcommand is part of this command (text version)
   const CmdPart *get_subcommand(std::string cmdname) const
   {
      for (const CmdPart* cmd : subcommands_)
         if(cmd->check_name(cmdname))
            return cmd;
      throw OptionNotFound(cmdname);
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
   void reset()
   {

      parsed_ = false;
      missing_.clear();

      for(Option* opt : options_)
         opt->clear();

      for(CmdPart* cmd : subcommands_)
         cmd->reset();
   }

   ///@}
   /// @name Post parsing
   ///@{

   /// Counts the number of times the given option was passed.
   int count(std::string name) const
   {
      for(const Option* opt : options_) {
         if(opt->check_name(name))
         {
            return opt->count();
         }
      }
      throw OptionNotFound(name);
   }

   /// Get a subcommand pointer list to the currently selected subcommands (after parsing)
   std::vector<CmdPart *> get_subcommands() const
   {
      std::vector<CmdPart *> cmd;
      for(CmdPart* cmdpart : subcommands_)
         if(cmdpart->parsed_)
            cmd.push_back(cmdpart);
      return cmd;
   }

   /// Check to see if given subcommand was selected
   bool got_subcommand(CmdPart* subcom) const
   {
      // get subcom needed to verify that this was a real subcommand
      return get_subcommand(subcom)->parsed_;
   }

   /// Check with name instead of pointer to see if subcommand was selected
   bool got_subcommand(std::string name) const
   {
      return get_subcommand(name)->parsed_;
   }

   ///@}
   /// @name Help
   ///@{

   /// Produce a string that could be read in as a config of the current values of the CmdPart.
   /// Set default_also to include default arguments.
   /// Prefix will add a string to the beginning of each option.
   std::string config_to_str(bool default_also = false, std::string prefix = "") const;

   /// Makes a help message, with a column wid for column 1
   std::string help(size_t wid = 30, std::string prev = "") const;


   ///@}
   /// @name Getters
   ///@{

   /// Get a pointer to the help flag.
   Option* get_help_ptr() { return help_ptr_; }

   /// Get a pointer to the help flag. (const)
   const Option* get_help_ptr() const { return help_ptr_; }

   /// Get a pointer to the config option.
   Option* get_config_ptr() { return config_ptr_; }

   /// Get a pointer to the config option. (const)
   const Option* get_config_ptr() const { return config_ptr_; }
   /// Get the name of the current CmdPart
   std::string get_name() const { return name_; }

   /// Check the name, case insensitive if set
   bool check_name(std::string name_to_check) const
   {
      std::string local_name = name_;
      if(ignore_case_)
      {
         local_name = aux::to_lower(name_);
         name_to_check = aux::to_lower(name_to_check);
      }

      return local_name == name_to_check;
   }

   /// This gets a vector of pointers with the original parse order
   const std::vector<Option *> &parse_order() const
   {
      return parse_order_;
   }

   ///@}

protected:
   /// Check the options to make sure there are no conficts.
   ///
   /// Currenly checks to see if mutiple positionals exist with -1 args
   void _validate() const
   {
      auto count = std::count_if(std::begin(options_), std::end(options_), [](const Option* opt) {
         return opt->get_expected() == -1 && opt->get_positional();
      });

      if(count > 1)
         throw InvalidError(name_ + ": Too many positional arguments with unlimited expected args");
      for(const CmdPart* cmd : subcommands_)
         cmd->_validate();
   }

   /// Return missing from the master
   missing_t* missing()
   {
      if(parent_ != nullptr)
         return parent_->missing();
      return &missing_;
   }

   /// Internal function to run CmdPart callback, top down
   void run_callback()
   {
      pre_callback();
      if(callback_)
         callback_();
      for(CmdPart* subc : get_subcommands()) {
         subc->run_callback();
      }
   }

   bool _valid_subcommand(const std::string &current) const
   {
      for(const CmdPart* com : subcommands_)
         if(com->check_name(current) && !*com)
            return true;
      if(parent_ != nullptr)
         return parent_->_valid_subcommand(current);
      else
         return false;
   }

   /// Selects a Classifer enum based on the type of the current argument
   Classifer _recognize(const std::string &current) const;

   /// Internal parse function
   void _parse(std::vector<std::string> &args);



   /// Parse one ini param, return false if not found in any subcommand, remove if it is
   ///
   /// If this has more than one dot.separated.name, go into the subcommand matching it
   /// Returns true if it managed to find the option, if false you'll need to remove the arg manully.
   bool _parse_ini(std::vector<aux::ini_ret_t> &args);

   /// Parse "one" argument (some may eat more than one), delegate to parent if fails, add to missing if missing from
   /// master
   void _parse_single(std::vector<std::string> &args, bool &positional_only);

   /// Count the required remaining positional arguments
   size_t _count_remaining_required_positionals() const {
      size_t retval = 0;
      for(const Option* opt : options_)
         if(opt->get_positional()
            && opt->get_required()
            && opt->get_expected() > 0
            && static_cast<int>(opt->count()) < opt->get_expected())
            retval = static_cast<size_t>(opt->get_expected()) - opt->count();

      return retval;
   }

   /// Parse a positional, go up the tree to check
   void _parse_positional(std::vector<std::string> &args);

   /// Parse a subcommand, modify args and continue
   ///
   /// Unlike the others, this one will always allow fallthrough
   void _parse_subcommand(std::vector<std::string> &args);

   /// Parse a short argument, must be at the top of the list
   void _parse_short(std::vector<std::string> &args);

   /// Parse a long argument, must be at the top of the list
   void _parse_long(std::vector<std::string> &args);
};



/// This class is simply to allow tests access to protected functions of CmdPart
class AppFriend
{
public:
   /// Wrap _parse_short, perfectly forward arguments and return
   template <typename... Args>
   static auto parse_short(CmdPart *app, Args &&... args) ->
   typename std::result_of<decltype (&CmdPart::_parse_short)(CmdPart, Args...)>::type
   {
      return app->_parse_short(std::forward<Args>(args)...);
   }

   /// Wrap _parse_long, perfectly forward arguments and return
   template <typename... Args>
   static auto parse_long(CmdPart *app, Args &&... args) ->
   typename std::result_of<decltype (&CmdPart::_parse_long)(CmdPart, Args...)>::type
   {
      return app->_parse_long(std::forward<Args>(args)...);
   }

   /// Wrap _parse_subcommand, perfectly forward arguments and return
   template <typename... Args>
   static auto parse_subcommand(CmdPart *app, Args &&... args) ->
   typename std::result_of<decltype (&CmdPart::_parse_subcommand)(CmdPart, Args...)>::type
   {
      return app->_parse_subcommand(std::forward<Args>(args)...);
   }
};



}