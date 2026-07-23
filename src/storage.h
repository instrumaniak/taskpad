#pragma once

#include "models.h"
#include <string>

namespace taskpad {

Result<std::string> readTaskDir(const std::string& projectRoot = ".");
Result<StatusFile> readStatusFile(const std::string& taskDir);
Result<void> writeStatusFile(const std::string& taskDir, const StatusFile& sf);
Result<void> createConfig(const std::string& projectRoot, const std::string& taskDir);
bool configExists(const std::string& projectRoot = ".");
std::string taskFilePath(const std::string& taskDir, const std::string& taskId, const std::string& taskName);
Result<std::string> readTaskFile(const std::string& path);
Result<void> writeTaskFile(const std::string& path, const std::string& taskId, const std::string& taskName);
Result<void> appendLog(const std::string& path, const std::string& message);

} // namespace taskpad
