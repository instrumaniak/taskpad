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

TEST_CASE("extractPhase") {
  CHECK(extractPhase("## Phase: 3") == 3);
  CHECK(extractPhase("## Phase: 0") == 0);
  CHECK(extractPhase("## Phase: 42") == 42);
  CHECK(extractPhase("no phase header here") == 0);
  CHECK(extractPhase("") == 0);
  CHECK(extractPhase("## Phase: -1") == 0);
  CHECK(extractPhase("## Phase: abc") == 0);
  CHECK(extractPhase("## Phase: 7\n## Goal: stuff") == 7);
  CHECK(extractPhase("## Phase:") == 0);
}

TEST_CASE("extractCritical") {
  CHECK(extractCritical("## Critical: true") == true);
  CHECK(extractCritical("## Critical: TRUE") == true);
  CHECK(extractCritical("## Critical: True") == true);
  CHECK(extractCritical("## Critical: tRuE") == true);
  CHECK(extractCritical("## Critical: false") == false);
  CHECK(extractCritical("## Critical: FALSE") == false);
  CHECK(extractCritical("no critical header") == false);
  CHECK(extractCritical("") == false);
  CHECK(extractCritical("## Critical: garbage") == false);
  CHECK(extractCritical("## Critical: maybe") == false);
  CHECK(extractCritical("## Critical: true\n## Goal: stuff") == true);
}

TEST_CASE("extractSectionListItems") {
  std::string content =
    "# T001: Test\n"
    "## Spec References\n"
    "- `spec1.md`\n"
    "- `path/to/spec2.md`\n"
    "## Files to Create/Modify\n"
    "- `src/file1.cpp`\n"
    "- `include/file2.h`\n";

  // Extract from Spec References
  std::vector<std::string> specs = extractSectionListItems(
      content, "## Spec References");
  CHECK(specs.size() == 2);
  CHECK(specs[0] == "spec1.md");
  CHECK(specs[1] == "path/to/spec2.md");

  // Extract from Files to Create/Modify
  std::vector<std::string> files = extractSectionListItems(
      content, "## Files to Create/Modify");
  CHECK(files.size() == 2);
  CHECK(files[0] == "src/file1.cpp");
  CHECK(files[1] == "include/file2.h");

  // Missing section
  std::vector<std::string> missing = extractSectionListItems(
      content, "## Nonexistent Section");
  CHECK(missing.empty());

  // Empty content
  std::vector<std::string> empty = extractSectionListItems(
      "", "## Spec References");
  CHECK(empty.empty());

  // No backtick items
  std::vector<std::string> noTicks = extractSectionListItems(
      "## Spec References\n- plain text\n- more text\n",
      "## Spec References");
  CHECK(noTicks.empty());

  // Mixed content
  std::string mixed =
    "## Files\n"
    "- `main.cpp` (MODIFY)\n"
    "- Makefile\n"
    "- `utils.h`\n";
  std::vector<std::string> mixedResult = extractSectionListItems(
      mixed, "## Files");
  CHECK(mixedResult.size() == 2);
  CHECK(mixedResult[0] == "main.cpp");
  CHECK(mixedResult[1] == "utils.h");
}
