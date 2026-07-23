#include "utils.h"
#include "storage.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace taskpad {

std::string toKebabCase(const std::string& name) {
  std::string result;
  for (size_t i = 0; i < name.size(); ++i) {
    char c = name[i];
    if (std::isalnum(c)) {
      if (std::isupper(c)) {
        if (!result.empty() && result.back() != '-') {
          result += '-';
        }
        result += static_cast<char>(std::tolower(c));
      } else {
        result += c;
      }
    } else if (c == ' ' || c == '_' || c == '-') {
      if (!result.empty() && result.back() != '-') {
        result += '-';
      }
    }
  }
  while (!result.empty() && result.back() == '-') {
    result.pop_back();
  }
  return result;
}

int parseTaskId(const std::string& id) {
  if (id.size() < 4 || id[0] != 'T') return 0;
  return std::stoi(id.substr(1));
}

std::string formatTaskId(int num) {
  if (num < 0) num = 0;
  if (num > 999) num = 999;
  std::ostringstream oss;
  oss << "T" << std::setw(3) << std::setfill('0') << num;
  return oss.str();
}

std::string normalizePath(const std::string& path) {
  std::string result = path;
  for (auto& c : result) {
    if (c == '\\') c = '/';
  }
  while (result.size() > 1 && result.back() == '/') {
    result.pop_back();
  }
  return result;
}

std::string currentTimestamp() {
  std::time_t t = std::time(nullptr);
  std::tm tm;
  localtime_r(&t, &tm);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
  return oss.str();
}

std::vector<std::string> split(const std::string& s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  for (char c : s) {
    if (c == delimiter) {
      tokens.push_back(trim(token));
      token.clear();
    } else {
      token += c;
    }
  }
  tokens.push_back(trim(token));
  return tokens;
}

std::string trim(const std::string& s) {
  size_t start = 0;
  while (start < s.size() && std::isspace(s[start])) ++start;
  size_t end = s.size();
  while (end > start && std::isspace(s[end - 1])) --end;
  return s.substr(start, end - start);
}

std::string resolveTaskDir(const std::string& tasksDir) {
  if (!tasksDir.empty()) return tasksDir;
  Result<std::string> r = readTaskDir(".");
  if (r.hasError()) return "specs/tasks";
  return r.value;
}

} // namespace taskpad
