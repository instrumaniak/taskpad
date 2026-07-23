#pragma once

#include <string>
#include <vector>

namespace taskpad {

std::string toKebabCase(const std::string& name);
int parseTaskId(const std::string& id);
std::string formatTaskId(int num);
std::string normalizePath(const std::string& path);
std::string currentTimestamp();
std::vector<std::string> split(const std::string& s, char delimiter);
std::string trim(const std::string& s);

} // namespace taskpad
