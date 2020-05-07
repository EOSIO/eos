#include <eosio/trace_api/compressed_file.hpp>
#include <eosio/trace_api/cmd_registration.hpp>

#include <iostream>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <fc/exception/exception.hpp>

using namespace eosio::trace_api;
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

namespace {
   std::string validate_input_path(const bpo::variables_map& vmap) {
      if (vmap.count("input-path") == 0) {
         throw bpo::required_option("input-path");
      }

      auto input_path = vmap.at("input-path").as<std::string>();

      if (!bfs::exists(input_path)) {
         throw std::logic_error(input_path + " does not exist or cannot be read");
      }

      if (bfs::is_directory(input_path)) {
         throw std::logic_error(input_path + " is a directory and not a trace file");
      }

      return input_path;
   }

   std::string validate_output_path(const bpo::variables_map& vmap, const std::string& input_path) {
      static const std::string default_output_extension = ".clog";
      std::string output_path;
      if (vmap.count("output-path")) {
         output_path = vmap.at("output-path").as<std::string>();

         if (bfs::exists(output_path) && bfs::is_directory(output_path)) {
            output_path = (bfs::path(output_path) / bfs::path(input_path).filename()).replace_extension(
                  default_output_extension).generic_string();
         } else {
            auto output_dir = bfs::path(output_path).parent_path();
            if (!output_dir.empty() && !bfs::exists(output_dir)) {
               throw std::logic_error(
                     std::string("Output file parent directory: " + output_dir.generic_string() + " does not exist"));
            }
         }
      } else {
         auto input = bfs::path(input_path);
         if (input.extension() != ".log") {
            throw std::logic_error(std::string("Unrecognized extension: ") + input.extension().generic_string());
         }

         output_path = input.replace_extension(default_output_extension).generic_string();
      }

      return output_path;
   }

   void print_help_text(std::ostream& os, const bpo::options_description& opts) {
      os <<
         "Usage: trace_api_util compress <options> input-path [output-path]\n"
         "\n"
         "Compress a trace file to into the \"clog\" format.  By default the name of the\n"
         "of the compressed file will the the same as the input-path with a changing the\n"
         "extension to \"clog\"."
         "\n\n"
         "Positional Options:\n"
         "  input-path                      path to the file to compress\n"
         "  output-path                     [Optional] output file or directory path.\n"
         "\n";
      os << opts << "\n";
   }

   int do_compress(const bpo::variables_map& global_args, const std::vector<std::string>& args) {
      bpo::options_description vis_desc("Options");
      auto opts = vis_desc.add_options();
      opts("help,h", "show usage help message");
      opts("seek-point-stride,s", bpo::value<uint32_t>()->default_value(512),
           "the number of bytes between seek points in a compressed trace.  "
           "A smaller stride may degrade compression efficiency but increase read efficiency");

      if (global_args.count("help")) {
         print_help_text(std::cout, vis_desc);
         return 0;
      }

      bpo::options_description hidden_desc("hidden");
      auto hidden_opts = hidden_desc.add_options();
      hidden_opts("input-path,i", bpo::value<std::string>(), "input path");
      hidden_opts("output-path,o", bpo::value<std::string>(), "output path");

      bpo::positional_options_description pos_desc;
      pos_desc.add("input-path", 1);
      pos_desc.add("output-path", 1);

      bpo::options_description cmdline_options;
      cmdline_options.add(vis_desc).add(hidden_desc);

      bpo::variables_map vmap;
      try {
         bpo::store(bpo::command_line_parser(args).options(cmdline_options).positional(pos_desc).run(), vmap);
         bpo::notify(vmap);
         if (global_args.count("help") == 0) {
            auto input_path = validate_input_path(vmap);
            auto output_path = validate_output_path(vmap, input_path);
            auto seek_point_stride = vmap.at("seek-point-stride").as<uint32_t>();

            if (!compressed_file::process(input_path, output_path, seek_point_stride)) {
               throw std::runtime_error("Unexpected compression failure");
            }
         } else {
            print_help_text(std::cout, vis_desc);
         }

         return 0;
      } catch (const bpo::error& e) {
         std::cerr << "Error: " << e.what() << "\n\n";
         print_help_text(std::cerr, vis_desc);
      } catch (const std::exception& e) {
         std::cerr << "Error: " << e.what() << "\n";
      } catch (const fc::exception& e) {
         std::cerr << "Error: " << e.to_detail_string() << "\n";
      } catch (...) {
         std::cerr << "An Unknown Error Occurred\n";
      }

      return 1;
   }

   auto _reg = command_registration("compress", "Compress a trace file to into the \"clog\" format", do_compress);
}

