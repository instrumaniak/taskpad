#pragma once

#include "models.h"
#include <string>
#include <vector>

namespace taskpad {

struct GlobalOptions;

struct Commands {

  static Result<void> init(const std::string& tasksDir);

  static Result<void> import_(const std::string& tasksDir, bool force);

  static Result<void> new_(const std::string& tasksDir,
      const std::string& name,
      const std::vector<std::string>& depends,
      int phase,
      bool critical);

  static Result<void> status(const std::string& tasksDir);

  static Result<void> next(const std::string& tasksDir);

  static Result<void> do_(const std::string& tasksDir,
      const std::string& taskId,
      bool force);

  static Result<void> done(const std::string& tasksDir,
      const std::string& taskId);

  static Result<void> pause(const std::string& tasksDir,
      const std::string& taskId);

  static Result<void> deps(const std::string& tasksDir,
      const std::string& taskId);

  static Result<void> log(const std::string& tasksDir,
      const std::string& taskId,
      const std::string& message);

  static Result<void> edit(const std::string& tasksDir,
      const std::string& taskId,
      const std::string& status,
      const std::vector<std::string>& depends,
      const std::string& phase,
      bool criticalSet,
      bool criticalVal,
      const std::string& phases,
      const std::string& criticalPath);

  static Result<void> summary(const std::string& tasksDir);

  static Result<void> remove(const std::string& tasksDir,
      const std::string& taskId,
      bool removeAll,
      bool force);
};

} // namespace taskpad
