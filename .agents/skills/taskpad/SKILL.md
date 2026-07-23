---
name: taskpad
description: Manage implementation tasks using the taskpad CLI. Use when the user says /taskpad, wants to check task status, start/pause/complete a task, see what to work on next, create new tasks, check dependencies, or view progress summary. Also use when the user asks about task queues, blocked tasks, or what needs to be done next.
license: MIT
compatibility: Requires taskpad CLI installed in PATH
metadata:
  source: https://github.com/instrumaniak/taskpad
  workflow: task-management
---

# Taskpad Skill

Manage the implementation task queue using the `taskpad` CLI. All task metadata lives in `status.yaml` — never edit task status in Markdown files directly.

## Initial Setup (one-time per project)

Run these in the project root:

```bash
taskpad init                                    # Create .taskpad config
taskpad import                                  # Scan T*.md files → create status.yaml
taskpad edit --phases "0:Scaffolding,1:Foundation,2:Pure Logic,3:Integration,4:Rendering,5:Screens,6:Polish"
taskpad edit --critical-path "T001,T003,T010,T011,T012,T016,T024"
```

- `taskpad init` creates `.taskpad` if it doesn't exist
- `taskpad import` scans `specs/tasks/` for T*.md files and builds `status.yaml`
- If `status.yaml` already exists, use `taskpad import --force` to overwrite
- If `taskpad edit --phases` or `--critical-path` is omitted, all tasks default to phase 0 and no critical path is set

## Command Reference

All commands use the `/taskpad` prefix:

**Global flag:** `--tasks-dir <path>` overrides the task directory from `.taskpad` config. Resolution order: (1) `--tasks-dir` flag, (2) `.taskpad` config's `task-dir` key, (3) fallback `specs/tasks/`.

### `/taskpad status`
Show all tasks grouped by phase with status.

```
> taskpad status
Phase 0: Scaffolding
  T001  Project Setup         [done]
  T002  Asset Acquisition     [in_progress]

Phase 1: Foundation Types
  T003  Core Types            [pending]  ← next (dependencies met)
  T004  ResourceManager       [pending]  ← blocked by T002
  ...

Progress: 1/30 done, 1 in_progress, 28 pending
```

The `← next` marker shows the recommended next task. `← blocked by TXXX` shows what's blocking it.

### `/taskpad next`
Show the single next task to work on.

```
> taskpad next
Next: T003 — Core Types
  Depends on: T001 [done] ✓
  Goal: Core types, enums, components, entities
  First step: Create src/core/types.h with game:: namespace
  Files: src/core/types.h, src/core/types.cpp
  Specs: raylib-cpp/01-architecture.md, game/01-core-concepts.md
```

Prioritization order: critical path → phase number → task number.
If output says "All tasks blocked or complete", no tasks are available.

### `/taskpad do TXXX`
Start working on a task.

```
> taskpad do T003
Started T003 — Core Types
Status changed: pending → in_progress
Now reading T003-core-types.md...
```

- Reads the task file and displays goal + first implementation step
- If dependencies are not met, blocks with an error listing the unmet dependencies. Use `taskpad do T003 --force` to override
- If already in_progress, reports "Already in_progress"
- If already done, reports "Already done"

### `/taskpad done TXXX`
Mark a task as complete.

```
> taskpad done T003
✓ T003 marked as done
Unblocked tasks:
  T009  Board Generation      [pending]
  T010  Item System           [pending]
```

Shows newly unblocked tasks whose dependencies are now all satisfied.

### `/taskpad pause TXXX`
Pause a task (revert to pending).

```
> taskpad pause T003
Paused T003 — Core Types
Status changed: in_progress → pending
```

### `/taskpad deps TXXX`
Show dependency information.

```
> taskpad deps T012
T012 depends on:
  T003  Core Types            [done] ✓
  T010  Item System           [done] ✓
  T011  Combat System         [in_progress] ✗
Tasks waiting on T012:
  T016  Turn Flow             [pending]
```

✓ = dependency satisfied, ✗ = not satisfied.

### `/taskpad new "Task Name"`
Create a new task.

```
> taskpad new "Project Setup"
Created T001-project-setup.md
Updated status.yaml
```

