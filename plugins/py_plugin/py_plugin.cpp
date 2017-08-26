#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

#include <eos/chain/config.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/utilities/key_conversion.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/algorithm/string/split.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <fc/io/fstream.hpp>


#include <eos/py_plugin/py_plugin.hpp>
#include <fc/log/logger_config.hpp>
#include <python.h>





using namespace std;
using namespace eos;
using namespace eos::chain;
using namespace eos::utilities;

//   chain_apis::read_write get_read_write_api();
extern "C" int get_info_(char *result,int length) {
    auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
    auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();
    chain_apis::read_only::get_info_params params;
    auto info = ro_api.get_info(params);
    
    auto json_str = fc::json::to_string(info);
    printf("json_str.size():%d\n",json_str.size());
    if (json_str.size()>length){
      return -1;
    }
    strcpy(result,json_str.data());
    ilog(result);
    return 0;
}


namespace eos {

class py_plugin_impl {
   public:
};

py_plugin::py_plugin():my(new py_plugin_impl()){}
py_plugin::~py_plugin(){}

void py_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void py_plugin::plugin_initialize(const variables_map& options) {
   if(options.count("option-name")) {
      // Handle the option
   }
}

extern "C" void c_printf(const char *s);
extern "C" void PyInit_hello();

void py_plugin::plugin_startup() {
    Py_Initialize();
    PyRun_SimpleString("import readline");
    PyInit_hello();
    PyRun_SimpleString("import hello");
    ilog("++++++++++++++py_plugin::plugin_startup");
    c_printf("hello,world");
//    get_info();
    PyRun_InteractiveLoop(stdin, "<stdin>");
    Py_Finalize();
}

void py_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

}
