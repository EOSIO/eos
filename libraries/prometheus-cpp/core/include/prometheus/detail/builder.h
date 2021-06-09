#pragma once

#include <map>
#include <string>

// IWYU pragma: private
// IWYU pragma: no_include "prometheus/family.h"

namespace prometheus {

template <typename T>
class Family;    // IWYU pragma: keep
class Registry;  // IWYU pragma: keep

namespace detail {

template <typename T>
class Builder {
 public:
  Builder& Labels(const std::map<std::string, std::string>& labels);
  Builder& Name(const std::string&);
  Builder& Help(const std::string&);
  Family<T>& Register(Registry&);

 private:
  std::map<std::string, std::string> labels_;
  std::string name_;
  std::string help_;
};

}  // namespace detail
}  // namespace prometheus
