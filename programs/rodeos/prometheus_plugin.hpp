#pragma once
#include <appbase/application.hpp>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/summary.h>
#include <fc/time.hpp>

namespace b1 {

struct metric_collection {

   struct duration_recoder {
      prometheus::Summary* summary;
      fc::time_point       start_time;
      duration_recoder(prometheus::Summary* summary) : summary(summary) , start_time(fc::time_point::now()) {}
      ~duration_recoder() { 
         auto duration = (fc::time_point::now() - start_time).count();
         summary->Observe(duration * 1e-6);
      }
   };

   prometheus::Registry                     registry;
   prometheus::Family<prometheus::Summary>* summary_family;
   prometheus::Family<prometheus::Counter>* counter_family;
   std::map<std::string, prometheus::Counter*> counters;
   std::map<std::string, prometheus::Summary*> summaries;

   metric_collection(const std::map<std::string, std::string>& labels);

   void increment_http_request_total(std::string target);
   [[nodiscard]] duration_recoder record_http_request_duration(std::string target);
};

class prometheus_plugin : public appbase::plugin<prometheus_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES()
   prometheus_plugin();
   virtual ~prometheus_plugin();

   virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;
   void         plugin_initialize(const appbase::variables_map& options);
   void         plugin_startup();
   void         plugin_shutdown();

   metric_collection& metrics();

 private:
   std::shared_ptr<struct prometheus_plugin_impl> my;
};

} // namespace b1