#include "cli.h"
#include "commands.h"
#include "utils.h"

#include <CLI/CLI.hpp>
#include <iostream>
#include <cstdlib>

namespace taskpad {

int runCLI(int argc, char** argv) {
  CLI::App app{"taskpad - Task Management CLI"};
  app.require_subcommand(1);

  std::string tasksDir;
  app.add_option("--tasks-dir", tasksDir, "Override task directory");

  int exitCode = 0;

  // ---- init ----
  CLI::App* initCmd = app.add_subcommand("init", "Initialize task tracking");
  initCmd->callback([&]() {
    Result<void> r = Commands::init(tasksDir);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- import ----
  CLI::App* importCmd = app.add_subcommand("import",
      "Import existing task files into taskpad");
  bool importForce = false;
  importCmd->add_flag("--force", importForce,
      "Overwrite existing status.yaml");
  importCmd->callback([&]() {
    Result<void> r = Commands::import_(tasksDir, importForce);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- new ----
  CLI::App* newCmd = app.add_subcommand("new", "Create a new task");
  std::string newName;
  newCmd->add_option("name", newName, "Task name")->required();
  std::vector<std::string> newDepends;
  newCmd->add_option("--depends", newDepends,
      "Dependency task ID (may be repeated)")
      ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);
  int newPhase = 0;
  newCmd->add_option("--phase", newPhase, "Phase number");
  bool newCritical = false;
  newCmd->add_flag("--critical", newCritical, "Mark as critical path");
  newCmd->callback([&]() {
    Result<void> r = Commands::new_(tasksDir, newName, newDepends, newPhase, newCritical);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- status ----
  CLI::App* statusCmd = app.add_subcommand("status",
      "Show all tasks with their status");
  statusCmd->callback([&]() {
    Result<void> r = Commands::status(tasksDir);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- next ----
  CLI::App* nextCmd = app.add_subcommand("next",
      "Show the next task to work on");
  nextCmd->callback([&]() {
    Result<void> r = Commands::next(tasksDir);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- do ----
  CLI::App* doCmd = app.add_subcommand("do", "Start working on a task");
  std::string doId;
  doCmd->add_option("id", doId, "Task ID (e.g. T001)")->required();
  bool doForce = false;
  doCmd->add_flag("--force", doForce,
      "Skip dependency check");
  doCmd->callback([&]() {
    Result<void> r = Commands::do_(tasksDir, doId, doForce);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- done ----
  CLI::App* doneCmd = app.add_subcommand("done", "Mark a task as complete");
  std::string doneId;
  doneCmd->add_option("id", doneId, "Task ID (e.g. T001)")->required();
  doneCmd->callback([&]() {
    Result<void> r = Commands::done(tasksDir, doneId);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- pause ----
  CLI::App* pauseCmd = app.add_subcommand("pause",
      "Pause a task (revert to pending)");
  std::string pauseId;
  pauseCmd->add_option("id", pauseId, "Task ID (e.g. T001)")->required();
  pauseCmd->callback([&]() {
    Result<void> r = Commands::pause(tasksDir, pauseId);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- deps ----
  CLI::App* depsCmd = app.add_subcommand("deps",
      "Show dependency information for a task");
  std::string depsId;
  depsCmd->add_option("id", depsId, "Task ID (e.g. T001)")->required();
  depsCmd->callback([&]() {
    Result<void> r = Commands::deps(tasksDir, depsId);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- log ----
  CLI::App* logCmd = app.add_subcommand("log",
      "Append a log entry to a task");
  std::string logId;
  logCmd->add_option("id", logId, "Task ID (e.g. T001)")->required();
  std::string logMsg;
  logCmd->add_option("message", logMsg, "Log message")->required();
  logCmd->callback([&]() {
    Result<void> r = Commands::log(tasksDir, logId, logMsg);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- edit ----
  CLI::App* editCmd = app.add_subcommand("edit",
      "Edit task metadata or project settings");
  std::string editId;
  editCmd->add_option("id", editId, "Task ID (omit for project-level edits)");
  std::string editStatus;
  editCmd->add_option("--status", editStatus, "Set status (pending|in_progress|done)");
  std::vector<std::string> editDepends;
  editCmd->add_option("--depends", editDepends,
      "Set dependency task ID (may be repeated)")
      ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);
  std::string editPhase;
  editCmd->add_option("--phase", editPhase, "Set phase number");
  bool editCritical = false;
  bool editNoCritical = false;
  editCmd->add_flag("--critical", editCritical, "Mark as critical");
  editCmd->add_flag("--no-critical", editNoCritical, "Unmark critical");
  std::string editFiles;
  editCmd->add_option("--files", editFiles,
      "Set files (comma-separated)");
  std::string editSpecs;
  editCmd->add_option("--specs", editSpecs,
      "Set spec references (comma-separated)");
  std::string editPhases;
  editCmd->add_option("--phases", editPhases,
      "Set phase mapping (e.g. '0:Scaffolding,1:Foundation')");
  std::string editCriticalPath;
  editCmd->add_option("--critical-path", editCriticalPath,
      "Set critical path (comma-separated task IDs)");
  editCmd->callback([&]() {
    std::vector<std::string> deps, files, specs;
    auto splitClean = [](const std::string& s) {
      if (s.empty()) return std::vector<std::string>();
      std::vector<std::string> parts = split(s, ',');
      std::vector<std::string> clean;
for (const std::string& p : parts) {
        if (!p.empty()) clean.push_back(p);
      }
      return clean;
    };
    deps = editDepends;
    if (!editFiles.empty()) files = splitClean(editFiles);
    if (!editSpecs.empty()) specs = splitClean(editSpecs);

    bool critSet = editCritical || editNoCritical;
    bool critVal = editCritical;

    Result<void> r = Commands::edit(tasksDir, editId,
        editStatus, deps, editPhase, critSet, critVal,
        files, specs, editPhases, editCriticalPath);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- summary ----
  CLI::App* summaryCmd = app.add_subcommand("summary",
      "Show overall progress statistics");
  summaryCmd->callback([&]() {
    Result<void> r = Commands::summary(tasksDir);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- remove ----
  CLI::App* removeCmd = app.add_subcommand("remove", "Remove a task");
  std::string removeId;
  removeCmd->add_option("id", removeId, "Task ID (e.g. T001)")->required();
  bool removeForce = false;
  removeCmd->add_flag("--force", removeForce,
      "Skip file deletion prompt");
  removeCmd->callback([&]() {
    Result<void> r = Commands::remove(tasksDir, removeId, removeForce);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  // ---- install-skills ----
  CLI::App* installSkillsCmd = app.add_subcommand("install-skills",
      "Install the taskpad AI agent skill file");
  bool installProject = false;
  installSkillsCmd->add_flag("--project", installProject,
      "Install for current project only");
  installSkillsCmd->callback([&]() {
    Result<void> r = Commands::installSkills(installProject);
    if (r.hasError()) {
      std::cerr << "error: " << r.errorMessage() << std::endl;
      exitCode = 1;
    }
  });

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  return exitCode;
}

} // namespace taskpad
