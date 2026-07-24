# T001: Enhance import with phase/critical extraction and clean up data model

## Goal

Add `## Phase:` and `## Critical:` extraction to `taskpad import`, remove `files`/`specs` from the `Task` struct and `status.yaml`, and update `taskpad next` to parse those from the T*.md file instead. Add unit + E2E tests for the new functionality.

## Depends On

(None)

## Spec References

- `spec.main.md` → §3 Data Model (Task struct), §4 status.yaml schema, §5 import/next commands
- `spec.testing.md` → §2 unit tests, §3 E2E tests
- `src/commands.cpp` → existing import loop, `next` command
- `src/utils.h` → new extract utilities
- `src/storage.cpp` → files/specs read/write removal

## Files to Create/Modify

- `src/utils.h` (MODIFY) — add `extractPhase`, `extractCritical`, `extractSectionListItems`
- `src/utils.cpp` (MODIFY) — implement the three functions (extractCritical case-insensitive)
- `src/models.h` (MODIFY) — remove `files`/`specs` from `Task` struct
- `src/storage.cpp` (MODIFY) — remove `files`/`specs` from read/write; add `## Phase:`/`## Critical:` to template
- `src/commands.h` (MODIFY) — remove `files`/`specs` params from `new_`/`edit`
- `src/commands.cpp` (MODIFY) — wire extract functions into import, update `next` to read from T*.md
- `tests/test_utils.cpp` (MODIFY) — unit tests for new extract functions
- `tests/helpers.mjs` (MODIFY) — add `importTasks` option to `createProject`
- `tests/e2e/import.mjs` (CREATE) — E2E tests for `taskpad import`
- `tests/e2e/next.mjs` (CREATE) — E2E tests for `taskpad next` (gap fill)
- `specs/spec.main.md` (MODIFY) — remove files/specs from data model and schema
- `specs/spec.testing.md` (MODIFY) — add import.mjs to test table
- `specs/tasks/status.yaml` (MODIFY) — normalize and remove files/specs

## Implementation Steps

### Part 1: Add extract utilities to `src/utils.h/cpp`

1. Add `int extractPhase(const std::string& content)` — parse `## Phase: N` from task file content, return int (default 0). Must handle: valid number, non-numeric, negative, missing section.

2. Add `bool extractCritical(const std::string& content)` — parse `## Critical: true/false` from task file content, return bool (default false). Must handle: `true`, `TRUE`, `false`, `FALSE`, missing section, garbage values.

3. Add `std::vector<std::string> extractSectionListItems(const std::string& content, const std::string& sectionHeader)` — extract backtick-quoted items from any `## Section Name` block. Used by `next` command to read files/specs from T*.md at query time.

### Part 2: Clean up data model

4. Remove `files` and `specs` from `Task` struct in `src/models.h`
5. Remove `files`/`specs` read/write from `src/storage.cpp` (both `readStatusFile` and `writeStatusFile`)
6. Remove `files`/`specs` CLI flags from `taskpad new` and `taskpad edit` in `src/commands.cpp` and `src/commands.h`
7. Update `taskpad next` to call `extractSectionListItems` on the T*.md content instead of reading `task.files`/`task.specs`
8. Update `spec.main.md` — remove files/specs from Task struct (§3), status.yaml schema (§4, Appendix E), and import command description (§5)

### Part 3: Wire extract functions into import

9. In the import loop (after dependency parsing), call `extractPhase` and `extractCritical` to populate `task.phase` and `task.critical`
10. Update `taskpad new` template — add `## Phase:` and `## Critical:` sections (optional: if not present, defaults apply)

### Part 4: Tests

11. Add unit tests in `tests/test_utils.cpp` covering:
    - `extractPhase`: valid input, missing, non-numeric, negative, boundary values
    - `extractCritical`: true, false, missing, case-insensitive, garbage
    - `extractSectionListItems`: backtick-quoted items, missing section, empty list, mixed content

12. Add `importTasks` option to `tests/helpers.mjs` `createProject` — writes T*.md files with custom content without creating status.yaml

13. Create `tests/e2e/import.mjs` with tests for:
    - Happy path: `taskpad import` creates status.yaml from T*.md files
    - `--force`: overwrites existing status.yaml
    - Phase parsing: `## Phase: N` → correct phase
    - Critical parsing: `## Critical: true/false` → correct flag
    - Status parsing: `## Status:` line (existing behavior)
    - Depends parsing: `## Depends On` (existing behavior)
    - Sections missing → defaults applied
    - Circular dependency → error
    - status.yaml exists (no --force) → error
    - No T*.md files → "No T*.md files found"

14. Update `spec.testing.md` — add `import.mjs` to the test files table and error coverage matrix

## Constraints

- Must handle missing sections gracefully (return defaults/empty)
- Phase must be a non-negative integer (return 0 for invalid)
- Extract functions must be pure — no filesystem or side effects
- Remove files/specs from Task struct completely — do not leave commented-out fields

## Acceptance Criteria

- [x] `extractPhase("## Phase: 3")` returns 3
- [x] `extractPhase("no phase")` returns 0
- [x] `extractCritical("## Critical: true")` returns true
- [x] `extractCritical("no critical")` returns false
- [x] `extractSectionListItems(content, "## Spec References")` extracts backtick items
- [x] After import, status.yaml contains correct phase and critical values from T*.md
- [x] status.yaml no longer contains files or specs per-task
- [x] `taskpad next` still displays files and specs by reading from T*.md
- [x] All existing unit tests still pass
- [x] All existing E2E tests still pass
- [x] New E2E tests for import pass

## Notes

- All 11 acceptance criteria met
- 31 unit + 29 E2E tests all passing
- extractCritical made truly case-insensitive (tolower comparison), handles True, tRuE, etc.
- tests/e2e/next.mjs created to fill pre-existing gap in next command E2E coverage
- Status display enhancements added during review session: ✓ prefix for done, → prefix for next task
