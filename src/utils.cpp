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

int extractPhase(const std::string& content) {
  size_t pos = content.find("## Phase:");
  if (pos == std::string::npos) return 0;

  size_t colonPos = content.find(':', pos);
  if (colonPos == std::string::npos) return 0;

  size_t valueStart = colonPos + 1;
  while (valueStart < content.size() && std::isspace(content[valueStart])) {
    ++valueStart;
  }

  size_t valueEnd = valueStart;
  while (valueEnd < content.size() && std::isdigit(content[valueEnd])) {
    ++valueEnd;
  }

  if (valueEnd == valueStart) return 0;

  int phase = std::stoi(content.substr(valueStart, valueEnd - valueStart));
  return (phase < 0) ? 0 : phase;
}

bool extractCritical(const std::string& content) {
  size_t pos = content.find("## Critical:");
  if (pos == std::string::npos) return false;

  size_t colonPos = content.find(':', pos);
  if (colonPos == std::string::npos) return false;

  size_t valueStart = colonPos + 1;
  while (valueStart < content.size() && std::isspace(content[valueStart])) {
    ++valueStart;
  }

  size_t valueEnd = valueStart;
  while (valueEnd < content.size() && !std::isspace(content[valueEnd]) &&
         content[valueEnd] != '\n' && content[valueEnd] != '\r') {
    ++valueEnd;
  }

  std::string value = content.substr(valueStart, valueEnd - valueStart);
  std::string lower;
  lower.reserve(value.size());
  for (char c : value) {
    lower += static_cast<char>(std::tolower(c));
  }
  return (lower == "true");
}

std::vector<std::string> extractSectionListItems(
    const std::string& content, const std::string& sectionHeader) {
  std::vector<std::string> result;

  size_t sectionPos = content.find(sectionHeader);
  if (sectionPos == std::string::npos) return result;

  size_t contentStart = content.find('\n', sectionPos);
  if (contentStart == std::string::npos) return result;
  ++contentStart;

  size_t sectionEnd = content.find("\n## ", contentStart);
  if (sectionEnd == std::string::npos) {
    sectionEnd = content.size();
  }

  std::string section = content.substr(contentStart, sectionEnd - contentStart);

  size_t lineStart = 0;
  while (lineStart < section.size()) {
    size_t lineEnd = section.find('\n', lineStart);
    if (lineEnd == std::string::npos) lineEnd = section.size();

    std::string line = section.substr(lineStart, lineEnd - lineStart);

    size_t tickStart = line.find('`');
    if (tickStart != std::string::npos) {
      size_t tickEnd = line.find('`', tickStart + 1);
      if (tickEnd != std::string::npos) {
        result.push_back(line.substr(tickStart + 1, tickEnd - tickStart - 1));
      }
    }

    lineStart = lineEnd + 1;
  }

  return result;
}

} // namespace taskpad
