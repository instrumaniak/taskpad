#pragma once

#include <string>
#include <vector>
#include <map>

namespace taskpad {

enum class Status {
  Pending,
  InProgress,
  Done
};

struct Task {
  std::string id;
  std::string name;
  Status status = Status::Pending;
  std::vector<std::string> depends;
  int phase = 0;
  bool critical = false;
  std::vector<std::string> files;
  std::vector<std::string> specs;
};

struct ProjectConfig {
  std::map<int, std::string> phases;
  std::vector<std::string> criticalPath;
};

struct StatusFile {
  std::map<std::string, Task> tasks;
  ProjectConfig config;
};

std::string statusToString(Status s);
Status stringToStatus(const std::string& s);

template<typename T>
struct Result {
  T value;
  std::string error;

  bool hasError() const { return !error.empty(); }
  const std::string& errorMessage() const { return error; }

  static Result success(T val) { return {val, ""}; }
  static Result failure(const std::string& err) { return {T{}, err}; }
};

template<>
struct Result<void> {
  std::string error;

  bool hasError() const { return !error.empty(); }
  const std::string& errorMessage() const { return error; }

  static Result<void> success() { return {""}; }
  static Result<void> failure(const std::string& err) { return {err}; }
};

} // namespace taskpad
