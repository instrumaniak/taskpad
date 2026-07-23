#include <doctest/doctest.h>
#include "utils.h"

using namespace taskpad;

TEST_CASE("toKebabCase") {
  CHECK(toKebabCase("Project Setup") == "project-setup");
  CHECK(toKebabCase("CoreTypes") == "core-types");
  CHECK(toKebabCase("simple") == "simple");
  CHECK(toKebabCase("") == "");
  CHECK(toKebabCase("  spaces  ") == "spaces");
  CHECK(toKebabCase("Already-Kebab") == "already-kebab");
  CHECK(toKebabCase("snake_case") == "snake-case");
}

TEST_CASE("parseTaskId") {
  CHECK(parseTaskId("T001") == 1);
  CHECK(parseTaskId("T042") == 42);
  CHECK(parseTaskId("T999") == 999);
  CHECK(parseTaskId("T000") == 0);
}

TEST_CASE("formatTaskId") {
  CHECK(formatTaskId(1) == "T001");
  CHECK(formatTaskId(42) == "T042");
  CHECK(formatTaskId(999) == "T999");
  CHECK(formatTaskId(0) == "T000");
  CHECK(formatTaskId(1000) == "T999");
  CHECK(formatTaskId(-1) == "T000");
}

TEST_CASE("currentTimestamp format") {
  std::string ts = currentTimestamp();
  // Format: YYYY-MM-DD HH:MM
  CHECK(ts.size() == 16);
  CHECK(ts[4] == '-');
  CHECK(ts[7] == '-');
  CHECK(ts[10] == ' ');
  CHECK(ts[13] == ':');
}

TEST_CASE("split") {
  std::vector<std::string> parts = split("a,b,c", ',');
  CHECK(parts.size() == 3);
  CHECK(parts[0] == "a");
  CHECK(parts[1] == "b");
  CHECK(parts[2] == "c");

  parts = split("single", ',');
  CHECK(parts.size() == 1);
  CHECK(parts[0] == "single");

  parts = split("", ',');
  CHECK(parts.size() == 1);
  CHECK(parts[0] == "");
}

TEST_CASE("trim") {
  CHECK(trim("  hello  ") == "hello");
  CHECK(trim("hello") == "hello");
  CHECK(trim("  ") == "");
  CHECK(trim("") == "");
  CHECK(trim("a b") == "a b");
}

TEST_CASE("normalizePath") {
  CHECK(normalizePath("specs/tasks/") == "specs/tasks");
  CHECK(normalizePath("specs/tasks") == "specs/tasks");
  CHECK(normalizePath("/") == "/");
}
