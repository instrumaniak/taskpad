#include <doctest/doctest.h>
#include "models.h"

using namespace taskpad;

TEST_CASE("Status to string conversion") {
  CHECK(statusToString(Status::Pending) == "pending");
  CHECK(statusToString(Status::InProgress) == "in_progress");
  CHECK(statusToString(Status::Done) == "done");
}

TEST_CASE("String to status conversion") {
  CHECK(stringToStatus("pending") == Status::Pending);
  CHECK(stringToStatus("in_progress") == Status::InProgress);
  CHECK(stringToStatus("done") == Status::Done);
  CHECK(stringToStatus("unknown") == Status::Pending);
  CHECK(stringToStatus("") == Status::Pending);
}

TEST_CASE("Task struct default values") {
  Task t;
  CHECK(t.id.empty());
  CHECK(t.name.empty());
  CHECK(t.status == Status::Pending);
  CHECK(t.depends.empty());
  CHECK(t.phase == 0);
  CHECK(t.critical == false);
  CHECK(t.files.empty());
  CHECK(t.specs.empty());
}

TEST_CASE("Task struct with values") {
  Task t;
  t.id = "T001";
  t.name = "Test Task";
  t.status = Status::InProgress;
  t.depends = {"T002"};
  t.phase = 2;
  t.critical = true;
  t.files = {"src/test.cpp"};
  t.specs = {"spec.md"};

  CHECK(t.id == "T001");
  CHECK(t.name == "Test Task");
  CHECK(t.status == Status::InProgress);
  CHECK(t.depends.size() == 1);
  CHECK(t.depends[0] == "T002");
  CHECK(t.phase == 2);
  CHECK(t.critical == true);
  CHECK(t.files.size() == 1);
  CHECK(t.files[0] == "src/test.cpp");
  CHECK(t.specs.size() == 1);
  CHECK(t.specs[0] == "spec.md");
}

TEST_CASE("Result success") {
  auto r = Result<int>::success(42);
  CHECK(!r.hasError());
  CHECK(r.value == 42);
}

TEST_CASE("Result failure") {
  auto r = Result<int>::failure("error message");
  CHECK(r.hasError());
  CHECK(r.errorMessage() == "error message");
}

TEST_CASE("Result<void> success") {
  auto r = Result<void>::success();
  CHECK(!r.hasError());
}

TEST_CASE("Result<void> failure") {
  auto r = Result<void>::failure("void error");
  CHECK(r.hasError());
  CHECK(r.errorMessage() == "void error");
}
