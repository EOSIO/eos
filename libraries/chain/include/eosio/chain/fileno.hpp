#pragma once

#include <iosfwd>

template <typename charT, typename traits>
int fileno_hack(const std::basic_ios<charT, traits>& stream);
