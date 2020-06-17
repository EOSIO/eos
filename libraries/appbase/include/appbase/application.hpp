#pragma once
#include <appbase/plugin.hpp>
#include <appbase/channel.hpp>
#include <appbase/method.hpp>
#include <appbase/execution_priority_queue.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/core/demangle.hpp>
#include <typeindex>

namespace appbase {
   namespace bpo = boost::program_options;
   namespace bfs = boost::filesystem;

   using config_comparison_f = std::function<bool(const boost::any& a, const boost::any& b)>;

   class application
   {
      public:
         ~application();


         /** @brief Set version
          *
          * @param version Version output with -v/--version
          */
         void set_version(uint64_t version);
         /** @brief Get version
          *
          * @return Version output with -v/--version
          */
         uint64_t version() const;
         /** @brief Get version string; generated from git describe if available
          *
          * @return A string worthy of output with -v/--version, or "Unknown" if git not available
          */
         string version_string() const;
         /** @brief User provided version string for version_string() which overrides git describe value.
          */
         void set_version_string(std::string v);
         /** @brief Get full version string; same as version_string() unless set differently.
          *
          * @return A string worthy of output with -v/--version, or "Unknown" if git not available
          */
         string full_version_string() const;
         /** @brief User provided full version string for full_version_string()
          */
         void set_full_version_string(std::string v);
         /** @brief Set default data directory
          *
          * @param data_dir Default data directory to use if not specified
          *                 on the command line.
          */
         void set_default_data_dir(const bfs::path& data_dir = "data-dir");
         /** @brief Get data directory
          *
          * @return Data directory, possibly from command line
          */
         bfs::path data_dir() const;
         /** @brief Set default config directory
          *
          * @param config_dir Default configuration directory to use if not
          *                   specified on the command line.
          */
         void set_default_config_dir(const bfs::path& config_dir = "etc");
         /** @brief Get config directory
          *
          * @return Config directory, possibly from command line
          */
         bfs::path config_dir() const;
         /** @brief Get logging configuration path.
          *
          * @return Logging configuration location from command line
          */
         bfs::path get_logging_conf() const;
         /** @brief Get full config.ini path
          *
          * @return Config directory & config file name, possibly from command line. Only
          *         valid after initialize() has been called.
          */
         bfs::path full_config_file_path() const;
         /** @brief Set function pointer invoked on receipt of SIGHUP
          *
          * The provided function will be invoked on receipt of SIGHUP followed
          * by invoking handle_sighup() on all initialized plugins. Caller
          * is responsible for preserving an object if necessary.
          *
          * @param callback Function pointer that will be invoked when the process
          *                 receives the HUP (1) signal.
          */
          void set_sighup_callback(std::function<void()> callback);
         /**
          * @brief Looks for the --plugin commandline / config option and calls initialize on those plugins
          *
          * @tparam Plugin List of plugins to initalize even if not mentioned by configuration. For plugins started by
          * configuration settings or dependency resolution, this template has no effect.
          * @return true if the application and plugins were initialized, false or exception on error
          */
         template<typename... Plugin>
         bool                 initialize(int argc, char** argv) {
            return initialize_impl(argc, argv, {find_plugin<Plugin>()...});
         }

         void                  startup();
         void                  shutdown();

         /**
          *  Wait until quit(), SIGINT or SIGTERM and then shutdown.
          *  Should only be executed from one thread.
          */
         void                 exec();
         void                 quit();

         /**
          * If in long running process this flag can be checked to see if processing should be stoppped.
          * @return true if quit() has been called.
          */
         bool                 is_quiting()const;

         /**
          * Register a configuration type with appbase. most "plain" types are already registered in
          * application.cpp. Failure to register a type will cause initialization to fail.
          */
         template <typename T> void register_config_type() {
            register_config_type_comparison(typeid(T), [](const auto& a, const auto& b) {
               return boost::any_cast<const T&>(a) == boost::any_cast<const T&>(b);
            });
         }
         void register_config_type_comparison(std::type_index, config_comparison_f comp);

         static application&  instance();

         abstract_plugin* find_plugin(const string& name)const;
         abstract_plugin& get_plugin(const string& name)const;

         template<typename Plugin>
         auto& register_plugin() {
            auto existing = find_plugin<Plugin>();
            if(existing)
               return *existing;

            auto plug = new Plugin();
            plugins[plug->name()].reset(plug);
            plug->register_dependencies();
            return *plug;
         }

         template<typename Plugin>
         Plugin* find_plugin()const {
            string name = boost::core::demangle(typeid(Plugin).name());
            return dynamic_cast<Plugin*>(find_plugin(name));
         }

         template<typename Plugin>
         Plugin& get_plugin()const {
            auto ptr = find_plugin<Plugin>();
            return *ptr;
         }

         template<typename Plugin>
         bool is_plugin_initialized()const {
            string name = boost::core::demangle(typeid(Plugin).name());
            for ( const auto& plugin: initialized_plugins ) {
               if ( plugin->name() == name ) {
                  return true;
               }
            }

            return false;
         }

         /**
          * Fetch a reference to the method declared by the passed in type.  This will construct the method
          * on first access.  This allows loose and deferred binding between plugins
          *
          * @tparam MethodDecl - @ref appbase::method_decl
          * @return reference to the method described by the declaration
          */
         template<typename MethodDecl>
         auto get_method() -> std::enable_if_t<is_method_decl<MethodDecl>::value, typename MethodDecl::method_type&>
         {
            using method_type = typename MethodDecl::method_type;
            auto key = std::type_index(typeid(MethodDecl));
            auto itr = methods.find(key);
            if(itr != methods.end()) {
               return *method_type::get_method(itr->second);
            } else {
               methods.emplace(std::make_pair(key, method_type::make_unique()));
               return  *method_type::get_method(methods.at(key));
            }
         }

