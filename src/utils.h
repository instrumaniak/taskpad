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
std::string resolveTaskDir(const std::string& tasksDir);

// Extract functions for parsing T*.md metadata sections
int extractPhase(const std::string& content);
bool extractCritical(const std::string& content);
std::vector<std::string> extractSectionListItems(
    const std::string& content, const std::string& sectionHeader);

} // namespace taskpad
