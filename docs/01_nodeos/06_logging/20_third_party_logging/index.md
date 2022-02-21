---
content_title: Third-Party Logging And Tracing Integration
link_text: Third-Party Logging And Tracing Integration
---

## Overview

To stay informed about the overall and detailed performance of your EOSIO-based blockchain node(s), you can make use of the telemetry tools available. EOSIO offers integration with two such telemetry tools:

* [Deep-mind logger](10_deep_mind_logger.md)
* [Zipkin tracer](20_zipkin_tracer.md)

## Performance Considerations

Many wise people already said it, everything comes with a price. The telemetry tools, when activated, will have a certain impact on your `nodeos` performance which depends on various factors, but nevertheless the performance will be impacted; therefore, it is recommended you use them wisely in those situations when you really need the extra detailed information they provide, and then you turn them off.
