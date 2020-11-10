#pragma once
#include <filesystem>
#include <fstream>
#include <vector>

struct temporary_file {
   std::string name;
   temporary_file(const std::vector<char>& content) {
      name = std::tmpnam(nullptr);
      std::filebuf fbuf;
      fbuf.open(name.c_str(), std::ios::out | std::ios::binary);
      fbuf.sputn(content.data(), content.size());
   }
   ~temporary_file() { std::filesystem::remove(name); }
};