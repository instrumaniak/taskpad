# taskpad — Testing Specification

## 1. Overview

Two-tier testing approach:

| Tier | Framework | Target | Speed |
|------|-----------|--------|-------|
| Unit | doctest (C++) | Individual functions (in-process) | ~100ms |
| E2E | node:test (Node.js) | CLI binary as black box | ~2-3s |

**Run all tests:** `make check`

---

## 2. Unit Tests (doctest)

### Framework

doctest (header-only, system package `doctest-dev`):

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
```

### Test Files

| File | Coverage |
|------|----------|
| `test_models.cpp` | Task struct, Status enum, ProjectConfig |
| `test_storage.cpp` | YAML read/write, file I/O |
| `test_utils.cpp` | String helpers (toKebabCase, parseTaskId, formatTaskId, normalizePath, extractPhase, extractCritical, extractSectionListItems) |
| `test_validator.cpp` | Input validation, edge cases |

### Test Examples

```cpp
// test_models.cpp
#include <doctest/doctest.h>
#include "models.h"

TEST_CASE("Task ID formatting") {
    CHECK(taskpad::formatTaskId(1) == "T001");
    CHECK(taskpad::formatTaskId(42) == "T042");
    CHECK(taskpad::formatTaskId(999) == "T999");
}

TEST_CASE("Status string conversion") {
    CHECK(taskpad::statusToString(taskpad::Status::Pending) == "pending");
    CHECK(taskpad::statusToString(taskpad::Status::InProgress) == "in_progress");
    CHECK(taskpad::statusToString(taskpad::Status::Done) == "done");
}

TEST_CASE("Dependency validation") {
    taskpad::Task t;
    t.id = "T003";
    t.depends = {"T001", "T002"};

    std::map<std::string, taskpad::Task> tasks;
    tasks["T001"] = {.id = "T001", .status = taskpad::Status::Done};
    tasks["T002"] = {.id = "T002", .status = taskpad::Status::Pending};

    auto result = taskpad::validateDependencies(t, tasks);
    CHECK(result.hasError());
    CHECK(result.errorMessage() == "Dependency T002 is not done");
}
```

### Requirements

- All public functions must have at least one test
- Edge cases must be tested (empty input, invalid IDs, missing files, boundary values)
- Zero test interdependence — each `TEST_CASE` is self-contained
- No filesystem side effects — unit tests operate on in-memory data structures

---

## 3. E2E Tests (node:test)

### Framework

Node.js 18+ built-in modules:

- `node:test` — test runner (`describe`, `it`, `before`, `after`)
- `node:assert/strict` — assertions (`strictEqual`, `match`, `doesNotMatch`)
- `node:child_process` — spawn binary (`execSync`, `spawnSync`)
- `node:fs` — temporary directories, fixture files
- `node:os` — `tmpdir()`, `platform`

**Zero npm dependencies.** Ships with Node.js.

### Helpers Module: `tests/helpers.mjs`

#### `createProject(tasks, opts)`

Sets up an isolated test project:

1. Builds the `taskpad` binary once (cached across tests within the same invocation)
2. Creates a temporary directory via `fs.mkdtempSync()`
3. Writes `.taskpad` config file
4. Writes `status.yaml` with the given tasks
5. Writes `T*.md` files for each task
6. Returns a `project` object

**Parameters:**

| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `tasks` | `Array<string>` | `[]` | Task IDs to create (e.g. `['T001', 'T002']`) |
| `opts.empty` | `boolean` | `false` | Skip writing any files (for `init` tests) |
| `opts.statusYaml` | `string` | — | Raw YAML string for `status.yaml` (overrides `tasks`) |

**Returns `project` object:**

| Method | Signature | Description |
|--------|-----------|-------------|
| `run` | `(...args: string[]) => Result` | Executes `./taskpad <args>` in project dir |
| `runInteractive` | `(...args: string[], input: string) => Result` | Executes with stdin input (for prompt tests) |
| `exists` | `(path: string) => boolean` | Checks file existence in project dir |
| `readFile` | `(path: string) => string` | Reads file content from project dir |
| `resolve` | `(path: string) => string` | Resolves relative path to absolute in project dir |
| `destroy` | `() => void` | Removes temp directory (called in `after()`) |

**Result object:**

```js
{ stdout: string, stderr: string, status: number | null, signal: NodeJS.Signals | null }
```

#### Example Usage

```js
import { describe, it, before } from 'node:test';
import assert from 'node:assert/strict';
import { createProject } from '../helpers.mjs';

