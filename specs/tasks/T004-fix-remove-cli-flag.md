# T004: fix remove cli flag

## Goal

Fix `taskpad remove` CLI flags so `--all` deletes the task file and `--force` skips confirmation prompt.

## Depends On

(None)

## Spec References

- `specs/spec.main.md` → Section 5, `taskpad remove` command

## Files to Create/Modify

- `src/cli.cpp` — add `--all` flag
- `src/commands.h` — update `remove` signature
- `src/commands.cpp` — implement confirmation prompt, `--all` file deletion
- `tests/e2e/remove.bats` — integration tests for the remove command

## Implementation Steps

1. **cli.cpp:** add `--all` boolean flag to remove subcommand
2. **cli.cpp:** pass both `removeAll` and `removeForce` to `Commands::remove`
3. **commands.h:** update `remove` signature: `(tasksDir, taskId, removeAll, force)`
4. **commands.cpp:** implement new flow:

| Command | status.yaml | .md file | Confirm? |
|---------|:-----------:|:--------:|:--------:|
| `remove T004` | remove | keep | yes |
| `remove T004 --all` | remove | delete | yes |
| `remove T004 --force` | remove | keep | no |
| `remove T004 --all --force` | remove | delete | no |

   - Show dependent warning (before confirmation prompt)
   - If `!force`: prompt `"Are you sure... [y/N]"` (message differs based on `--all`)
   - Remove from status.yaml + critical path
   - If `removeAll`: delete the T*.md file (with `fileExists` check)
   - Print success messages

## Constraints

- Follow existing code style in commands.cpp (no exceptions, `Result<T>` returns)
- Keep existing dependent warning logic unchanged

## Acceptance Criteria

- [x] `taskpad remove T004` removes from status.yaml, keeps .md, asks confirmation
- [x] `taskpad remove T004 --all` removes from status.yaml, deletes .md, asks confirmation
- [x] `taskpad remove T004 --force` removes from status.yaml, keeps .md, no prompt
- [x] `taskpad remove T004 --all --force` removes from status.yaml, deletes .md, no prompt
- [x] `taskpad remove T004` (answered N) does nothing
- [x] Dependent warning shown before prompt
- [x] Invalid task ID still returns error
- [x] `taskpad remove T999` → error: Task T999 not found
- [x] E2E tests all pass (via `make e2e-test` or `make check`)

## Notes

**Implementation complete.** All code changes done in:
- `src/commands.h` — signature updated to `(tasksDir, taskId, removeAll, force)`
- `src/cli.cpp` — `--all` flag added, both flags passed to `Commands::remove`
- `src/commands.cpp` — confirmation prompt, `--all` file deletion, new flow
- `tests/helpers.mjs` — E2E test helper (temp project, run/runInteractive, cleanup)
- `tests/e2e/remove.mjs` — 10 E2E tests covering all 4 flag combos, prompts, errors
- `Makefile` — `e2e-test` target updated to use glob pattern
- All 28 unit tests pass (`make test`)
- All 10 E2E tests pass (`make e2e-test`)
- `make check` runs both suites successfully
