/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <stdlib.h>
#include <time.h>
#include <vector>


namespace eosio {

   using namespace std;

   /**
    * all measurements are normalized to integer samples.
    * metric.last is the last observed measurement for the sample.
    * metric.min is the smallest sampled value
    * metric.max is the largest sampled value
    * metric.avg is the "average" value. Since historical measurements are not
    *           retained, the average is computed by multiplying the previous avg
    *           value times the number of samples taken so far, adding the new
    *           sample and dividing by the number of samples + 1.
    */
   struct metric {
      uint32_t count;
      uint32_t last;
      uint32_t min;
      uint32_t max;
      uint32_t avg;

      metric() : count(0), last(0), min((uint32_t)-1),max(0),avg(0) {}
      void add_sample(uint32_t value ) {
         last = value;
         if( value < min ) min = value;
         if( value > max ) max = value;
         uint64_t num = avg * count + value;
         avg = (uint32_t)(num / (++count));
      }
   };

   /**
    * In order to have maximum flexibility and minimize the size of in-flight sample
    * updates, the link metrics are stored in a map with this enumeration providing
    * key values.
    */

   enum metric_kind
      {
       queue_depth = 0x01,          // how many messages are waiting to be sent
       queue_max_depth = 0x02,      // what was the high water mark for this period
       queue_latency = 0x04,        // how long does a message sit in the queue
       net_latency = 0x08,          // how long does a network traversal take
       bytes_sent = 0x10,           // how many bytes have been sent on a link since its connection
       messages_sent = 0x20,        // how many messages have been sent on a link since connection
       bytes_per_second = 0x40,
       messages_per_second = 0x80,
       fork_instances = 0x100,      // how many forks have been detected from a peer
       fork_depth = 0x200,          // how many blocks we on the most recent / current fork
       fork_max_depth = 0x400       // how many blocks on the longest fork since connection
      };

   using topology_sample = std::pair<metric_kind, uint32_t>;

   /**
    * Link metrics is the accumulator of sample measurements. The measurements are all
    * kept in the context of the sender.
    * @attributes
    * last_sample  is the time when the metrics
    * measurements is the container for all the sampled info.
    * link_metrics.total_bytes  is the quantity of data sent measured in bytes and
    *
    */

   using named_metric = pair<metric_kind, metric>;

   struct link_metrics {
      uint32_t                       last_sample;
      uint32_t                       first_sample;
      fc::flat_map<metric_kind,metric> measurements;
      // vector<named_metric>         measurements;
      uint64_t                     total_bytes;
      uint64_t                     total_messages;


      void sample(const vector<topology_sample>& samples) {
         last_sample = time(0);
         if (first_sample == 0) {
            first_sample = last_sample;
         }
         time_t duration = last_sample - first_sample;
         for (const auto& s : samples) {
            uint32_t value = s.second;
            metric_kind kind = s.first;
            if (s.first == messages_sent) {
               total_messages += s.second;
               kind = messages_per_second;
               value = duration == 0 ? 0 : total_messages / duration;
            }
            else if (s.first == bytes_sent) {
               total_bytes += s.second;
               kind = bytes_per_second;
               value = duration == 0 ? 0: total_bytes / duration;
            }
            measurements[kind].add_sample(value);
         }
      }
   };

} // namespace eosio
FC_REFLECT_ENUM( eosio::metric_kind, (queue_depth)(queue_max_depth)(queue_latency)(net_latency)(bytes_sent)(messages_sent)(bytes_per_second)(messages_per_second)(fork_instances)(fork_depth)(fork_max_depth))
FC_REFLECT( eosio::metric, (count)(last)(min)(max)(avg))
FC_REFLECT( eosio::link_metrics, (last_sample)(first_sample)(measurements)(total_bytes)(total_messages))