describe('taskpad remove', () => {
  let p;
  before(() => { p = createProject({ tasks: ['T001', 'T002'] }); });
  after(() => p.destroy());

  it('removes task from status.yaml with --force', () => {
    const r = p.run('remove', 'T001', '--force');
    assert.match(r.stdout, /Removed T001/);
    assert.match(r.stdout, /Updated status.yaml/);

    const s = p.run('status');
    assert.doesNotMatch(s.stdout, /T001/);
  });
});
```

### Test Files

One file per command under `tests/e2e/`:

| File | Tests |
|------|-------|
| `import.mjs` | Creates status.yaml from existing T*.md files; --force overwrites; parses phase/critical/status/depends; missing sections default; circular dependency error; "no files" message |
| `init.mjs` | Creates `.taskpad` config; fails if already initialized; custom task-dir |
| `new.mjs` | Creates T*.md + status.yaml entry; auto-increments IDs; --depends/--phase/--critical flags; circular dependency rejection; duplicate name warning; empty name error |
| `remove.mjs` | Removes from status.yaml only (with confirmation); --force skips prompt; --all deletes .md; --all --force both; dependent warning |
| `status.mjs` | Groups by phase; marks next/blocked tasks; shows progress summary; empty project |
| `next.mjs` | Picks correct task by priority (critical → phase → number); shows details from T*.md; "all blocked or complete" message |
| `do.mjs` | Changes status pending → in_progress; reads task file; already_in_progress error; unmet dependencies warning; --force skips check |
| `done.mjs` | Changes status in_progress → done; shows unblocked tasks; already_done error |
| `pause.mjs` | Reverts in_progress → pending; already_pending error |
| `deps.mjs` | Shows depends and dependents; (none) for empty lists; ✓/✗ markers |
| `log.mjs` | Appends timestamped entry to Notes section; creates Notes if missing; empty message error |
| `edit.mjs` | --status/--phase/--critical/--no-critical/--depends task flags; --phases/--critical-path project flags; validation errors |
| `summary.mjs` | Shows totals, percentages, per-phase breakdown, critical path |

### Coverage Requirements

Every CLI command must be tested for:

1. **Happy path** — basic usage produces expected stdout/stderr and exit code 0
2. **Flag combinations** — every documented flag is exercised
3. **Error messages** — every error/warning/info message from §6 Edge Cases table must have a test that triggers it
4. **Prompt workflows** — interactive confirmation with `y`/`N` responses tested via `runInteractive`
5. **Non-zero exit codes** — error conditions exit non-zero; prompts do not
6. **File system effects** — files created, modified, or deleted as specified

### Error Message Coverage Matrix

Every error case from `spec.main.md` §6 Edge Cases must have a corresponding E2E test:

| Case | Test | Command |
|------|------|---------|
| Task ID not found | `run('do', 'T999')` | do, done, pause, deps, log, edit, remove |
| Invalid task ID format | `run('do', 'abc')` | do, done, pause, deps, log, edit, remove |
| Task already in_progress | `run('do', 'T001', '--force'); run('do', 'T001')` | do |
| Task already done | `run('done', 'T001'); run('done', 'T001')` | done |
| Dependencies not met | `run('do', 'T002')` (depends on T001 which is pending) | do |
| Circular dependency | `run('new', 'X', '--depends', 'T001')` where T001 depends on X | new, edit |
| .taskpad missing | `run('status')` in empty dir | all commands except init |
| status.yaml missing | `run('status')` after init only | status, next, do, done, etc. |
| status.yaml malformed | init project with invalid YAML, then `run('status')` | all read commands |
| T*.md missing | init project with status.yaml pointing to nonexistent .md file | next, do |
| No tasks available | `run('next')` with all done or all blocked | next |
| Empty task name | `run('new', '')` | new |
| Duplicate task name | `run('new', 'Test'); run('new', 'Test')` | new |
| Already initialized | init twice | init |
| status.yaml already exists (import) | init + new, then import | import |
| Circular dependency (import) | T*.md with T002→T003→T002 cycle, then `import` | import |
| Dependency not found (import) | T*.md with depends on nonexistent T999, then `import` | import |
| Phase extraction | T*.md with `## Phase: 3`, then check status.yaml for phase=3 | import |
| Critical extraction | T*.md with `## Critical: true`, then check status.yaml for critical=true | import |
| Invalid task-dir path | set task-dir to nonexistent path, then status | all commands |

### Example E2E Test

```js
// tests/e2e/remove.mjs
import { describe, it, before, after } from 'node:test';
import assert from 'node:assert/strict';
import { createProject } from '../helpers.mjs';

describe('taskpad remove', () => {
  let p;
  before(() => { p = createProject({ tasks: ['T001', 'T002'] }); });
  after(() => p.destroy());

  it('removes task from status.yaml with --force', () => {
    const r = p.run('remove', 'T001', '--force');
    assert.match(r.stdout, /Removed T001/);
    assert.match(r.stdout, /Updated status.yaml/);

    const s = p.run('status');
    assert.doesNotMatch(s.stdout, /T001/);
  });

  it('--all --force deletes .md file and status.yaml entry', () => {
    p.run('remove', 'T001', '--all', '--force');
    assert.ok(!p.exists('T001-test.md'));
  });

  it('warns about dependents before removing', () => {
    const r = p.runInteractive('remove', 'T001', 'N\n');
    assert.match(r.stdout, /depends on T001/);
    // Task should still exist
    const s = p.run('status');
    assert.match(s.stdout, /T001/);
  });

  it('returns error for nonexistent task', () => {
    const r = p.run('remove', 'T999');
    assert.match(r.stderr, /T999 not found/);
  });
});
```

---

## 4. Running Tests

```bash
# Unit tests only (doctest, ~100ms)
make test

# E2E tests only (node:test, ~2-3s)
make e2e-test

# Full test suite
make check
```

### Requirements

- Node.js 18+ (for `node:test` — check with `node --version`)
- No `npm install` required — zero Node.js dependencies

---

## 5. Test Data

`tests/data/` contains fixture files used by unit tests:

| File | Purpose |
|------|---------|
| `sample_status.yaml` | Valid status.yaml for storage tests |
| `sample_tasks/` | Example T*.md files for import tests |

E2E tests generate their own fixtures programmatically via `tests/helpers.mjs`.

Doctest unit tests operate on in-memory data only and do not use external fixture files.

---

## 6. CI Integration

CI should run:

```bash
make check
```

No special setup needed beyond:
- C++ compiler with C++17 support
- `libyaml-cpp-dev`, `libcli11-dev`, `doctest-dev` system packages
- Node.js 18+
