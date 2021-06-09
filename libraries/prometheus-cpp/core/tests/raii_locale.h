#pragma once

#include <locale>

class RAIILocale {
 public:
  RAIILocale(const char* name) : savedLocale_(std::locale::classic()) {
    std::locale::global(std::locale(name));
  }

  ~RAIILocale() { std::locale::global(savedLocale_); }

 private:
  const std::locale savedLocale_;
};
