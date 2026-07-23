#include "storage.h"
#include "utils.h"
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace taskpad {

static bool fileExists(const std::string& path) {
  std::ifstream f(path.c_str());
  return f.good();
}

static std::string readFile(const std::string& path) {
  std::ifstream f(path);
  if (!f) return "";
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static bool writeFile(const std::string& path, const std::string& content) {
  std::ofstream f(path);
  if (!f) return false;
  f << content;
  return f.good();
}

// ---- Config ----

Result<std::string> readTaskDir(const std::string& projectRoot) {
  std::string configPath = normalizePath(projectRoot) + "/.taskpad";
  if (!fileExists(configPath)) {
    return Result<std::string>::failure(
        "Not initialized. Run 'taskpad init' first");
  }

  try {
    YAML::Node config = YAML::LoadFile(configPath);
    if (!config.IsMap() || !config["task-dir"]) {
      return Result<std::string>::failure(
          "Invalid .taskpad format. Expected YAML mapping with 'task-dir'");
    }
    return Result<std::string>::success(config["task-dir"].as<std::string>());
  } catch (const YAML::Exception& e) {
    return Result<std::string>::failure(
        "Invalid .taskpad format: " + std::string(e.what()));
  }
}

Result<void> createConfig(const std::string& projectRoot,
    const std::string& taskDir) {
  std::string configPath = normalizePath(projectRoot) + "/.taskpad";
  if (fileExists(configPath)) {
    return Result<void>::failure(
        "Already initialized. Remove .taskpad to re-initialize");
  }

  YAML::Emitter out;
  out << YAML::Comment("taskpad project config");
  out << YAML::BeginMap;
  out << YAML::Key << "task-dir";
  out << YAML::Value << taskDir;
  out << YAML::EndMap;

  std::ofstream f(configPath);
  if (!f) {
    return Result<void>::failure(
        "Cannot write to " + configPath + ". Check permissions");
  }
  f << out.c_str() << "\n";
  return Result<void>::success();
}

bool configExists(const std::string& projectRoot) {
  return fileExists(normalizePath(projectRoot) + "/.taskpad");
}

// ---- Status File ----

static std::vector<std::string> parseStringList(const YAML::Node& node) {
  std::vector<std::string> result;
  if (!node.IsDefined() || !node.IsSequence()) return result;
  for (const auto& item : node) {
    result.push_back(item.as<std::string>());
  }
  return result;
}

Result<StatusFile> readStatusFile(const std::string& taskDir) {
  std::string path = normalizePath(taskDir) + "/status.yaml";
  if (!fileExists(path)) {
    return Result<StatusFile>::failure(
        "No status.yaml found. Run 'taskpad import' or 'taskpad new' first");
  }

  try {
    YAML::Node root = YAML::LoadFile(path);
    if (!root.IsMap()) {
      return Result<StatusFile>::failure(
          "Invalid status.yaml format. Expected YAML mapping");
    }

    StatusFile sf;

    // Parse tasks
    YAML::Node tasksNode = root["tasks"];
    if (tasksNode.IsDefined() && tasksNode.IsMap()) {
      for (const auto& kv : tasksNode) {
        std::string id = kv.first.as<std::string>();
        YAML::Node t = kv.second;

        Task task;
        task.id = id;
        task.name = t["name"] ? t["name"].as<std::string>() : "";
        task.status = t["status"] ? stringToStatus(t["status"].as<std::string>())
                                  : Status::Pending;
        task.depends = parseStringList(t["depends"]);
        task.phase = t["phase"] ? t["phase"].as<int>() : 0;
        task.critical = t["critical"] ? t["critical"].as<bool>() : false;
        task.files = parseStringList(t["files"]);
        task.specs = parseStringList(t["specs"]);
        sf.tasks[id] = task;
      }
    }

    // Parse phases
    YAML::Node phasesNode = root["phases"];
    if (phasesNode.IsDefined() && phasesNode.IsMap()) {
      for (const auto& kv : phasesNode) {
        int phaseNum = std::stoi(kv.first.as<std::string>());
        sf.config.phases[phaseNum] = kv.second.as<std::string>();
      }
    }

    // Parse critical_path
    YAML::Node cpNode = root["critical_path"];
    if (cpNode.IsDefined() && cpNode.IsSequence()) {
      sf.config.criticalPath = parseStringList(cpNode);
    }

    return Result<StatusFile>::success(sf);
  } catch (const YAML::Exception& e) {
    return Result<StatusFile>::failure(
        "Invalid status.yaml format: " + std::string(e.what()));
  }
}

Result<void> writeStatusFile(const std::string& taskDir,
    const StatusFile& sf) {
  YAML::Node root;

  // Tasks
  YAML::Node tasksNode;
  for (const auto& kv : sf.tasks) {
    const Task& t = kv.second;
    YAML::Node taskNode;
    taskNode["name"] = t.name;
    taskNode["status"] = statusToString(t.status);

    YAML::Node dependsNode;
    for (const auto& d : t.depends) {
      dependsNode.push_back(d);
    }
    taskNode["depends"] = dependsNode;

    taskNode["phase"] = t.phase;
    taskNode["critical"] = t.critical;

    YAML::Node filesNode;
    for (const auto& f : t.files) {
      filesNode.push_back(f);
    }
    taskNode["files"] = filesNode;

    YAML::Node specsNode;
    for (const auto& s : t.specs) {
      specsNode.push_back(s);
    }
    taskNode["specs"] = specsNode;

    tasksNode[t.id] = taskNode;
  }
  root["tasks"] = tasksNode;

  // Phases
  if (!sf.config.phases.empty()) {
    YAML::Node phasesNode;
    for (const auto& kv : sf.config.phases) {
      phasesNode[std::to_string(kv.first)] = kv.second;
    }
    root["phases"] = phasesNode;
  }

  // Critical path
  if (!sf.config.criticalPath.empty()) {
    YAML::Node cpNode;
    for (const auto& id : sf.config.criticalPath) {
      cpNode.push_back(id);
    }
    root["critical_path"] = cpNode;
  }

  std::string path = normalizePath(taskDir) + "/status.yaml";
  std::ofstream f(path);
  if (!f) {
    return Result<void>::failure(
        "Cannot write to " + path + ". Check permissions");
  }
  f << YAML::Dump(root);
  return Result<void>::success();
}

// ---- Task Files ----

std::string taskFilePath(const std::string& taskDir,
    const std::string& taskId,
    const std::string& taskName) {
  std::string dir = normalizePath(taskDir);
  std::string kebab = toKebabCase(taskName);
  if (kebab.empty()) {
    return dir + "/" + taskId + ".md";
  }
  return dir + "/" + taskId + "-" + kebab + ".md";
}

Result<std::string> readTaskFile(const std::string& path) {
  if (!fileExists(path)) {
    return Result<std::string>::failure(
        "Task file " + path + " not found");
  }
  return Result<std::string>::success(readFile(path));
}

Result<void> writeTaskFile(const std::string& path,
    const std::string& taskId,
    const std::string& taskName) {
  std::string content =
    "# " + taskId + ": " + taskName + "\n"
    "\n"
    "## Goal\n"
    "\n"
    "(Describe the goal)\n"
    "\n"
    "## Depends On\n"
    "\n"
    "(None)\n"
    "\n"
    "## Spec References\n"
    "\n"
    "- (Add spec references here)\n"
    "\n"
    "## Files to Create/Modify\n"
    "\n"
    "- (Add files here)\n"
    "\n"
    "## Implementation Steps\n"
    "\n"
    "1. (Add steps here)\n"
    "\n"
    "## Constraints\n"
    "\n"
    "- (Add constraints here)\n"
    "\n"
    "## Acceptance Criteria\n"
    "\n"
    "- [ ] (Add criteria here)\n"
    "\n"
    "## Notes\n"
    "\n"
    "(filled in during/after implementation)\n";

  if (!writeFile(path, content)) {
    return Result<void>::failure(
        "Cannot write to " + path + ". Check permissions");
  }
  return Result<void>::success();
}

Result<void> appendLog(const std::string& path,
    const std::string& message) {
  if (!fileExists(path)) {
    return Result<void>::failure("Task file " + path + " not found");
  }

  std::string content = readFile(path);
  std::string entry = "- [" + currentTimestamp() + "] " + message;

  // Find or create ## Notes section
  size_t notesPos = content.rfind("\n## Notes");
  if (notesPos == std::string::npos) {
    notesPos = content.rfind("## Notes");
  }

  if (notesPos != std::string::npos) {
    // Find end of Notes section (next ## or end of file)
    size_t insertPos = notesPos;
    // Skip past "## Notes" line
    size_t eol = content.find('\n', notesPos + 2);
    if (eol != std::string::npos) {
      insertPos = eol + 1;
    }
    // Find next ## if any
    size_t nextSection = content.find("\n## ", insertPos);
    if (nextSection == std::string::npos) {
      content = content + "\n" + entry + "\n";
    } else {
      content = content.substr(0, nextSection) + "\n" + entry +
                content.substr(nextSection);
    }
  } else {
    // No Notes section found, append one
    if (!content.empty() && content.back() != '\n') {
      content += '\n';
    }
    content += "\n## Notes\n\n" + entry + "\n";
  }

  if (!writeFile(path, content)) {
    return Result<void>::failure(
        "Cannot write to " + path + ". Check permissions");
  }
  return Result<void>::success();
}

} // namespace taskpad