Optional flags: `--depends T001,T002`, `--phase 1`, `--critical`.
The task file is created from template with no Status line (status is tracked in `status.yaml`).

### `/taskpad summary`
Show overall progress statistics.

```
> taskpad summary
Task Summary
─────────────
Total tasks:    30
Done:            5 (16.7%)
In progress:     1 (3.3%)
Pending:        24 (80.0%)
By Phase:
  Phase 0: 2/2 done
  Phase 1: 1/6 done
  ...
Critical Path: T001 → T003 → T010 → T011 → T012 → T016 → T024
  Status: 3/7 done, 1 in_progress, 3 pending
```

### `/taskpad log TXXX "message"`
Append a timestamped note to the task file.

```
> taskpad log T003 "Fixed namespace issue in types.h"
Logged to T003-core-types.md
```

Format: `- [YYYY-MM-DD HH:MM] message`. Creates `## Notes` section if it doesn't exist.

### `/taskpad edit TXXX --<field> <value>`
Edit task or project metadata.

```
> taskpad edit T003 --phase 2
> taskpad edit T003 --depends T001,T002
> taskpad edit T003 --files "src/foo.cpp,include/foo.h"
> taskpad edit T003 --specs "specs/ARCH.md,specs/API.md"
> taskpad edit --phases "0:Scaffolding,1:Foundation"
> taskpad edit --critical-path "T001,T003,T010"
```

Task flags: `--status`, `--phase`, `--critical`, `--depends`, `--files`, `--specs`.
Project flags (no task ID): `--phases`, `--critical-path`.

### `/taskpad remove TXXX`
Remove a task.

```
> taskpad remove T030
Removed T030 — Final Polish
Updated status.yaml
```

Without `--force`, prints a hint about also removing the .md file. Use `--force` to suppress the hint (does not delete the file).

### `/taskpad import`
Import existing T*.md files into taskpad.

```
> taskpad import
Scanning specs/tasks/ for T*.md files...
Found 30 task files
Created status.yaml with 30 tasks
```

Use `--force` to overwrite an existing `status.yaml`.

### `/taskpad install-skills`
Install this skill file for AI agent discovery.

```
> taskpad install-skills            # → ~/.agents/skills/taskpad/
> taskpad install-skills --project  # → .agents/skills/taskpad/
```

## Daily Workflow

1. **Start**: Run `taskpad next` to find the next unblocked task
2. **Read**: Run `taskpad do TXXX` to start it — reads the task goal and first step
3. **Implement**: Read the full T*.md file, load referenced specs, write code
4. **Build/Test**: Run `make test` or build commands
5. **Log**: Run `taskpad log TXXX "what was done"` to record progress
6. **Complete**: Run `taskpad done TXXX` to mark done
7. **Repeat**: Check `taskpad next` for the next task
8. **Check progress**: Run `taskpad status` or `taskpad summary` periodically

## Progress Tracking

- `taskpad status` — full task table by phase
- `taskpad summary` — stats and critical path status
- `taskpad deps TXXX` — dependency graph for a specific task

## Error Recovery

| Situation | Action |
|-----------|--------|
| `Not initialized` | Run `taskpad init` first |
| `status.yaml not found` | Run `taskpad import` or `taskpad new` |
| `Task not found` | Check task ID with `taskpad status` |
| `Already in_progress` / `Already done` | Use `taskpad pause` to revert, or proceed |
| `Unmet dependencies` | Use `taskpad do TXXX --force` to override |
| `Circular dependency` | Fix depends via `taskpad edit TXXX --depends TYYY,TZZZ` |
| `status.yaml already exists` | Use `taskpad import --force` |
| `Invalid task ID` | Use format TXXX (T001, T002, etc.) |
| `tasks-dir not found` | Check `.taskpad` config or use `--tasks-dir <path>` |
| `Skill files not found` | Ensure SKILL.md exists at `.agents/skills/taskpad/` or the install path |

## Notes

- Task status is stored ONLY in `status.yaml`, never in the Markdown files
- The `status.yaml` file is the single source of truth — do not edit it manually unless necessary
- Task files (T*.md) contain goal, steps, acceptance criteria — edit these freely
- This skill replaces any previous file-based task management approach
- The `taskpad` CLI must be installed and available in PATH
