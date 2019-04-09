#pragma once

#define READ_HANDLER(api_name, api_namespace, object, method, http_response_code) \
{std::string("/v1/" #api_name "/" #method), \
   [&](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = object.method(fc::json::from_string(body).as<method ## _params>()); \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #method, body, cb); \
          } \
       }}

#define CREATE_READ_HANDLER(object, method, http_response_code) READ_HANDLER(chain, chain_api_plugin_impl, object, method, http_response_code)

#define WRITE_HANDLER(api_name, object, method, call_result, http_response_code) \
{std::string("/v1/" #api_name "/" #method), \
   [&](string, string body, url_response_callback cb) mutable { \
      if (body.empty()) body = "{}"; \
      object.validate(); \
      object.method(fc::json::from_string(body).as<method ## _params>(),\
         [cb, body](const fc::static_variant<fc::exception_ptr, call_result>& result){\
            if (result.contains<fc::exception_ptr>()) {\
               try {\
                  result.get<fc::exception_ptr>()->dynamic_rethrow_exception();\
               } catch (...) {\
                  http_plugin::handle_exception(#api_name, #method, body, cb);\
               }\
            } else {\
               cb(http_response_code, result.visit(async_result_visitor()));\
            }\
         });\
   }\
}

#define CREATE_WRIGHT_HANDLER(object, method, call_result, http_response_code) WRITE_HANDLER(chain, object, method, call_result, http_response_code)
