#pragma once
#include <appbase/application.hpp>

namespace eos {

using namespace appbase;

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class template_plugin : public appbase::plugin<template_plugin> {
public:
   template_plugin();
   virtual ~template_plugin();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class template_plugin_impl> my;
};

}
