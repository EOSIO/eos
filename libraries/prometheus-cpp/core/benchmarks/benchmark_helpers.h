#pragma once

#include <cstddef>
#include <map>
#include <string>

std::string GenerateRandomString(std::size_t length);
std::map<std::string, std::string> GenerateRandomLabels(
    std::size_t number_of_labels);
