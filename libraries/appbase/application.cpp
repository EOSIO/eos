#include <appbase/application.hpp>
#include <appbase/version.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <future>

namespace appbase {

namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;
using std::cout;

using any_type_compare_map = std::unordered_map<std::type_index, std::function<bool(const boost::any& a, const boost::any& b)>>;

class application_impl {
   public:
      application_impl():_app_options("Application Options"){
      }
      options_description     _app_options;
      options_description     _cfg_options;
      variables_map           _options;

      bfs::path               _data_dir{"data-dir"};
      bfs::path               _config_dir{"config-dir"};
      bfs::path               _logging_conf{"logging.json"};
      bfs::path               _config_file_name;

      uint64_t                _version = 0;
      std::string             _version_str = appbase_version_string;
      std::string             _full_version_str = appbase_version_string;

      std::atomic_bool        _is_quiting{false};

      any_type_compare_map    _any_compare_map;
};

application::application()
:my(new application_impl()){
   io_serv = std::make_shared<boost::asio::io_service>();

   register_config_type<std::string>();
   register_config_type<bool>();
   register_config_type<unsigned short>();
   register_config_type<unsigned>();
   register_config_type<unsigned long>();
   register_config_type<unsigned long long>();
   register_config_type<short>();
   register_config_type<int>();
   register_config_type<long>();
   register_config_type<long long>();
   register_config_type<double>();
   register_config_type<std::vector<std::string>>();
   register_config_type<boost::filesystem::path>();
}

application::~application() { }

void application::set_version(uint64_t version) {
  my->_version = version;
}

uint64_t application::version() const {
  return my->_version;
}

string application::version_string() const {
   return my->_version_str;
}

void application::set_version_string( std::string v ) {
   my->_version_str = std::move( v );
}

string application::full_version_string() const {
   return my->_full_version_str;
}

void application::set_full_version_string( std::string v ) {
   my->_full_version_str = std::move( v );
}

void application::set_default_data_dir(const bfs::path& data_dir) {
  my->_data_dir = data_dir;
}

void application::set_default_config_dir(const bfs::path& config_dir) {
  my->_config_dir = config_dir;
}

bfs::path application::get_logging_conf() const {
  return my->_logging_conf;
}

void application::wait_for_signal(std::shared_ptr<boost::asio::signal_set> ss) {
   ss->async_wait([this, ss](const boost::system::error_code& ec, int) {
      if(ec)
         return;
      quit();
      wait_for_signal(ss);
   });
}

void application::setup_signal_handling_on_ios(boost::asio::io_service& ios, bool startup) {
   std::shared_ptr<boost::asio::signal_set> ss = std::make_shared<boost::asio::signal_set>(ios, SIGINT, SIGTERM);
#ifdef SIGPIPE
   ss->add(SIGPIPE);
#endif
#ifdef SIGHUP
   if( startup ) {
      ss->add(SIGHUP);
   }
#endif
   wait_for_signal(ss);
}

void application::startup() {
   //during startup, run a second thread to catch SIGINT/SIGTERM/SIGPIPE/SIGHUP
   boost::asio::io_service startup_thread_ios;
   setup_signal_handling_on_ios(startup_thread_ios, true);
   std::thread startup_thread([&startup_thread_ios]() {
      startup_thread_ios.run();
   });
   auto clean_up_signal_thread = [&startup_thread_ios, &startup_thread]() {
      startup_thread_ios.stop();
      startup_thread.join();
   };

   try {
      for( auto plugin : initialized_plugins ) {
         if( is_quiting() ) break;
         plugin->startup();
      }

   } catch( ... ) {
      clean_up_signal_thread();
      shutdown();
      throw;
   }

   //after startup, shut down the signal handling thread and catch the signals back on main io_service
   clean_up_signal_thread();
   setup_signal_handling_on_ios(get_io_service(), false);

#ifdef SIGHUP
   std::shared_ptr<boost::asio::signal_set> sighup_set(new boost::asio::signal_set(get_io_service(), SIGHUP));
   start_sighup_handler( sighup_set );
#endif
}

void application::start_sighup_handler( std::shared_ptr<boost::asio::signal_set> sighup_set ) {
#ifdef SIGHUP
   sighup_set->async_wait([sighup_set, this](const boost::system::error_code& err, int /*num*/) {
      if( err ) return;
      app().post(priority::medium, [sighup_set, this]() {
         sighup_callback();
         for( auto plugin : initialized_plugins ) {
            if( is_quiting() ) return;
            plugin->handle_sighup();
         }
      });
      start_sighup_handler( sighup_set );
   });
#endif
}

application& application::instance() {
   static application _app;
   return _app;
}
application& app() { return application::instance(); }

void application::register_config_type_comparison(std::type_index i, config_comparison_f comp) {
   my->_any_compare_map.emplace(i, comp);
}

void application::set_program_options()
{
   for(auto& plug : plugins) {
      boost::program_options::options_description plugin_cli_opts("Command Line Options for " + plug.second->name());
      boost::program_options::options_description plugin_cfg_opts("Config Options for " + plug.second->name());
      plug.second->set_program_options(plugin_cli_opts, plugin_cfg_opts);
      if(plugin_cfg_opts.options().size()) {
         my->_app_options.add(plugin_cfg_opts);
         my->_cfg_options.add(plugin_cfg_opts);
      }
      if(plugin_cli_opts.options().size())
         my->_app_options.add(plugin_cli_opts);
   }

   options_description app_cfg_opts( "Application Config Options" );
   options_description app_cli_opts( "Application Command Line Options" );
   app_cfg_opts.add_options()
         ("plugin", bpo::value< vector<string> >()->composing(), "Plugin(s) to enable, may be specified multiple times");

   app_cli_opts.add_options()
         ("help,h", "Print this help message and exit.")
         ("version,v", "Print version information.")
         ("full-version", "Print full version information.")
         ("print-default-config", "Print default configuration template")
         ("data-dir,d", bpo::value<std::string>(), "Directory containing program runtime data")
         ("config-dir", bpo::value<std::string>(), "Directory containing configuration files such as config.ini")
         ("config,c", bpo::value<std::string>()->default_value( "config.ini" ), "Configuration file name relative to config-dir")
         ("logconf,l", bpo::value<std::string>()->default_value( "logging.json" ), "Logging configuration file name/path for library users");

   my->_cfg_options.add(app_cfg_opts);
   my->_app_options.add(app_cfg_opts);
   my->_app_options.add(app_cli_opts);
}

bool application::initialize_impl(int argc, char** argv, vector<abstract_plugin*> autostart_plugins) {
   set_program_options();

   bpo::variables_map& options = my->_options;
   try {
      bpo::parsed_options parsed = bpo::command_line_parser(argc, argv).options(my->_app_options).run();
      bpo::store(parsed, options);
      vector<string> positionals = bpo::collect_unrecognized(parsed.options, bpo::include_positional);
      if(!positionals.empty())
         BOOST_THROW_EXCEPTION(std::runtime_error("Unknown option '" + positionals[0] + "' passed as command line argument"));
   } catch( const boost::program_options::unknown_option& e ) {
      BOOST_THROW_EXCEPTION(std::runtime_error("Unknown option '" + e.get_option_name() + "' passed as command line argument"));
   }

   if( options.count( "help" ) ) {
      cout << my->_app_options << std::endl;
      return false;
   }

   if( options.count( "version" ) ) {
      cout << version_string() << std::endl;
      return false;
   }

   if( options.count( "full-version" ) ) {
      cout << full_version_string() << std::endl;
      return false;
   }

   if( options.count( "print-default-config" ) ) {
      print_default_config(cout);
      return false;
   }

   if( options.count( "data-dir" ) ) {
      // Workaround for 10+ year old Boost defect
      // See https://svn.boost.org/trac10/ticket/8535
      // Should be .as<bfs::path>() but paths with escaped spaces break bpo e.g.
      // std::exception::what: the argument ('/path/with/white\ space') for option '--data-dir' is invalid
      auto workaround = options["data-dir"].as<std::string>();
      bfs::path data_dir = workaround;
      if( data_dir.is_relative() )
         data_dir = bfs::current_path() / data_dir;
      my->_data_dir = data_dir;
   }

   if( options.count( "config-dir" ) ) {
      auto workaround = options["config-dir"].as<std::string>();
      bfs::path config_dir = workaround;
      if( config_dir.is_relative() )
         config_dir = bfs::current_path() / config_dir;
      my->_config_dir = config_dir;
   }

   auto workaround = options["logconf"].as<std::string>();
   bfs::path logconf = workaround;
   if( logconf.is_relative() )
      logconf = my->_config_dir / logconf;
   my->_logging_conf = logconf;

   workaround = options["config"].as<std::string>();
   my->_config_file_name = workaround;
   if( my->_config_file_name.is_relative() )
      my->_config_file_name = my->_config_dir / my->_config_file_name;

   if(!bfs::exists(my->_config_file_name)) {
      if(my->_config_file_name.compare(my->_config_dir / "config.ini") != 0)
      {
         cout << "Config file " << my->_config_file_name << " missing." << std::endl;
         return false;
      }
      write_default_config(my->_config_file_name);
   }

   std::vector< bpo::basic_option<char> > opts_from_config;
   try {
      bpo::parsed_options parsed_opts_from_config = bpo::parse_config_file<char>(my->_config_file_name.make_preferred().string().c_str(), my->_cfg_options, false);
      bpo::store(parsed_opts_from_config, options);
      opts_from_config = parsed_opts_from_config.options;
   } catch( const boost::program_options::unknown_option& e ) {
      BOOST_THROW_EXCEPTION(std::runtime_error("Unknown option '" + e.get_option_name() + "' inside the config file " +  full_config_file_path().string()));
   }

   std::vector<string> set_but_default_list;

   for(const boost::shared_ptr<bpo::option_description>& od_ptr : my->_cfg_options.options()) {
      boost::any default_val, config_val;
      if(!od_ptr->semantic()->apply_default(default_val))
         continue;

      if(my->_any_compare_map.find(default_val.type()) == my->_any_compare_map.end()) {
         std::cerr << "APPBASE: Developer -- the type " << default_val.type().name() << " is not registered with appbase," << std::endl;
         std::cerr << "         add a register_config_type<>() in your plugin's ctor" << std::endl;
         return false;
      }

      for(const bpo::basic_option<char>& opt : opts_from_config) {
         if(opt.string_key != od_ptr->long_name())
            continue;

         od_ptr->semantic()->parse(config_val, opt.value, true);
         if(my->_any_compare_map.at(default_val.type())(default_val, config_val))
            set_but_default_list.push_back(opt.string_key);
         break;
      }
   }
   if(set_but_default_list.size()) {
      std::cerr << "APPBASE: Warning: The following configuration items in the config.ini file are redundantly set to" << std::endl;
      std::cerr << "         their default value:" << std::endl;
      std::cerr << "             ";
      size_t chars_on_line = 0;
      for(auto it = set_but_default_list.cbegin(); it != set_but_default_list.end(); ++it) {
         std::cerr << *it;
         if(it + 1 != set_but_default_list.end())
            std::cerr << ", ";
         if((chars_on_line += it->size()) > 65) {
            std::cerr << std::endl << "             ";
            chars_on_line = 0;
         }
      }
      std::cerr << std::endl;
      std::cerr << "         Explicit values will override future changes to application defaults. Consider commenting out or" << std::endl;
      std::cerr << "         removing these items." << std::endl;
   }

   if(options.count("plugin") > 0)
   {
      auto plugins = options.at("plugin").as<std::vector<std::string>>();
      for(auto& arg : plugins)
      {
         vector<string> names;
         boost::split(names, arg, boost::is_any_of(" \t,"));
         for(const std::string& name : names)
            get_plugin(name).initialize(options);
      }
   }
   try {
      for (auto plugin : autostart_plugins)
         if (plugin != nullptr && plugin->get_state() == abstract_plugin::registered)
            plugin->initialize(options);

      bpo::notify(options);
   } catch (...) {
      std::cerr << "Failed to initialize\n";
      return false;
   }

   return true;
}

void application::shutdown() {
   for(auto ritr = running_plugins.rbegin();
       ritr != running_plugins.rend(); ++ritr) {
      (*ritr)->shutdown();
   }
   for(auto ritr = running_plugins.rbegin();
       ritr != running_plugins.rend(); ++ritr) {
      plugins.erase((*ritr)->name());
   }
   running_plugins.clear();
   initialized_plugins.clear();
   plugins.clear();
   quit();
}

void application::quit() {
   my->_is_quiting = true;
   io_serv->stop();
}

bool application::is_quiting() const {
   return my->_is_quiting;
}

void application::set_thread_priority_max() {
#if __has_include(<pthread.h>)
   pthread_t this_thread = pthread_self();
   struct sched_param params{};
   int policy = 0;
   int ret = pthread_getschedparam(this_thread, &policy, &params);
   if( ret != 0 ) {
      std::cerr << "ERROR: Unable to get thread priority" << std::endl;
   }

   params.sched_priority = sched_get_priority_max(policy);
   ret = pthread_setschedparam(this_thread, policy, &params);
   if( ret != 0 ) {
      std::cerr << "ERROR: Unable to set thread priority" << std::endl;
   }
#endif
}

void application::exec() {
   boost::asio::io_service::work work(*io_serv);
   (void)work;
   bool more = true;
   while( more || io_serv->run_one() ) {
      while( io_serv->poll_one() ) {}
      // execute the highest priority item
      more = pri_queue.execute_highest();
   }

   shutdown(); /// perform synchronous shutdown
   io_serv.reset();
}

void application::write_default_config(const bfs::path& cfg_file) {
   if(!bfs::exists(cfg_file.parent_path()))
      bfs::create_directories(cfg_file.parent_path());

   std::ofstream out_cfg( bfs::path(cfg_file).make_preferred().string());
   print_default_config(out_cfg);
   out_cfg.close();
}

void application::print_default_config(std::ostream& os) {
   std::map<std::string, std::string> option_to_plug;
   for(auto& plug : plugins) {
      boost::program_options::options_description plugin_cli_opts;
      boost::program_options::options_description plugin_cfg_opts;
      plug.second->set_program_options(plugin_cli_opts, plugin_cfg_opts);

      for(const boost::shared_ptr<bpo::option_description>& opt : plugin_cfg_opts.options())
         option_to_plug[opt->long_name()] = plug.second->name();
   }

   for(const boost::shared_ptr<bpo::option_description> od : my->_cfg_options.options())
   {
      if(!od->description().empty()) {
         std::string desc = od->description();
         boost::replace_all(desc, "\n", "\n# ");
         os << "# " << desc;
         std::map<std::string, std::string>::iterator it;
         if((it = option_to_plug.find(od->long_name())) != option_to_plug.end())
            os << " (" << it->second << ")";
         os << std::endl;
      }
      boost::any store;
      if(!od->semantic()->apply_default(store))
         os << "# " << od->long_name() << " = " << std::endl;
      else {
         auto example = od->format_parameter();
         if(example.empty())
            // This is a boolean switch
            os << "# " << od->long_name() << " = " << "false" << std::endl;
         else if(store.type() == typeid(bool))
            os << "# " << od->long_name() << " = " << (boost::any_cast<bool&>(store) ? "true" : "false") << std::endl;
         else {
            // The string is formatted "arg (=<interesting part>)"
            example.erase(0, 6);
            if(!example.empty()) example.erase(example.length()-1);
            os << "# " << od->long_name() << " = " << example << std::endl;
         }
      }
      os << std::endl;
   }
}

abstract_plugin* application::find_plugin(const string& name)const
{
   auto itr = plugins.find(name);
   if(itr == plugins.end()) {
      return nullptr;
   }
   return itr->second.get();
}

abstract_plugin& application::get_plugin(const string& name)const {
   auto ptr = find_plugin(name);
   if(!ptr)
      BOOST_THROW_EXCEPTION(std::runtime_error("unable to find plugin: " + name));
   return *ptr;
}

bfs::path application::data_dir() const {
   return my->_data_dir;
}

bfs::path application::config_dir() const {
   return my->_config_dir;
}

bfs::path application::full_config_file_path() const {
   return bfs::canonical(my->_config_file_name);
}

void application::set_sighup_callback(std::function<void()> callback) {
   sighup_callback = callback;
}

const bpo::variables_map& application::get_options() const{
      return my->_options;
}

} /// namespace appbase
