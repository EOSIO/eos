#include <eos/py_plugin/py_plugin.hpp>
#include <fc/log/logger_config.hpp>
#include <python.h>

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
    PyInit_hello();
    ilog("++++++++++++++py_plugin::plugin_startup");
    c_printf("hello,world");
    PyRun_InteractiveLoop(stdin, "<stdin>");
    Py_Finalize();
}

void py_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

}
