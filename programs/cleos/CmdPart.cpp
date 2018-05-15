//
// Created by Roman Brod on 5/11/18.
//////////////////////////////////////////////////////////////////////
#include "CmdPart.hpp"


namespace eosio {


// Add a subcommand. Like the constructor, you can override the help message addition by setting help=false
CmdPart* CmdPart::add_subcommand(std::string name, std::string description, bool help)
{
   subcommands_.emplace_back(new CmdPart(description, help, aux::dummy));
   subcommands_.back()->name_ = name;
   subcommands_.back()->allow_extras();
   subcommands_.back()->parent_ = this;
   subcommands_.back()->ignore_case_ = ignore_case_;
   subcommands_.back()->fallthrough_ = fallthrough_;
   for(const auto subc : subcommands_)
      if(subc != subcommands_.back())
         if(subc->check_name(subcommands_.back()->name_) || subcommands_.back()->check_name(subc->name_))
            throw OptionAlreadyAdded(subc->name_);
   return subcommands_.back();
}


Option* CmdPart::add_option(std::string name, callback_t callback, std::string description, bool defaulted)
{
   Option* opt = new Option(name, description, callback, defaulted, this);

   if(std::find_if(std::begin(options_), std::end(options_),
                   [opt](const Option* v) { return v == opt; }) == std::end(options_))
   {
      options_.emplace_back(opt);
      //Option* option = options_.back();
      //option.reset(new Option(name, description, callback, defaulted, this));
      return opt;
   }
   else
      throw OptionAlreadyAdded(name);
}


/// Add set of options, string only, ignore case (no default)
Option * CmdPart::add_set_ignore_case(std::string name,
                            std::string &member,           ///< The selected member of the set
                            std::set<std::string> options, ///< The set of posibilities
                            std::string description)
{
   callback_t fun = [&member, options](results_t res)
   {
      if(res.size() != 1)
         return false;

      member = aux::to_lower(res[0]);
      auto iter = std::find_if(std::begin(options), std::end(options),
                               [&member](std::string val) { return aux::to_lower(val) == member; });

      if(iter == std::end(options))
         return false;
      else
      {
         member = *iter;
         return true;
      }
   };

   Option *opt = add_option(name, fun, description, false);
   std::string typeval = aux::type_name<std::string>();
   typeval += " in {" + aux::join(options) + "}";
   opt->set_custom_option(typeval);

   return opt;
}


// Produce a string that could be read in as a config of the current values of the App. Set default_also to include
// default arguments. Prefix will add a string to the beginning of each option.
std::string CmdPart::config_to_str(bool default_also, std::string prefix) const
{
   std::stringstream out;
   for(const Option* opt : options_)
   {
      // Only process option with a long-name
      if(!opt->lnames_.empty()) {
         std::string name = prefix + opt->lnames_[0];

         // Non-flags
         if(opt->get_expected() != 0) {

            // If the option was found on command line
            if(opt->count() > 0)
               out << name << "=" << aux::inijoin(opt->results()) << std::endl;

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
         } else if(opt->count() == 0 && default_also && opt != get_help_ptr()) {
            out << name << "=false" << std::endl;
         }
      }
   }

   for(const CmdPart* subcom : subcommands_)
      out << subcom->config_to_str(default_also, prefix + subcom->name_ + ".");
   return out.str();
}


/// Makes a help message, with a column wid for column 1
std::string CmdPart::help(size_t wid, std::string prev) const
{
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
   for(const Option* opt : options_)
   {
      if(opt->nonpositional())
      {
         npos = true;
         groups.insert(opt->get_group());
      }
   }

   if(npos)
      out << " [OPTIONS]";

   // Positionals
   bool pos = false;
   for(const Option* opt : options_)
   {
      if (opt->get_positional())
      {
         // A hidden positional should still show up in the usage statement
         // if(detail::to_lower(opt->get_group()) == "hidden")
         //    continue;
         out << " " << opt->help_positional();
         if (opt->_has_help_positional())
            pos = true;
      }
   }

   if(!subcommands_.empty())
   {
      if(require_subcommand_ != 0)
         out << " SUBCOMMAND";
      else
         out << " [SUBCOMMAND]";
   }

   out << std::endl << std::endl;

   // Positional descriptions
   if(pos)
   {
      out << "Positionals:" << std::endl;
      for(const Option* opt : options_)
      {
         if(aux::to_lower(opt->get_group()) == "hidden")
            continue;
         if(opt->_has_help_positional())
            aux::format_help(out, opt->help_pname(), opt->get_description(), wid);
      }
      out << std::endl;
   }

   // Options
   if(npos)
   {
      for(const std::string &group : groups)
      {
         if(aux::to_lower(group) == "hidden")
            continue;

         out << group << ":" << std::endl;
         for(const Option* opt : options_)
         {
            if(opt->nonpositional() && opt->get_group() == group)
               aux::format_help(out, opt->help_name(), opt->get_description(), wid);
         }
         out << std::endl;
      }
   }

   // Subcommands
   if (!subcommands_.empty())
   {
      out << "Subcommands:" << std::endl;
      for(const CmdPart* com : subcommands_)
         aux::format_help(out, com->get_name(), com->description_, wid);
   }

   return out.str();
}


/// Internal parse function
void CmdPart::_parse(std::vector<std::string> &args)
{
   parsed_ = true;
   bool positional_only = false;

   while (!args.empty()) {
      _parse_single(args, positional_only);
   }

   if (help_ptr_ != nullptr && help_ptr_->count() > 0) {
      throw CallForHelp();
   }

   // Process an INI file
   if(config_ptr_ != nullptr && config_name_ != "")
   {
      try
      {
         std::vector<aux::ini_ret_t> values = aux::parse_ini(config_name_);
         while(!values.empty())
         {
            if(!_parse_ini(values))
            {
               throw ExtrasINIError(values.back().fullname);
            }
         }
      }
      catch(const FileError &)
      {
         if(config_required_)
            throw;
      }
   }

   // Get envname options if not yet passed
   for (Option* opt : options_)
   {
      if(opt->count() == 0 && opt->envname_ != "")
      {
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

         if(!ename_string.empty())
            opt->add_result(ename_string);
      }
   }

   // Process callbacks
   for (Option* opt : options_)
   {
      if(opt->count() > 0 && !opt->get_callback_run())
         opt->run_callback();
   }

   // Verify required options
   for(Option* opt : options_)
   {
      // Required
      if(opt->get_required() && (opt->count() < opt->get_expected() || opt->count() == 0))
         throw RequiredError(opt->get_name());

      // Requires
      for (const Option *opt_req : opt->requires_)
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
   else if(require_subcommand_ > 0
           && static_cast<int>(selected_subcommands.size()) != require_subcommand_)
   {
      throw RequiredError(std::to_string(require_subcommand_) + " subcommand(s) required");
   }

   // Convert missing (pairs) to extras (string only)
   if(parent_ == nullptr)
   {
      args.resize(missing()->size());
      std::transform(std::begin(*missing()),
                     std::end(*missing()),
                     std::begin(args),
                     [](const std::pair<Classifer, std::string> &val) { return val.second; });
      std::reverse(std::begin(args), std::end(args));

      size_t num_left_over = std::count_if(std::begin(*missing()), std::end(*missing()),
              [](std::pair<Classifer, std::string> &val) {
                 return val.first != Classifer::POSITIONAL_MARK; });

      if(num_left_over > 0 && !(allow_extras_ || prefix_command_))
         throw ExtrasError("[" + aux::rjoin(args, " ") + "]");
   }
}


// Parse a subcommand, modify args and continue
// Unlike the others, this one will always allow fallthrough
void CmdPart::_parse_subcommand(std::vector<std::string> &args)
{
   if(_count_remaining_required_positionals() > 0)
      return _parse_positional(args);

   for(CmdPart* com : subcommands_)
   {
      if(com->check_name(args.back()))
      {
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


// Selects a Classifer enum based on the type of the current argument
CmdPart::Classifer CmdPart::_recognize(const std::string &current) const
{
   std::string dummy1, dummy2;

   if(current == "--")
      return Classifer::POSITIONAL_MARK;

   if(_valid_subcommand(current))
      return Classifer::SUBCOMMAND;

   if(aux::split_long(current, dummy1, dummy2))
      return Classifer::LONG;

   if(aux::split_short(current, dummy1, dummy2))
      return Classifer::SHORT;

   return Classifer::NONE;
}


// Parse "one" argument (some may eat more than one), delegate to parent if fails,
// add to missing if missing from master
void CmdPart::_parse_single(std::vector<std::string> &args, bool &positional_only)
{
   Classifer classifer = positional_only ? Classifer::NONE : _recognize(args.back());
   switch(classifer)
   {
      case Classifer::POSITIONAL_MARK:
         missing()->emplace_back(classifer, args.back());
         args.pop_back();
         positional_only = true;
         break;

      case Classifer::SUBCOMMAND:
         _parse_subcommand(args);
         break;

      case Classifer::LONG:
         // If already parsed a subcommand, don't accept options_
         _parse_long(args);
         break;

      case Classifer::SHORT:
         // If already parsed a subcommand, don't accept options_
         _parse_short(args);
         break;

      case Classifer::NONE:
         // Probably a positional or something for a parent (sub)command
         _parse_positional(args);
   }
}


/// Parse a positional, go up the tree to check
void CmdPart::_parse_positional(std::vector<std::string> &args)
{

   std::string positional = args.back();
   for(Option* opt : options_)
   {
      // Eat options, one by one, until done
      if(opt->get_positional() &&
         (opt->count() < opt->get_expected() || opt->get_expected() < 0))
      {

         opt->add_result(positional);
         parse_order_.push_back(opt);
         args.pop_back();
         return;
      }
   }

   if(parent_ != nullptr && fallthrough_)
      return parent_->_parse_positional(args);
   else
   {
      args.pop_back();
      missing()->emplace_back(Classifer::NONE, positional);

      if(prefix_command_)
      {
         while(!args.empty())
         {
            missing()->emplace_back(Classifer::NONE, args.back());
            args.pop_back();
         }
      }
   }
}


/// Parse a short argument, must be at the top of the list
void CmdPart::_parse_short(std::vector<std::string> &args)
{
   std::string current = args.back();

   std::string name;
   std::string rest;
   if(!aux::split_short(current, name, rest))
      throw HorribleError("Short");

   auto op_ptr = std::find_if(
           std::begin(options_), std::end(options_),
           [name](const Option* opt) { return opt->check_sname(name); });

   // Option not found
   if(op_ptr == std::end(options_))
   {
      // If a subcommand, try the master command
      if(parent_ != nullptr && fallthrough_)
         return parent_->_parse_short(args);
         // Otherwise, add to missing
      else
      {
         args.pop_back();
         missing()->emplace_back(Classifer::SHORT, current);
         return;
      }
   }

   args.pop_back();

   // Get a reference to the pointer to make syntax bearable
   Option* op = *op_ptr;

   int num = op->get_expected();

   if(num == 0)
   {
      op->add_result("");
      parse_order_.push_back(op);
   }
   else if(rest != "")
   {
      if(num > 0)
         num--;
      op->add_result(rest);
      parse_order_.push_back(op);
      rest = "";
   }

   if(num == -1) {
      while(!args.empty() && _recognize(args.back()) == Classifer::NONE)
      {
         op->add_result(args.back());
         parse_order_.push_back(op);
         args.pop_back();
      }
   }
   else
   {
      while (num > 0 && !args.empty())
      {
        num--;
         std::string current_ = args.back();
         args.pop_back();
         op->add_result(current_);
         parse_order_.push_back(op);
      }
   }

   if(rest != "")
   {
      rest = "-" + rest;
      args.push_back(rest);
   }
}

/// Parse a long argument, must be at the top of the list
void CmdPart::_parse_long(std::vector<std::string> &args)
{
   std::string current = args.back();

   std::string name;
   std::string value;
   if(!aux::split_long(current, name, value))
      throw HorribleError("Long:" + args.back());

   auto op_ptr = std::find_if(
           std::begin(options_), std::end(options_),
           [name](const Option* v) { return v->check_lname(name); });

   // Option not found
   if(op_ptr == std::end(options_))
   {
      // If a subcommand, try the master command
      if(parent_ != nullptr && fallthrough_)
         return parent_->_parse_long(args);
         // Otherwise, add to missing
      else
      {
         args.pop_back();
         missing()->emplace_back(Classifer::LONG, current);
         return;
      }
   }

   args.pop_back();

   // Get a reference to the pointer to make syntax bearable
   Option* op = *op_ptr;

   int num = op->get_expected();

   if(value != "")
   {
      if(num != -1)
         num--;
      op->add_result(value);
      parse_order_.push_back(op);
   }
   else if(num == 0)
   {
      op->add_result("");
      parse_order_.push_back(op);
   }

   if(num == -1)
   {
      while(!args.empty() && _recognize(args.back()) == Classifer::NONE)
      {
         op->add_result(args.back());
         parse_order_.push_back(op);
         args.pop_back();
      }
   }
   else
      while(num > 0 && !args.empty())
      {
         num--;
         op->add_result(args.back());
         parse_order_.push_back(op);
         args.pop_back();
      }
   return;
}


/// Parse one ini param, return false if not found in any subcommand, remove if it is
///
/// If this has more than one dot.separated.name, go into the subcommand matching it
/// Returns true if it managed to find the option, if false you'll need to remove the arg manully.
bool CmdPart::_parse_ini(std::vector<aux::ini_ret_t> &args)
{
   aux::ini_ret_t &current = args.back();
   std::string parent = current.parent(); // respects curent.level
   std::string name = current.name();

   // If a parent is listed, go to a subcommand
   if(parent != "")
   {
      current.level++;
      for(CmdPart* com : subcommands_)
      {
         if (com->check_name(parent))
            return com->_parse_ini(args);
      }
      return false;
   }

   auto op_ptr = std::find_if(
           std::begin(options_), std::end(options_),
           [name](const Option* v) { return v->check_lname(name); });

   if(op_ptr == std::end(options_))
      return false;

   // Let's not go crazy with pointer syntax
   Option* op = *op_ptr;

   if(op->results_.empty())
   {
      // Flag parsing
      if(op->get_expected() == 0)
      {
         if(current.inputs.size() == 1)
         {
            std::string val = current.inputs.at(0);
            val = aux::to_lower(val);
            if(val == "true" || val == "on" || val == "yes")
               op->results_ = {""};
            else if(val == "false" || val == "off" || val == "no")
               ;
            else
            {
               try
               {
                  size_t ui = std::stoul(val);
                  for (size_t i = 0; i < ui; i++)
                     op->results_.emplace_back("");
               }
               catch (const std::invalid_argument &)
               {
                  throw ConversionError(current.fullname + ": Should be true/false or a number");
               }
            }
         }
         else
            throw ConversionError(current.fullname + ": too many inputs for a flag");
      }
      else
      {
         op->results_ = current.inputs;
         op->run_callback();
      }
   }

   args.pop_back();
   return true;
}


}
