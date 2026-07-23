#pragma once

#include "models.h"
#include <string>
#include <vector>
#include <map>

namespace taskpad {

bool isValidTaskId(const std::string& id);
bool isValidStatus(const std::string& status);
Result<void> validateCircularDependencies(
    const std::string& taskId,
    const std::vector<std::string>& depends,
    const std::map<std::string, Task>& tasks);
Result<void> validateTaskExists(
    const std::string& id,
    const std::map<std::string, Task>& tasks);
Result<void> validateDependsExist(
    const std::vector<std::string>& depends,
    const std::map<std::string, Task>& tasks);

} // namespace taskpad
