# T002: Prepare cardgame repo for taskpad management

## Goal

Set up the cardgame-raylib repo so its 30 existing task files can be managed by taskpad CLI with correct phases, dependencies, critical path, and critical flags.

## Depends On

- T001

## Spec References

- `/mnt/mint/home/raziur/GameLab/forward-card-game-study/cardgame-raylib/specs/tasks/README.md` — phase table, critical path, and parallelization groups

## Files to Create/Modify

- `/mnt/mint/home/raziur/GameLab/forward-card-game-study/cardgame-raylib/specs/tasks/T*.md` (MODIFY — add `## Phase: N` and `## Critical: true/false` to all 30)
- `/mnt/mint/home/raziur/GameLab/forward-card-game-study/cardgame-raylib/.taskpad` (CREATE)

## Implementation Steps

1. Add `## Phase: N` AND `## Critical: true/false` to each of the 30 T*.md files in cardgame's `specs/tasks/` using the README phase table and critical path:
   - Critical path tasks: T001, T003, T010, T011, T012, T016, T024 → `## Critical: true`
   - All other tasks → `## Critical: false` (can be omitted since false is default)
   - Phase values from the README phase table:
     - Phase 0: T001–T002
     - Phase 1: T003–T008
     - Phase 2: T009–T015
     - Phase 3: T016
     - Phase 4: T017–T022
     - Phase 5: T023–T025
     - Phase 6: T026–T030
2. Create `.taskpad` config with `task-dir: specs/tasks`
3. Run `taskpad init` then `taskpad import --force` in cardgame repo
4. Run `taskpad edit --phases "0:Scaffolding,1:Foundation Types,2:Pure Logic,3:Integration,4:Rendering,5:Screens,6:Polish"`
5. Run `taskpad edit --critical-path "T001,T003,T010,T011,T012,T016,T024"`
6. Run `taskpad status` to verify 30 tasks in 7 phases
7. Run `taskpad summary` to verify critical path with 7 tasks

## Phase Mapping

| Phase | Tasks |
|-------|-------|
| 0 | T001–T002 |
| 1 | T003–T008 |
| 2 | T009–T015 |
| 3 | T016 |
| 4 | T017–T022 |
| 5 | T023–T025 |
| 6 | T026–T030 |

## Acceptance Criteria

- [ ] All 30 task files have `## Phase: N` header
- [ ] All 7 critical path task files have `## Critical: true`
- [ ] `.taskpad` exists in cardgame repo root
- [ ] `taskpad import` succeeds with no errors
- [ ] `taskpad status` shows 30 tasks across 7 phases
- [ ] `taskpad summary` shows critical path with 7 tasks

## Notes

(filled in during/after implementation)
