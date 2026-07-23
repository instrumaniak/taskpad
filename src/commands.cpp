#include "commands.h"
#include "storage.h"
#include "utils.h"
#include "validator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <vector>

namespace taskpad {

// ---- Helpers ----

static std::string extractStatusLine(const std::string& content) {
  size_t pos = content.find("## Status:");
  if (pos == std::string::npos) return "";

  size_t eol = content.find('\n', pos);
  if (eol == std::string::npos) return "";

  std::string line = content.substr(pos + 10, eol - pos - 10);
  return trim(line);
}

static std::vector<std::string> extractDepends(const std::string& content) {
  std::vector<std::string> result;
  size_t pos = content.find("## Depends On");
  if (pos == std::string::npos) return result;

  size_t start = content.find('\n', pos);
  if (start == std::string::npos) return result;

  size_t end = content.find("\n## ", start + 1);
  if (end == std::string::npos) end = content.size();

  std::string section = content.substr(start, end - start);
  for (size_t i = 0; i + 4 <= section.size(); ++i) {
    if (section[i] == 'T' && std::isdigit(section[i + 1]) &&
        std::isdigit(section[i + 2]) && std::isdigit(section[i + 3])) {
      bool wordBoundaryBefore = (i == 0 || !std::isalnum(section[i - 1]));
      bool wordBoundaryAfter = (i + 4 >= section.size() ||
                                !std::isalnum(section[i + 4]));
      if (wordBoundaryBefore && wordBoundaryAfter) {
        result.push_back(section.substr(i, 4));
      }
    }
  }
  return result;
}

static std::string kebabToTitle(const std::string& kebab) {
  std::string result;
  bool capitalize = true;
  for (char c : kebab) {
    if (c == '-') {
      result += ' ';
      capitalize = true;
    } else {
      result += capitalize ? static_cast<char>(std::toupper(c)) : c;
      capitalize = false;
    }
  }
  return result;
}

static std::string findNextTaskId(const std::map<std::string, Task>& tasks) {
  int maxId = 0;
  for (const auto& kv : tasks) {
    int num = parseTaskId(kv.first);
    if (num > maxId) maxId = num;
  }
  return formatTaskId(maxId + 1);
}

static bool allDepsDone(const Task& task,
    const std::map<std::string, Task>& tasks) {
  for (const auto& dep : task.depends) {
    auto it = tasks.find(dep);
    if (it == tasks.end() || it->second.status != Status::Done) {
      return false;
    }
  }
  return true;
}

static std::string getFirstStep(const std::string& content) {
  size_t pos = content.find("## Implementation Steps");
  if (pos == std::string::npos) return "";

  size_t start = content.find('\n', pos);
  if (start == std::string::npos) return "";

  size_t nextSection = content.find("\n## ", start + 1);
  if (nextSection == std::string::npos) nextSection = content.size();

  std::string section = content.substr(start, nextSection - start);

  // Find first list item or numbered item
  for (size_t i = 0; i < section.size(); ++i) {
    char c = section[i];
    if (c == '-' && i + 1 < section.size() && section[i + 1] == ' ') {
      size_t lineEnd = section.find('\n', i);
      if (lineEnd == std::string::npos) lineEnd = section.size();
      return trim(section.substr(i + 2, lineEnd - i - 2));
    }
    if (std::isdigit(c) && i + 1 < section.size() && section[i + 1] == '.') {
      size_t lineEnd = section.find('\n', i);
      if (lineEnd == std::string::npos) lineEnd = section.size();
      std::string item = trim(section.substr(i + 2, lineEnd - i - 2));
      if (!item.empty()) return item;
    }
  }
  return "";
}

static std::string extractGoal(const std::string& content) {
  size_t pos = content.find("## Goal");
  if (pos == std::string::npos) return "";

  size_t start = content.find('\n', pos);
  if (start == std::string::npos) return "";

  // Skip blank lines
  while (start + 1 < content.size() && content[start + 1] == '\n') {
    ++start;
  }

  size_t end = content.find("\n## ", start + 1);
  if (end == std::string::npos) end = content.size();

  return trim(content.substr(start, end - start));
}

static std::string statusColor(Status s) {
  switch (s) {
    case Status::Done: return "[done]";
    case Status::InProgress: return "[in_progress]";
    case Status::Pending: return "[pending]";
  }
  return "[unknown]";
}

static bool fileExists(const std::string& path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static bool copyFile(const std::string& src, const std::string& dst) {
  std::ifstream in(src, std::ios::binary);
  if (!in) return false;

  // Create directories if needed
  size_t pos = dst.rfind('/');
  if (pos != std::string::npos) {
    std::string dir = dst.substr(0, pos);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
  }

  std::ofstream out(dst, std::ios::binary);
  if (!out) return false;

  out << in.rdbuf();
  return out.good();
}

static int countStatus(const std::map<std::string, Task>& tasks,
    Status s) {
  int count = 0;
  for (const auto& kv : tasks) {
    if (kv.second.status == s) ++count;
  }
  return count;
}

// ---- Commands ----

Result<void> Commands::init(const std::string& tasksDir) {
  std::string dir = tasksDir.empty() ? "specs/tasks" : tasksDir;
  Result<void> result = createConfig(".", dir);
  if (result.hasError()) return result;

  // Create task directory if it doesn't exist
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);

  std::cout << "Initialized taskpad in " << dir << "/" << std::endl;
  std::cout << "Created .taskpad config" << std::endl;
  std::cout << "Ready to add tasks with `taskpad new` or `taskpad import`"
            << std::endl;
  return Result<void>::success();
}

Result<void> Commands::import_(const std::string& tasksDir, bool force) {
  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> result = readStatusFile(dir);
  if (!result.hasError() && !force) {
    return Result<void>::failure(
        "status.yaml already exists. Use --force to overwrite");
  }

  std::cout << "Scanning " << dir << "/ for T*.md files..." << std::endl;

  std::vector<std::string> taskFiles;
  std::error_code ec;
  std::filesystem::path dirPath(dir);
  for (std::filesystem::directory_iterator it(dirPath, ec), end; it != end; ++it) {
    if (!std::filesystem::is_regular_file(it->status())) continue;
    std::string filename = it->path().filename().string();
    if (filename.size() > 3 && filename.substr(filename.size() - 3) == ".md" &&
        filename[0] == 'T' && filename.size() > 4 &&
        std::isdigit(filename[1]) && std::isdigit(filename[2]) && std::isdigit(filename[3]) &&
        filename[4] == '-') {
      taskFiles.push_back(it->path().string());
    }
  }
  if (ec) {
    return Result<void>::failure("Cannot scan " + dir);
  }

  if (taskFiles.empty()) {
    std::cout << "No T*.md files found in " << dir << "/" << std::endl;
    return Result<void>::success();
  }

  StatusFile sf;
  std::vector<std::string> errors;

for (const std::string& filePath : taskFiles) {
    // Extract ID and name from filename
    std::string filename = filePath;
    size_t slashPos = filename.rfind('/');
    if (slashPos != std::string::npos) {
      filename = filename.substr(slashPos + 1);
    }

    // Remove .md extension
    if (filename.size() > 3 &&
        filename.substr(filename.size() - 3) == ".md") {
      filename = filename.substr(0, filename.size() - 3);
    }

    std::string id = filename.substr(0, 4); // T001
    std::string kebabName;
    if (filename.size() > 5) {
      kebabName = filename.substr(5); // project-setup
    }

    if (!isValidTaskId(id)) {
      errors.push_back("Invalid filename: " + filePath);
      continue;
    }

    // Read file content
    Result<std::string> contentResult = readTaskFile(filePath);
    if (contentResult.hasError()) {
      errors.push_back(contentResult.errorMessage());
      continue;
    }

    std::string content = contentResult.value;
    Task task;
    task.id = id;
    task.name = kebabToTitle(kebabName);

    // Parse optional status line
    std::string statusStr = extractStatusLine(content);
    if (!statusStr.empty()) {
      task.status = stringToStatus(statusStr);
    }

    // Parse dependencies
    task.depends = extractDepends(content);

    sf.tasks[id] = task;
  }

  // Validate dependencies exist
for (std::pair<const std::string, Task>& kv : sf.tasks) {
for (const std::string& dep : kv.second.depends) {
      if (sf.tasks.find(dep) == sf.tasks.end()) {
        errors.push_back("Task " + kv.first + " depends on " + dep +
                         " which was not found");
      }
    }
  }

  // Validate no circular dependencies
  for (const auto& kv : sf.tasks) {
    Result<void> cycleResult = validateCircularDependencies(
        kv.first, kv.second.depends, sf.tasks);
    if (cycleResult.hasError()) {
      errors.push_back(cycleResult.errorMessage());
    }
  }

  if (!errors.empty()) {
    // Still write, but report errors
    std::cerr << "warning: " << errors.size() << " issue(s) found"
              << std::endl;
for (const std::string& err : errors) {
      std::cerr << "  " << err << std::endl;
    }
  }

  Result<void> writeResult = writeStatusFile(dir, sf);
  if (writeResult.hasError()) return writeResult;

  int done = countStatus(sf.tasks, Status::Done);
  int inProg = countStatus(sf.tasks, Status::InProgress);
  int pend = countStatus(sf.tasks, Status::Pending);

  std::cout << "Found " << sf.tasks.size() << " task files" << std::endl;
  if (force) {
    std::cout << "Overwrote status.yaml with " << sf.tasks.size()
              << " tasks" << std::endl;
  } else {
    std::cout << "Created status.yaml with " << sf.tasks.size()
              << " tasks" << std::endl;
  }
  std::cout << "Status distribution: " << done << " done, "
            << inProg << " in_progress, " << pend << " pending"
            << std::endl;

  return Result<void>::success();
}

Result<void> Commands::new_(const std::string& tasksDir,
    const std::string& name,
    const std::vector<std::string>& depends,
    int phase,
    bool critical) {
  if (name.empty()) {
    return Result<void>::failure("Task name cannot be empty");
  }

  std::string dir = resolveTaskDir(tasksDir);

  StatusFile sf;
  Result<StatusFile> readResult = readStatusFile(dir);
  if (!readResult.hasError()) {
    sf = readResult.value;
  }

  // Check for duplicate names
  for (const auto& kv : sf.tasks) {
    if (kv.second.name == name) {
      std::cerr << "warning: Task with similar name exists: "
                << kv.first << "-" << toKebabCase(name) << ".md"
                << std::endl;
    }
  }

  // Validate depends
for (const std::string& dep : depends) {
    if (!isValidTaskId(dep)) {
      return Result<void>::failure("Invalid dependency ID: " + dep);
    }
    if (sf.tasks.find(dep) == sf.tasks.end()) {
      return Result<void>::failure("Dependency " + dep + " not found");
    }
  }

  std::string nextId = findNextTaskId(sf.tasks);

  // Build a temporary task to validate cycles
  Task tempTask;
  tempTask.id = nextId;
  tempTask.depends = depends;
  sf.tasks[nextId] = tempTask;

  // Validate circular deps
  Result<void> cycleResult = validateCircularDependencies(
      nextId, depends, sf.tasks);
  if (cycleResult.hasError()) {
    sf.tasks.erase(nextId);
    return cycleResult;
  }

  sf.tasks.erase(nextId);

  // Create task file
  std::string filePath = taskFilePath(dir, nextId, name);
  Result<void> fileResult = writeTaskFile(filePath, nextId, name);
  if (fileResult.hasError()) return fileResult;

  // Add to status
  Task task;
  task.id = nextId;
  task.name = name;
  task.status = Status::Pending;
  task.depends = depends;
  task.phase = phase;
  task.critical = critical;

  sf.tasks[nextId] = task;

  Result<void> writeResult = writeStatusFile(dir, sf);
  if (writeResult.hasError()) return writeResult;

  std::cout << "Created " << nextId << "-" << toKebabCase(name)
            << ".md" << std::endl;
  std::cout << "Updated status.yaml" << std::endl;

  return Result<void>::success();
}

Result<void> Commands::status(const std::string& tasksDir) {
  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  // Determine next task
  std::string nextTaskId;
  for (const auto& kv : sf.tasks) {
    if (kv.second.status != Status::Pending) continue;
    if (!allDepsDone(kv.second, sf.tasks)) continue;

    if (nextTaskId.empty()) {
      nextTaskId = kv.first;
    } else {
      // Prioritize: critical → phase → task number
      const Task& current = kv.second;
      const Task& best = sf.tasks.at(nextTaskId);
      bool currentBetter = false;

      if (current.critical && !best.critical) {
        currentBetter = true;
      } else if (current.critical == best.critical) {
        if (current.phase < best.phase) {
          currentBetter = true;
        } else if (current.phase == best.phase &&
                   parseTaskId(current.id) < parseTaskId(best.id)) {
          currentBetter = true;
        }
      }

      if (currentBetter) {
        nextTaskId = kv.first;
      }
    }
  }

  // Group by phase
  std::map<int, std::vector<std::string>> byPhase;
  for (const auto& kv : sf.tasks) {
    byPhase[kv.second.phase].push_back(kv.first);
  }

  // Sort phases
  std::vector<int> phaseNumbers;
  for (const auto& kv : byPhase) {
    phaseNumbers.push_back(kv.first);
  }
  std::sort(phaseNumbers.begin(), phaseNumbers.end());

  // Display
  for (int pNum : phaseNumbers) {
    std::string phaseName;
    auto phaseIt = sf.config.phases.find(pNum);
    if (phaseIt != sf.config.phases.end()) {
      phaseName = phaseIt->second;
    }

    std::cout << "Phase " << pNum;
    if (!phaseName.empty()) {
      std::cout << ": " << phaseName;
    }
    std::cout << std::endl;

    std::vector<std::string>& ids = byPhase[pNum];
    std::sort(ids.begin(), ids.end());

for (const std::string& id : ids) {
      const Task& t = sf.tasks.at(id);
      std::cout << "  " << t.id << "  " << t.name;

      // Padding
      int padding = 20 - static_cast<int>(t.name.size());
      if (padding < 1) padding = 1;
      for (int i = 0; i < padding; ++i) std::cout << ' ';

      std::cout << statusColor(t.status);

      if (t.status == Status::Pending) {
        if (id == nextTaskId) {
          std::cout << "  \u2190 next (dependencies met)";
        } else if (!allDepsDone(t, sf.tasks)) {
          std::string blockers;
          for (const auto& dep : t.depends) {
            auto depIt = sf.tasks.find(dep);
            if (depIt != sf.tasks.end() &&
                depIt->second.status != Status::Done) {
              if (!blockers.empty()) blockers += ", ";
              blockers += depIt->first;
            }
          }
          std::cout << "  \u2190 blocked by " << blockers;
        }
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  // Progress
  int total = sf.tasks.size();
  int done = countStatus(sf.tasks, Status::Done);
  int inProg = countStatus(sf.tasks, Status::InProgress);
  int pend = countStatus(sf.tasks, Status::Pending);

  std::cout << "Progress: " << done << "/" << total << " done, "
            << inProg << " in_progress, " << pend << " pending"
            << std::endl;

  return Result<void>::success();
}

Result<void> Commands::next(const std::string& tasksDir) {
  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  // Find candidates: pending with all deps done
  std::vector<std::string> candidates;
  for (const auto& kv : sf.tasks) {
    if (kv.second.status == Status::Pending &&
        allDepsDone(kv.second, sf.tasks)) {
      candidates.push_back(kv.first);
    }
  }

  if (candidates.empty()) {
    std::cout << "info: All tasks blocked or complete" << std::endl;
    return Result<void>::success();
  }

  // Prioritize
  std::sort(candidates.begin(), candidates.end(),
      [&](const std::string& a, const std::string& b) {
        const Task& ta = sf.tasks.at(a);
        const Task& tb = sf.tasks.at(b);
        if (ta.critical != tb.critical) return ta.critical;
        if (ta.phase != tb.phase) return ta.phase < tb.phase;
        return parseTaskId(ta.id) < parseTaskId(tb.id);
      });

  std::string bestId = candidates[0];
  const Task& best = sf.tasks.at(bestId);

  std::cout << "Next: " << best.id << " \u2014 " << best.name << std::endl;

  // Dependencies
  if (!best.depends.empty()) {
    std::cout << "  Depends on:";
  for (const auto& dep : best.depends) {
      auto depIt = sf.tasks.find(dep);
      if (depIt != sf.tasks.end()) {
        std::cout << " " << depIt->first << " ["
                  << statusToString(depIt->second.status) << "]";
        if (depIt->second.status == Status::Done) {
          std::cout << " \u2713";
        } else {
          std::cout << " \u2717";
        }
      }
    }
    std::cout << std::endl;
  }

  // Read task file
  std::string filePath = taskFilePath(dir, best.id, best.name);
  Result<std::string> contentResult = readTaskFile(filePath);
  if (!contentResult.hasError()) {
    std::string content = contentResult.value;
    std::string goal = extractGoal(content);
    if (!goal.empty()) {
      std::cout << "  Goal: " << goal << std::endl;
    }

    std::string firstStep = getFirstStep(content);
    if (!firstStep.empty()) {
      std::cout << "  First step: " << firstStep << std::endl;
    }
  }

  // Files
  if (!best.files.empty()) {
    std::cout << "  Files:";
for (const std::string& f : best.files) {
      std::cout << " " << f;
    }
    std::cout << std::endl;
  }

  // Specs
  if (!best.specs.empty()) {
    std::cout << "  Specs:";
for (const std::string& s : best.specs) {
      std::cout << " " << s;
    }
    std::cout << std::endl;
  }

  return Result<void>::success();
}

Result<void> Commands::do_(const std::string& tasksDir,
    const std::string& taskId, bool force) {
  if (!isValidTaskId(taskId)) {
    return Result<void>::failure(
        "Invalid task ID format. Expected TXXX (see Task ID Format)");
  }

  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  auto taskIt = sf.tasks.find(taskId);
  if (taskIt == sf.tasks.end()) {
    return Result<void>::failure("Task " + taskId + " not found");
  }

  Task& task = taskIt->second;

  if (task.status == Status::InProgress) {
    return Result<void>::failure("Task " + taskId + " already in_progress");
  }
  if (task.status == Status::Done) {
    return Result<void>::failure("Task " + taskId + " already done");
  }

  // Check dependencies
  if (!force && !allDepsDone(task, sf.tasks)) {
    std::string blockers;
    for (const auto& dep : task.depends) {
      auto depIt = sf.tasks.find(dep);
      if (depIt != sf.tasks.end() &&
          depIt->second.status != Status::Done) {
        if (!blockers.empty()) blockers += ", ";
        blockers += dep + " (" + statusToString(depIt->second.status) + ")";
      }
    }
    return Result<void>::failure(
        "Unmet dependencies: " + blockers + ". Use --force to proceed");
  }

  task.status = Status::InProgress;
  Result<void> writeResult = writeStatusFile(dir, sf);
  if (writeResult.hasError()) return writeResult;

  std::cout << "Started " << taskId << " \u2014 " << task.name << std::endl;
  std::cout << "Status changed: pending \u2192 in_progress" << std::endl;

  // Read and display task info
  std::string filePath = taskFilePath(dir, taskId, task.name);
  Result<std::string> contentResult = readTaskFile(filePath);
  if (!contentResult.hasError()) {
    std::cout << std::endl << "Now reading " << filePath << "..." << std::endl;
    std::string content = contentResult.value;
    std::string goal = extractGoal(content);
    if (!goal.empty()) {
      std::cout << "Goal: " << goal << std::endl;
    }
    std::string firstStep = getFirstStep(content);
    if (!firstStep.empty()) {
      std::cout << "First step: " << firstStep << std::endl;
    }
  }

  return Result<void>::success();
}

Result<void> Commands::done(const std::string& tasksDir,
    const std::string& taskId) {
  if (!isValidTaskId(taskId)) {
    return Result<void>::failure(
        "Invalid task ID format. Expected TXXX (see Task ID Format)");
  }

  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  auto taskIt = sf.tasks.find(taskId);
  if (taskIt == sf.tasks.end()) {
    return Result<void>::failure("Task " + taskId + " not found");
  }

  if (taskIt->second.status == Status::Done) {
    return Result<void>::failure("Task " + taskId + " already done");
  }

  taskIt->second.status = Status::Done;
  Result<void> writeResult = writeStatusFile(dir, sf);
  if (writeResult.hasError()) return writeResult;

  std::cout << "\u2713 " << taskId << " marked as done" << std::endl;

  // Find newly unblocked tasks
  std::vector<std::string> unblocked;
  for (const auto& kv : sf.tasks) {
    if (kv.second.status != Status::Pending) continue;
    if (!allDepsDone(kv.second, sf.tasks)) continue;
    // Previously blocked by this task
    for (const auto& dep : kv.second.depends) {
      if (dep == taskId) {
        unblocked.push_back(kv.first);
        break;
      }
    }
  }

  if (!unblocked.empty()) {
    std::cout << std::endl << "Unblocked tasks:" << std::endl;
for (const std::string& id : unblocked) {
      const Task& t = sf.tasks.at(id);
      std::cout << "  " << id << "  " << t.name << "  [pending]"
                << std::endl;
    }
  }

  return Result<void>::success();
}

Result<void> Commands::pause(const std::string& tasksDir,
    const std::string& taskId) {
  if (!isValidTaskId(taskId)) {
    return Result<void>::failure(
        "Invalid task ID format. Expected TXXX (see Task ID Format)");
  }

  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  auto taskIt = sf.tasks.find(taskId);
  if (taskIt == sf.tasks.end()) {
    return Result<void>::failure("Task " + taskId + " not found");
  }

  if (taskIt->second.status == Status::Pending) {
    return Result<void>::failure("Task " + taskId + " already pending");
  }

  std::string oldStatus = statusToString(taskIt->second.status);
  taskIt->second.status = Status::Pending;
  Result<void> writeResult = writeStatusFile(dir, sf);
  if (writeResult.hasError()) return writeResult;

  std::cout << "Paused " << taskId << " \u2014 " << taskIt->second.name
            << std::endl;
  std::cout << "Status changed: " << oldStatus << " \u2192 pending"
            << std::endl;

  return Result<void>::success();
}

Result<void> Commands::deps(const std::string& tasksDir,
    const std::string& taskId) {
  if (!isValidTaskId(taskId)) {
    return Result<void>::failure(
        "Invalid task ID format. Expected TXXX (see Task ID Format)");
  }

  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  auto taskIt = sf.tasks.find(taskId);
  if (taskIt == sf.tasks.end()) {
    return Result<void>::failure("Task " + taskId + " not found");
  }

  const Task& task = taskIt->second;

  std::cout << taskId << " depends on:" << std::endl;
  if (task.depends.empty()) {
    std::cout << "  (none)" << std::endl;
  } else {
  for (const auto& dep : task.depends) {
      auto depIt = sf.tasks.find(dep);
      std::cout << "  " << dep;
      if (depIt != sf.tasks.end()) {
        std::cout << "  " << depIt->second.name << "  ["
                  << statusToString(depIt->second.status) << "]";
        if (depIt->second.status == Status::Done) {
          std::cout << " \u2713";
        } else {
          std::cout << " \u2717";
        }
      }
      std::cout << std::endl;
    }
  }

  std::vector<std::string> dependents;
  for (const auto& kv : sf.tasks) {
    for (const auto& dep : kv.second.depends) {
      if (dep == taskId) {
        dependents.push_back(kv.first);
        break;
      }
    }
  }

  std::cout << std::endl << "Tasks waiting on " << taskId << ":"
            << std::endl;
  if (dependents.empty()) {
    std::cout << "  (none)" << std::endl;
  } else {
    for (const auto& id : dependents) {
      auto depIt = sf.tasks.find(id);
      if (depIt != sf.tasks.end()) {
        std::cout << "  " << id << "  " << depIt->second.name
                  << "  [" << statusToString(depIt->second.status) << "]"
                  << std::endl;
      }
    }
  }

  return Result<void>::success();
}

Result<void> Commands::log(const std::string& tasksDir,
    const std::string& taskId,
    const std::string& message) {
  if (!isValidTaskId(taskId)) {
    return Result<void>::failure(
        "Invalid task ID format. Expected TXXX (see Task ID Format)");
  }

  if (message.empty()) {
    return Result<void>::failure("Log message cannot be empty");
  }

  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  auto taskIt = sf.tasks.find(taskId);
  if (taskIt == sf.tasks.end()) {
    return Result<void>::failure("Task " + taskId + " not found");
  }

  std::string filePath = taskFilePath(dir, taskId, taskIt->second.name);
  Result<void> logResult = appendLog(filePath, message);
  if (logResult.hasError()) return logResult;

  std::cout << "Logged to " << filePath << std::endl;
  return Result<void>::success();
}

Result<void> Commands::edit(const std::string& tasksDir,
    const std::string& taskId,
    const std::string& status,
    const std::vector<std::string>& depends,
    const std::string& phase,
    bool criticalSet,
    bool criticalVal,
    const std::vector<std::string>& files,
    const std::vector<std::string>& specs,
    const std::string& phases,
    const std::string& criticalPath) {

  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  // Project-level edits
  if (taskId.empty()) {
    bool changed = false;

    if (!phases.empty()) {
      std::map<int, std::string> newPhases;
      std::vector<std::string> pairs = split(phases, ',');
for (const std::string& pair : pairs) {
        std::vector<std::string> parts = split(pair, ':');
        if (parts.size() == 2) {
          int num = std::stoi(parts[0]);
          newPhases[num] = trim(parts[1]);
        }
      }
      sf.config.phases = newPhases;
      changed = true;
    }

    if (!criticalPath.empty()) {
      std::vector<std::string> cp = split(criticalPath, ',');
      // Validate task IDs exist
for (const std::string& id : cp) {
        if (sf.tasks.find(id) == sf.tasks.end()) {
          return Result<void>::failure(
              "Task " + id + " in critical path not found");
        }
      }
      sf.config.criticalPath = cp;
      changed = true;
    }

    if (!changed) {
      return Result<void>::failure(
          "No project-level changes specified. Use --phases or --critical-path");
    }

    Result<void> writeResult = writeStatusFile(dir, sf);
    if (writeResult.hasError()) return writeResult;

    if (!phases.empty()) {
      std::cout << "Updated phases mapping" << std::endl;
    }
    if (!criticalPath.empty()) {
      std::cout << "Updated critical path" << std::endl;
    }

    return Result<void>::success();
  }

  // Task-level edits
  if (!isValidTaskId(taskId)) {
    return Result<void>::failure(
        "Invalid task ID format. Expected TXXX (see Task ID Format)");
  }

  auto taskIt = sf.tasks.find(taskId);
  if (taskIt == sf.tasks.end()) {
    return Result<void>::failure("Task " + taskId + " not found");
  }

  Task& task = taskIt->second;
  bool changed = false;

  if (!status.empty()) {
    if (!isValidStatus(status)) {
      return Result<void>::failure(
          "Invalid status. Must be: pending, in_progress, or done");
    }
    task.status = stringToStatus(status);
    changed = true;
  }

  if (!depends.empty()) {
    // Validate depends exist
    Result<void> depResult = validateDependsExist(depends, sf.tasks);
    if (depResult.hasError()) return depResult;

    // Validate no circular deps
    Result<void> cycleResult = validateCircularDependencies(
        taskId, depends, sf.tasks);
    if (cycleResult.hasError()) return cycleResult;

    task.depends = depends;
    changed = true;
  }

  if (!phase.empty()) {
    int p = std::stoi(phase);
    if (p < 0) {
      return Result<void>::failure("Phase must be non-negative");
    }
    task.phase = p;
    changed = true;
  }

  if (criticalSet) {
    task.critical = criticalVal;
    changed = true;
  }

  if (!files.empty()) {
    task.files = files;
    changed = true;
  }

  if (!specs.empty()) {
    task.specs = specs;
    changed = true;
  }

  if (!changed) {
    return Result<void>::failure(
        "No changes specified. Use --status, --phase, --critical, --depends, --files, or --specs");
  }

  Result<void> writeResult = writeStatusFile(dir, sf);
  if (writeResult.hasError()) return writeResult;

  std::cout << "Updated " << taskId;
  if (!status.empty()) {
    std::cout << " status: " << status;
  }
  if (!depends.empty()) {
    std::cout << " depends: ";
    for (size_t i = 0; i < depends.size(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << depends[i];
    }
  }
  if (!phase.empty()) {
    std::cout << " phase: " << phase;
  }
  if (criticalSet) {
    std::cout << " critical: " << (criticalVal ? "true" : "false");
  }
  std::cout << std::endl;

  return Result<void>::success();
}

Result<void> Commands::summary(const std::string& tasksDir) {
  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  int total = sf.tasks.size();
  int done = countStatus(sf.tasks, Status::Done);
  int inProg = countStatus(sf.tasks, Status::InProgress);
  int pend = countStatus(sf.tasks, Status::Pending);

  std::cout << "Task Summary" << std::endl;
  std::cout << "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
               "\u2500\u2500\u2500" << std::endl;
  std::cout << "Total tasks:    " << total << std::endl;

  auto printPct = [](int count, int total) {
    if (total == 0) return std::string("0.0%");
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed << (count * 100.0 / total) << "%";
    return oss.str();
  };

  std::cout << "Done:           " << done << " ("
            << printPct(done, total) << ")" << std::endl;
  std::cout << "In progress:    " << inProg << " ("
            << printPct(inProg, total) << ")" << std::endl;
  std::cout << "Pending:        " << pend << " ("
            << printPct(pend, total) << ")" << std::endl;

  if (!sf.config.phases.empty() || !sf.tasks.empty()) {
    std::cout << std::endl << "By Phase:" << std::endl;

    // Group by phase
    std::map<int, int> phaseTotal, phaseDone;
    for (const auto& kv : sf.tasks) {
      phaseTotal[kv.second.phase]++;
      if (kv.second.status == Status::Done) {
        phaseDone[kv.second.phase]++;
      }
    }

    for (const auto& kv : phaseTotal) {
      auto phaseIt = sf.config.phases.find(kv.first);
      std::cout << "  Phase " << kv.first;
      if (phaseIt != sf.config.phases.end()) {
        std::cout << " (" << phaseIt->second << ")";
      }
      std::cout << ": " << phaseDone[kv.first] << "/" << kv.second
                << " done" << std::endl;
    }
  }

  if (!sf.config.criticalPath.empty()) {
    std::cout << std::endl << "Critical Path: ";
    for (size_t i = 0; i < sf.config.criticalPath.size(); ++i) {
      if (i > 0) std::cout << " \u2192 ";
      std::cout << sf.config.criticalPath[i];
    }
    std::cout << std::endl;

    int cpDone = 0, cpInProg = 0, cpPend = 0;
    for (const auto& id : sf.config.criticalPath) {
      auto it = sf.tasks.find(id);
      if (it != sf.tasks.end()) {
        switch (it->second.status) {
          case Status::Done: ++cpDone; break;
          case Status::InProgress: ++cpInProg; break;
          case Status::Pending: ++cpPend; break;
        }
      }
    }

    std::cout << "  Status: " << cpDone << "/"
              << sf.config.criticalPath.size() << " done, "
              << cpInProg << " in_progress, " << cpPend << " pending"
              << std::endl;
  }

  return Result<void>::success();
}

Result<void> Commands::remove(const std::string& tasksDir,
    const std::string& taskId, bool force) {
  if (!isValidTaskId(taskId)) {
    return Result<void>::failure(
        "Invalid task ID format. Expected TXXX (see Task ID Format)");
  }

  std::string dir = resolveTaskDir(tasksDir);

  Result<StatusFile> readResult = readStatusFile(dir);
  if (readResult.hasError()) return Result<void>::failure(readResult.errorMessage());

  StatusFile sf = readResult.value;

  auto taskIt = sf.tasks.find(taskId);
  if (taskIt == sf.tasks.end()) {
    return Result<void>::failure("Task " + taskId + " not found");
  }

  // Check for dependents
  std::vector<std::string> dependents;
  for (const auto& kv : sf.tasks) {
    for (const auto& dep : kv.second.depends) {
      if (dep == taskId) {
        dependents.push_back(kv.first);
        break;
      }
    }
  }

  if (!dependents.empty()) {
    std::cerr << "warning: The following tasks depend on " << taskId << ":";
    for (const auto& id : dependents) {
      std::cerr << " " << id;
    }
    std::cerr << std::endl;
  }

  std::string taskName = taskIt->second.name;
  sf.tasks.erase(taskIt);

  // Also remove from critical path if present
  std::vector<std::string>& cp = sf.config.criticalPath;
  cp.erase(std::remove(cp.begin(), cp.end(), taskId), cp.end());

  Result<void> writeResult = writeStatusFile(dir, sf);
  if (writeResult.hasError()) return writeResult;

  std::cout << "Removed " << taskId << " \u2014 " << taskName << std::endl;
  std::cout << "Updated status.yaml" << std::endl;

  if (!force) {
    std::cout << "To also remove " << taskId << "-" << toKebabCase(taskName)
              << ".md, use --force" << std::endl;
  }

  return Result<void>::success();
}

Result<void> Commands::installSkills(bool project) {
  // Find source directory
  std::string sourceDir;
  std::string installedPath =
      std::string(TASKPAD_DATA_DIR) + "/share/taskpad/skills/taskpad";
  std::string devPath = ".agents/skills/taskpad";

  if (fileExists(installedPath + "/SKILL.md")) {
    sourceDir = installedPath;
  } else if (fileExists(devPath + "/SKILL.md")) {
    sourceDir = devPath;
  } else {
    return Result<void>::failure("Skill files not found");
  }

  std::string targetDir;
  if (project) {
    targetDir = ".agents/skills/taskpad";
  } else {
    const char* home = std::getenv("HOME");
    if (!home) {
      return Result<void>::failure("HOME environment variable not set");
    }
    targetDir = std::string(home) + "/.agents/skills/taskpad";
  }

  std::string targetFile = targetDir + "/SKILL.md";

  if (fileExists(targetFile)) {
    return Result<void>::failure(
        "Already installed. Use --force to overwrite");
  }

  if (!copyFile(sourceDir + "/SKILL.md", targetFile)) {
    return Result<void>::failure("Failed to install skill to " + targetDir);
  }

  std::cout << "Installed skill to " << targetDir << "/" << std::endl;
  return Result<void>::success();
}

} // namespace taskpad
