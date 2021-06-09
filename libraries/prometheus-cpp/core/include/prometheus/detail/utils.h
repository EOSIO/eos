#pragma once

#include <cstddef>
#include <map>
#include <string>

#include "prometheus/detail/core_export.h"

namespace prometheus {

namespace detail {

/// \brief Compute the hash value of a map of labels.
///
/// \param labels The map that will be computed the hash value.
///
/// \returns The hash value of the given labels.
PROMETHEUS_CPP_CORE_EXPORT std::size_t hash_labels(
    const std::map<std::string, std::string>& labels);

}  // namespace detail

}  // namespace prometheus
