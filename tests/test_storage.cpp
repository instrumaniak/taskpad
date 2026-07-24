#include <doctest/doctest.h>
#include "storage.h"
#include "utils.h"
#include "models.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>

using namespace taskpad;

static std::string testDir() {
  return "tests/data";
}

static void createDir(const std::string& path) {
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
}

static void removeDir(const std::string& path) {
  std::error_code ec;
  std::filesystem::remove_all(path, ec);
}

TEST_CASE("createConfig and check existence") {
  std::string tmpDir = testDir() + "/test_config";
  createDir(tmpDir);

  // Should not exist yet
  CHECK(configExists(tmpDir) == false);

  // Create config
  Result<void> r = createConfig(tmpDir, "my-tasks");
  CHECK(r.hasError() == false);

  // Should exist now
  CHECK(configExists(tmpDir) == true);

  // Re-creating should fail
  Result<void> r2 = createConfig(tmpDir, "other");
  CHECK(r2.hasError() == true);

  // Read back
  Result<std::string> dirResult = readTaskDir(tmpDir);
  CHECK(dirResult.hasError() == false);
  CHECK(dirResult.value == "my-tasks");

  removeDir(tmpDir);
}

TEST_CASE("readStatusFile - file not found") {
  std::string tmpDir = testDir() + "/test_nonexistent";
  createDir(tmpDir);

  Result<StatusFile> r = readStatusFile(tmpDir);
  CHECK(r.hasError() == true);
  CHECK(r.errorMessage().find("No status.yaml found") != std::string::npos);

  removeDir(tmpDir);
}

TEST_CASE("readStatusFile and writeStatusFile roundtrip") {
  std::string tmpDir = testDir() + "/test_roundtrip";
  createDir(tmpDir);

  StatusFile sf;
  Task t1;
  t1.id = "T001";
  t1.name = "First Task";
  t1.status = Status::Pending;
  t1.depends = {};
  t1.phase = 0;
  t1.critical = false;

  Task t2;
  t2.id = "T002";
  t2.name = "Second Task";
  t2.status = Status::Done;
  t2.depends = {"T001"};
  t2.phase = 1;
  t2.critical = true;

  sf.tasks["T001"] = t1;
  sf.tasks["T002"] = t2;
  sf.config.phases[0] = "Setup";
  sf.config.phases[1] = "Core";
  sf.config.criticalPath = {"T002"};

  // Write
  Result<void> writeResult = writeStatusFile(tmpDir, sf);
  CHECK(writeResult.hasError() == false);

  // Read back
  Result<StatusFile> readResult = readStatusFile(tmpDir);
  CHECK(readResult.hasError() == false);

  StatusFile loaded = readResult.value;

  // Verify tasks
  CHECK(loaded.tasks.size() == 2);
  CHECK(loaded.tasks["T001"].name == "First Task");
  CHECK(loaded.tasks["T001"].status == Status::Pending);
  CHECK(loaded.tasks["T001"].depends.empty());
  CHECK(loaded.tasks["T001"].phase == 0);
  CHECK(loaded.tasks["T001"].critical == false);

  CHECK(loaded.tasks["T002"].name == "Second Task");
  CHECK(loaded.tasks["T002"].status == Status::Done);
  CHECK(loaded.tasks["T002"].depends.size() == 1);
  CHECK(loaded.tasks["T002"].depends[0] == "T001");
  CHECK(loaded.tasks["T002"].phase == 1);
  CHECK(loaded.tasks["T002"].critical == true);

  // Verify config
  CHECK(loaded.config.phases.size() == 2);
  CHECK(loaded.config.phases[0] == "Setup");
  CHECK(loaded.config.phases[1] == "Core");
  CHECK(loaded.config.criticalPath.size() == 1);
  CHECK(loaded.config.criticalPath[0] == "T002");

  removeDir(tmpDir);
}

TEST_CASE("taskFilePath") {
  CHECK(taskFilePath("specs/tasks", "T001", "My Task") ==
        "specs/tasks/T001-my-task.md");
  CHECK(taskFilePath("specs/tasks", "T042", "Hello World") ==
        "specs/tasks/T042-hello-world.md");
}

TEST_CASE("writeTaskFile and readTaskFile") {
  std::string tmpDir = testDir() + "/test_taskfile";
  createDir(tmpDir);

  std::string path = taskFilePath(tmpDir, "T001", "Test Task");
  Result<void> writeResult = writeTaskFile(path, "T001", "Test Task");
  CHECK(writeResult.hasError() == false);

  Result<std::string> readResult = readTaskFile(path);
  CHECK(readResult.hasError() == false);
  CHECK(readResult.value.find("# T001: Test Task") != std::string::npos);
  CHECK(readResult.value.find("## Goal") != std::string::npos);

  removeDir(tmpDir);
}

TEST_CASE("appendLog creates Notes section") {
  std::string tmpDir = testDir() + "/test_log";
  createDir(tmpDir);

  std::string path = taskFilePath(tmpDir, "T001", "Log Test");
  Result<void> writeResult = writeTaskFile(path, "T001", "Log Test");
  CHECK(writeResult.hasError() == false);

  Result<void> logResult = appendLog(path, "Test message");
  CHECK(logResult.hasError() == false);

  Result<std::string> content = readTaskFile(path);
  CHECK(content.hasError() == false);
  CHECK(content.value.find("## Notes") != std::string::npos);
  CHECK(content.value.find("Test message") != std::string::npos);
  CHECK(content.value.find("2026") != std::string::npos);

  removeDir(tmpDir);
}
