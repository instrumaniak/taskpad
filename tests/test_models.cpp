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
}

TEST_CASE("Task struct with values") {
  Task t;
  t.id = "T001";
  t.name = "Test Task";
  t.status = Status::InProgress;
  t.depends = {"T002"};
  t.phase = 2;
  t.critical = true;

  CHECK(t.id == "T001");
  CHECK(t.name == "Test Task");
  CHECK(t.status == Status::InProgress);
  CHECK(t.depends.size() == 1);
  CHECK(t.depends[0] == "T002");
  CHECK(t.phase == 2);
  CHECK(t.critical == true);
}

TEST_CASE("Result success") {
  Result<int> r = Result<int>::success(42);
  CHECK(!r.hasError());
  CHECK(r.value == 42);
}

TEST_CASE("Result failure") {
  Result<int> r = Result<int>::failure("error message");
  CHECK(r.hasError());
  CHECK(r.errorMessage() == "error message");
}

TEST_CASE("Result<void> success") {
  Result<void> r = Result<void>::success();
  CHECK(!r.hasError());
}

TEST_CASE("Result<void> failure") {
  Result<void> r = Result<void>::failure("void error");
  CHECK(r.hasError());
  CHECK(r.errorMessage() == "void error");
}
