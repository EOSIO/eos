#define BOOST_TEST_MODULE fc_filesystem
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem/fstream.hpp>
#include <fc/filesystem.hpp>
#include <fc/exception/exception.hpp>

using namespace fc;

std::string getFileContent(const string& filename) {
   string ret;
   boost::filesystem::ifstream  ifs { filename };
   ifs >> ret;
   ifs.close();
   return ret;
}

BOOST_AUTO_TEST_SUITE(fc_filesystem)

BOOST_AUTO_TEST_CASE(dir_copy) try {
   // 1. check whether dir can be copied when target dir does not exist, 
   // but not recursively (compatible with 1.73)
   
   const string src_dir {"/tmp/fc_copy_test_src"};
   if (!fc::exists(src_dir)) {
      create_directories(src_dir);
   }

   BOOST_CHECK_EQUAL(fc::exists(src_dir), true);
   const string test_file_name = "fc_copy_test_file";
   if (!fc::exists(src_dir + "/" + test_file_name)) {
      boost::filesystem::ofstream  ofs { src_dir + "/" + test_file_name};
      ofs << "This the test of fc system copy \n";
      ofs.close();
   }
   BOOST_CHECK_EQUAL(fc::exists(src_dir + "/" + test_file_name), true);
      
   const string tgt_dir {"/tmp/fc_copy_test_tgt"};
   if (fc::exists(tgt_dir)) {
      remove_all(tgt_dir);
   }   
   BOOST_CHECK_EQUAL(fc::exists(tgt_dir), false);
  
   fc::copy(src_dir, tgt_dir);
   BOOST_CHECK_EQUAL(fc::exists(tgt_dir), true);
   // check not copied recursively
   BOOST_CHECK_EQUAL(fc::exists(tgt_dir + "/" + test_file_name), false);
   
   // 2. check whether exception be thrown (no overwritten) when target dir does exist, 
   BOOST_CHECK_EQUAL(fc::exists(tgt_dir), true);
   BOOST_CHECK_EXCEPTION(fc::copy(src_dir, tgt_dir), fc::exception,  [](const fc::exception& e) {
      return e.code() == fc::unspecified_exception_code;
   });

} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_CASE(file_copy) try {
   // 1. check whether file can be copied when target file does not exist, 
   const string src_dir {"/tmp/fc_copy_test_src"};
   if (!fc::exists(src_dir)) {
      create_directories(src_dir);
   }

   BOOST_CHECK_EQUAL(fc::exists(src_dir), true);
   const string test_file_name = "fc_copy_test_file";
   if (!fc::exists(src_dir + "/" + test_file_name)) {
      boost::filesystem::ofstream  ofs { src_dir + "/" + test_file_name};
      ofs << "This the test of fc system copy \n";
      ofs.close();
   }
   BOOST_CHECK_EQUAL(fc::exists(src_dir + "/" + test_file_name), true);
      
   const string tgt_dir {"/tmp/fc_copy_test_tgt"};
   if (!fc::exists(tgt_dir)) {
      fc::copy(src_dir, tgt_dir);
   }   
   BOOST_CHECK_EQUAL(fc::exists(tgt_dir), true);
   BOOST_CHECK_EQUAL(fc::exists(tgt_dir + "/" + test_file_name), false);
   fc::copy(src_dir + "/" + test_file_name, tgt_dir + "/" + test_file_name);
   BOOST_CHECK_EQUAL(fc::exists(tgt_dir + "/" + test_file_name), true);
   const string src_file_content = getFileContent(src_dir + "/" + test_file_name);
   BOOST_CHECK_EQUAL(src_file_content.empty(), false);
   const string tgt_file_content = getFileContent(tgt_dir + "/" + test_file_name);
   BOOST_CHECK_EQUAL(src_file_content, tgt_file_content);

   // 2. check whether exception be thrown (no overwritten) when target file does exist, 
   BOOST_CHECK_EQUAL(fc::exists(tgt_dir + "/" + test_file_name), true);
   BOOST_CHECK_EXCEPTION(fc::copy(src_dir + "/" + test_file_name, tgt_dir + "/" + test_file_name), 
                         fc::exception,  
                         [](const fc::exception& e) {
                              return e.code() == fc::unspecified_exception_code;
                         });
   
} FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_SUITE_END()