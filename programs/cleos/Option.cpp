//
// Created by Roman Brod on 5/11/18.
//////////////////////////////////////////////////////////////////////
#include "Option.hpp"
#include "CLI11.hpp"



namespace eosio {

/// Making an option by hand is not defined, it must be made by the CmdPart class
Option::Option(std::string name, std::string description,
               std::function<bool(eosio::results_t)> callback, bool default_,
               eosio::CmdPart *owner)
        : description_(std::move(description)), default_(default_),
          parent_(owner), callback_(std::move(callback))
{
   std::tie(snames_, lnames_, pname_) = aux::get_names(aux::split_names(name));
}



/// Process the callback
void Option::run_callback() const
{
    if(!callback_(results_))
        throw ConversionError(get_name() + "=" + aux::join(results_));

    if(!validators_.empty())
    {
        for(const std::string &result : results_)
        {
            for (const std::function<bool(std::string)> &vali : validators_)
                if (!vali(result))
                    throw ValidationError(get_name() + "=" + result);
        }
    }
}


/// Check a name. Requires "-" or "--" for short / long, supports positional name
bool Option::check_name(std::string name) const
{
    if(name.length() > 2 && name.substr(0, 2) == "--")
        return check_lname(name.substr(2));
    else if(name.length() > 1 && name.substr(0, 1) == "-")
        return check_sname(name.substr(1));
    else
    {
        std::string local_pname = pname_;
        if(ignore_case_)
        {
            local_pname = aux::to_lower(local_pname);
            name = aux::to_lower(name);
        }
        return name == local_pname;
    }
}


/// Requires "-" to be removed from string
bool Option::check_sname(std::string name) const
{
    if(ignore_case_)
    {
        name = aux::to_lower(name);
        return std::find_if(std::begin(snames_), std::end(snames_),
                            [&name](std::string local_sname) { return aux::to_lower(local_sname) == name; })
               != std::end(snames_);
    }
    else
        return std::find(std::begin(snames_), std::end(snames_), name) != std::end(snames_);
}

/// Requires "--" to be removed from string
bool Option::check_lname(std::string name) const
{
    if(ignore_case_)
    {
        name = aux::to_lower(name);
        return std::find_if(std::begin(lnames_), std::end(lnames_),
                            [&name](std::string local_sname) { return aux::to_lower(local_sname) == name; })
               != std::end(lnames_);
    }
    else
        return std::find(std::begin(lnames_), std::end(lnames_), name) != std::end(lnames_);
}


/// Gets a , sep list of names. Does not include the positional name if opt_only=true.
std::string Option::get_name(bool opt_only) const
{
    std::vector<std::string> name_list;
    if(!opt_only && pname_.length() > 0)
        name_list.push_back(pname_);

    for(const std::string &sname : snames_)
        name_list.push_back("-" + sname);

    for(const std::string &lname : lnames_)
        name_list.push_back("--" + lname);

    return aux::join(name_list);
}


/// The name and any extras needed for positionals
std::string Option::help_positional() const
{
    std::string out = pname_;
    if(get_expected() > 1)
        out = out + "(" + std::to_string(get_expected()) + "x)";
    else if(get_expected() == -1)
        out = out + "...";
    out = get_required() ? out : "[" + out + "]";
    return out;
}




/// This is the part after the name is printed but before the description
std::string Option::help_aftername() const
{
    std::stringstream out;

    if(get_expected() != 0)
    {
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

    if(!requires_.empty())
    {
        out << " Requires:";
        for(const Option *opt : requires_)
            out << " " << opt->get_name();
    }

    if(!excludes_.empty())
    {
        out << " Excludes:";
        for(const Option *opt : excludes_)
            out << " " << opt->get_name();
    }
    return out.str();
}


/// If options share any of the same names, they are equal (not counting positional)
bool Option::operator==(const Option &other) const
{
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


/// Set the number of expected arguments (Flags bypass this)
Option * Option::expected(int value)
{
   if (value == 0)
      throw IncorrectConstruction("Cannot set 0 expected, use a flag instead");
   else if (expected_ == 0)
      throw IncorrectConstruction("Cannot make a flag take arguments!");
   else if (!changeable_)
      throw IncorrectConstruction("You can only change the expected arguments for vectors");
   expected_ = value;
   return this;
}

}