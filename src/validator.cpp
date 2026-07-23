#include "validator.h"
#include <algorithm>
#include <set>
#include <vector>

namespace taskpad {

bool isValidTaskId(const std::string& id) {
  if (id.size() != 4) return false;
  if (id[0] != 'T') return false;
  for (size_t i = 1; i < 4; ++i) {
    if (!std::isdigit(id[i])) return false;
  }
  int num = std::stoi(id.substr(1));
  return num >= 1 && num <= 999;
}

bool isValidStatus(const std::string& status) {
  return status == "pending" || status == "in_progress" || status == "done";
}

Result<void> validateCircularDependencies(
    const std::string& taskId,
    const std::vector<std::string>& depends,
    const std::map<std::string, Task>& tasks) {

for (const std::string& dep : depends) {
    if (dep == taskId) {
      return Result<void>::failure(
          "Circular dependency detected: " + taskId + " depends on itself");
    }

    auto it = tasks.find(dep);
    if (it == tasks.end()) continue;

    std::set<std::string> visited;
    std::vector<std::string> stack = {dep};

    while (!stack.empty()) {
      std::string current = stack.back();
      stack.pop_back();

      if (current == taskId) {
        return Result<void>::failure(
            "Circular dependency detected: " + taskId + " → ... → " + dep);
      }

      if (!visited.insert(current).second) continue;

      auto taskIt = tasks.find(current);
      if (taskIt != tasks.end()) {
        for (const auto& d : taskIt->second.depends) {
          if (!visited.count(d)) {
            stack.push_back(d);
          }
        }
      }
    }
  }

  return Result<void>::success();
}

Result<void> validateTaskExists(
    const std::string& id,
    const std::map<std::string, Task>& tasks) {
  if (tasks.find(id) == tasks.end()) {
    return Result<void>::failure("Task " + id + " not found");
  }
  return Result<void>::success();
}

Result<void> validateDependsExist(
    const std::vector<std::string>& depends,
    const std::map<std::string, Task>& tasks) {
for (const std::string& dep : depends) {
    if (tasks.find(dep) == tasks.end()) {
      return Result<void>::failure("Dependency " + dep + " not found");
    }
  }
  return Result<void>::success();
}

} // namespace taskpad
