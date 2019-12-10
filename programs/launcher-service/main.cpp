
#include <eosio/launcher_service_api_plugin/launcher_service_api_plugin.hpp>

using namespace appbase;
using namespace eosio;

int main(int argc, char **argv)
{
   try {
      app().register_plugin<launcher_service_api_plugin>();
      if(!app().initialize<launcher_service_plugin, launcher_service_api_plugin, http_plugin>(argc, argv)) {
         const auto& opts = app().get_options();
         if( opts.count("help") || opts.count("version") || opts.count("full-version") || opts.count("print-default-config") ) {
            return 0;
         }
         return -1;
      }
      app().startup();
      app().exec();
      return 0;
   } catch (const fc::exception& e) {
      elog("${e}", ("e",e.to_detail_string()));
   } catch (const boost::exception& e) {
      elog("${e}", ("e",boost::diagnostic_information(e)));
   } catch (const std::exception& e) {
      elog("${e}", ("e",e.what()));
   } catch (...) {
      elog("unknown exception");
   }
   return -1;
}