         /**
          * Fetch a reference to the channel declared by the passed in type.  This will construct the channel
          * on first access.  This allows loose and deferred binding between plugins
          *
          * @tparam ChannelDecl - @ref appbase::channel_decl
          * @return reference to the channel described by the declaration
          */
         template<typename ChannelDecl>
         auto get_channel() -> std::enable_if_t<is_channel_decl<ChannelDecl>::value, typename ChannelDecl::channel_type&>
         {
            using channel_type = typename ChannelDecl::channel_type;
            auto key = std::type_index(typeid(ChannelDecl));
            auto itr = channels.find(key);
            if(itr != channels.end()) {
               return *channel_type::get_channel(itr->second);
            } else {
               channels.emplace(std::make_pair(key, channel_type::make_unique()));
               return  *channel_type::get_channel(channels.at(key));
            }
         }

         /**
          * Do not run io_service in any other threads, as application assumes single-threaded execution in exec().
          * @return io_serivice of application
          */
         boost::asio::io_service& get_io_service() { return *io_serv; }

         /**
          * Post func to run on io_service with given priority.
          *
          * @param priority can be appbase::priority::* constants or any int, larger ints run first
          * @param func function to run on io_service
          * @return result of boost::asio::post
          */
         template <typename Func>
         auto post( int priority, Func&& func ) {
            return boost::asio::post(*io_serv, pri_queue.wrap(priority, std::forward<Func>(func)));
         }

         /**
          * Provide access to execution priority queue so it can be used to wrap functions for
          * prioritized execution.
          *
          * Example:
          *   boost::asio::steady_timer timer( app().get_io_service() );
          *   timer.async_wait( app().get_priority_queue().wrap(priority::low, [](){ do_something(); }) );
          */
         auto& get_priority_queue() {
            return pri_queue;
         }

         const bpo::variables_map& get_options() const;

         /**
          * Set the current thread schedule priority to maximum.
          * Works for pthreads.
          */
         void set_thread_priority_max();

      protected:
         template<typename Impl>
         friend class plugin;

         bool initialize_impl(int argc, char** argv, vector<abstract_plugin*> autostart_plugins);

         /** these notifications get called from the plugin when their state changes so that
          * the application can call shutdown in the reverse order.
          */
         ///@{
         void plugin_initialized(abstract_plugin& plug){ initialized_plugins.push_back(&plug); }
         void plugin_started(abstract_plugin& plug){ running_plugins.push_back(&plug); }
         ///@}

      private:
         application(); ///< private because application is a singleton that should be accessed via instance()
         map<string, std::unique_ptr<abstract_plugin>> plugins; ///< all registered plugins
         vector<abstract_plugin*>                  initialized_plugins; ///< stored in the order they were started running
         vector<abstract_plugin*>                  running_plugins; ///< stored in the order they were started running

         std::function<void()>                     sighup_callback;
         map<std::type_index, erased_method_ptr>   methods;
         map<std::type_index, erased_channel_ptr>  channels;

         std::shared_ptr<boost::asio::io_service>  io_serv;
         execution_priority_queue                  pri_queue;

         void start_sighup_handler( std::shared_ptr<boost::asio::signal_set> sighup_set );
         void set_program_options();
         void write_default_config(const bfs::path& cfg_file);
         void print_default_config(std::ostream& os);

         void wait_for_signal(std::shared_ptr<boost::asio::signal_set> ss);
         void setup_signal_handling_on_ios(boost::asio::io_service& ios, bool startup);

         std::unique_ptr<class application_impl> my;

   };

   application& app();


   template<typename Impl>
   class plugin : public abstract_plugin {
      public:
         plugin():_name(boost::core::demangle(typeid(Impl).name())){}
         virtual ~plugin(){}

         virtual state get_state()const override         { return _state; }
         virtual const std::string& name()const override { return _name; }

         virtual void register_dependencies() {
            static_cast<Impl*>(this)->plugin_requires([&](auto& plug){});
         }

         virtual void initialize(const variables_map& options) override {
            if(_state == registered) {
               _state = initialized;
               static_cast<Impl*>(this)->plugin_requires([&](auto& plug){ plug.initialize(options); });
               static_cast<Impl*>(this)->plugin_initialize(options);
               //ilog( "initializing plugin ${name}", ("name",name()) );
               app().plugin_initialized(*this);
            }
            assert(_state == initialized); /// if initial state was not registered, final state cannot be initialized
         }

         virtual void handle_sighup() override {
         }

         virtual void startup() override {
            if(_state == initialized) {
               _state = started;
               static_cast<Impl*>(this)->plugin_requires([&](auto& plug){ plug.startup(); });
               static_cast<Impl*>(this)->plugin_startup();
               app().plugin_started(*this);
            }
            assert(_state == started); // if initial state was not initialized, final state cannot be started
         }

         virtual void shutdown() override {
            if(_state == started) {
               _state = stopped;
               //ilog( "shutting down plugin ${name}", ("name",name()) );
               static_cast<Impl*>(this)->plugin_shutdown();
            }
         }

      protected:
         plugin(const string& name) : _name(name){}

      private:
         state _state = abstract_plugin::registered;
         std::string _name;
   };

   template<typename Data, typename DispatchPolicy>
   void channel<Data,DispatchPolicy>::publish(int priority, const Data& data) {
      if (has_subscribers()) {
         // this will copy data into the lambda
         app().post( priority, [this, data]() {
            _signal(data);
         });
      }
   }

}
