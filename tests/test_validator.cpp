#include <doctest/doctest.h>
#include "validator.h"
#include "models.h"

using namespace taskpad;

TEST_CASE("isValidTaskId") {
  CHECK(isValidTaskId("T001") == true);
  CHECK(isValidTaskId("T999") == true);
  CHECK(isValidTaskId("T000") == false);
  CHECK(isValidTaskId("T123") == true);

  CHECK(isValidTaskId("") == false);
  CHECK(isValidTaskId("T00") == false);
  CHECK(isValidTaskId("T0001") == false);
  CHECK(isValidTaskId("abc") == false);
  CHECK(isValidTaskId("T01a") == false);
  CHECK(isValidTaskId("t001") == false);
}

TEST_CASE("isValidStatus") {
  CHECK(isValidStatus("pending") == true);
  CHECK(isValidStatus("in_progress") == true);
  CHECK(isValidStatus("done") == true);

  CHECK(isValidStatus("") == false);
  CHECK(isValidStatus("unknown") == false);
  CHECK(isValidStatus("PENDING") == false);
}

TEST_CASE("validateTaskExists") {
  std::map<std::string, Task> tasks;
  tasks["T001"] = Task();
  tasks["T002"] = Task();

  CHECK(validateTaskExists("T001", tasks).hasError() == false);
  CHECK(validateTaskExists("T002", tasks).hasError() == false);
  CHECK(validateTaskExists("T003", tasks).hasError() == true);
  CHECK(validateTaskExists("", tasks).hasError() == true);
}

TEST_CASE("validateDependsExist") {
  std::map<std::string, Task> tasks;
  tasks["T001"] = Task();

  CHECK(validateDependsExist({"T001"}, tasks).hasError() == false);
  CHECK(validateDependsExist({}, tasks).hasError() == false);
  CHECK(validateDependsExist({"T002"}, tasks).hasError() == true);
  CHECK(validateDependsExist({"T001", "T002"}, tasks).hasError() == true);
}

TEST_CASE("validateCircularDependencies - no cycle") {
  std::map<std::string, Task> tasks;
  tasks["T001"] = Task();
  tasks["T001"].depends = {};
  tasks["T002"] = Task();
  tasks["T002"].depends = {"T001"};
  tasks["T003"] = Task();
  tasks["T003"].depends = {"T002"};

  auto r = validateCircularDependencies("T003", {"T002"}, tasks);
  CHECK(r.hasError() == false);
}

TEST_CASE("validateCircularDependencies - self dependency") {
  std::map<std::string, Task> tasks;
  tasks["T001"] = Task();

  auto r = validateCircularDependencies("T001", {"T001"}, tasks);
  CHECK(r.hasError() == true);
  CHECK(r.errorMessage().find("Circular dependency") != std::string::npos);
}

TEST_CASE("validateCircularDependencies - transitive cycle") {
  std::map<std::string, Task> tasks;
  tasks["T001"] = Task();
  tasks["T001"].depends = {"T002", "T003"};
  tasks["T002"] = Task();
  tasks["T002"].depends = {};
  tasks["T003"] = Task();
  tasks["T003"].depends = {"T002"};

  // T001 depends on T002,T003. Can T003 reach T001? No.
  auto r = validateCircularDependencies("T001", {"T002", "T003"}, tasks);
  CHECK(r.hasError() == false);

  // Now add that T003 depends on T001 (in the existing graph)
  tasks["T003"].depends = {"T001"};

  // T001 depends on T002,T003. Can T003 reach T001? T003 -> T001 -> T003. Yes!
  auto r2 = validateCircularDependencies("T001", {"T002", "T003"}, tasks);
  CHECK(r2.hasError() == true);
}
