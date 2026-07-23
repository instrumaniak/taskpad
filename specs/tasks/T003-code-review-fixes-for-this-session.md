# T003: Code review fixes for this session

## Goal

Fix all issues identified in the code review: 

(1) `done` command lists always-unblocked tasks as "Unblocked" due to a spurious second loop, 

(2) shell injection via `system()`/`popen()` for mkdir and ls, 

(3) 4096B buffer can truncate ls output in import, 

(4) task directory resolution duplicated 13 times across commands, 

(5) circular dependency check in `new_` passes `sf.tasks` without the new task entry, 

(6) `do_` warns "Use --force" but proceeds anyway, 

(7) unused writeFile in test_storage.cpp, 

(8) missing "Scanning..." output line in import, 

(9) `auto` keyword used in `src/` and `tests/` contrary to project's explicit-types convention.

## Depends On

(None)

## Spec References

- specs/spec.main.md (sections 5, 6, 7)

## Files to Create/Modify

- src/commands.cpp
- src/commands.h
- src/storage.cpp
- src/storage.h
- src/validator.cpp
- src/utils.cpp
- src/models.cpp
- src/main.cpp
- src/cli.cpp
- src/cli.h
- tests/test_storage.cpp
- tests/test_validator.cpp
- tests/test_utils.cpp
- tests/test_main.cpp
- tests/test_models.cpp

## Implementation Steps

1. Fix `done` command: remove the second loop (lines 772-781) that adds all pending tasks with met deps regardless of whether they were blocked by the completed task
2. Replace `system("mkdir -p ...")` with `std::filesystem::create_directories()` in `copyFile()` and `init()`
3. Replace `popen("ls ...")` with `std::filesystem::directory_iterator` in `import_()`
4. Fix `do_`: return `Result<void>::failure` when unmet deps exist and `!force`, instead of just warning and proceeding
5. Add a `resolveTaskDir()` helper function and use it to eliminate the 13 duplicated directory resolution blocks
6. Fix cycle check in `new_`: temporarily insert the new task into `sf.tasks` before calling `validateCircularDependencies`
7. Add `std::cout << "Scanning " << dir << "/ for T*.md files..." << std::endl;` at the start of `import_()`
8. Remove unused `writeFile()` function in `tests/test_storage.cpp`
9. Replace `system("mkdir ...")` and `system("rm -rf ...")` in `tests/test_storage.cpp` `createDir()`/`removeDir()` helpers with `std::filesystem::create_directory()` and `std::filesystem::remove_all()`
10. Replace every `auto` keyword in `src/` and `tests/` with explicit types — iterators (`std::map<std::string, Task>::iterator`), `Result<T>` returns, range-for variables, `dynamic_cast` results (`auto*` → `ClassName*`), lambda parameters. Reference style: `src/storage.cpp` (zero `auto` usage, all types explicit)
11. Build and run all tests to verify no regressions

## Constraints

- Must compile with `g++ -std=c++17` (the project standard)
- Must use `std::filesystem` (C++17) for all filesystem operations
- All 28 existing tests must pass with no regressions
- No new warnings from compiler
- Must follow existing code style (2-space indent, K&R braces, Result<T> pattern)

## Acceptance Criteria

- [ ] `done TXXX` only shows tasks that were actually blocked by TXXX as "Unblocked"
- [ ] No shell commands (`system()`, `popen()`) used for mkdir or filesystem listing
- [ ] `import` uses directory iteration directly instead of `ls` pipe
- [ ] `do TXXX` blocks and returns error when deps unmet without `--force`
- [ ] All 13 task dir resolution blocks consolidated into single helper
- [ ] `new_` detects cycles involving the new task itself
- [ ] `import` prints "Scanning..." line matching spec
- [ ] No unused functions in test files
- [ ] All 28 tests pass, no compiler warnings
- [ ] Zero `auto` keywords remain in `src/` or `tests/` (explicit types everywhere)

## Notes

- Place `resolveTaskDir()` in `utils.h`/`utils.cpp` as a shared utility (used across all commands)

- [2026-07-23 17:12] Filled in task file with all 7 code review fix items + acceptance criteria
