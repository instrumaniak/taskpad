# T002: Prepare cardgame repo for taskpad management

## Goal

Set up the cardgame-raylib repo so its 30 existing task files can be managed by taskpad CLI with correct phases, dependencies, and critical path.

## Depends On

- T001

## Spec References

- `/mnt/mint/home/raziur/GameLab/forward-card-game-study/cardgame-raylib/specs/tasks/README.md` — phase table and critical path

## Files to Create/Modify

- `/mnt/mint/home/raziur/GameLab/forward-card-game-study/cardgame-raylib/specs/tasks/T*.md` (MODIFY — add `## Phase: N` to all 30)
- `/mnt/mint/home/raziur/GameLab/forward-card-game-study/cardgame-raylib/.taskpad` (CREATE)

## Implementation Steps

1. Add `## Phase: N` to each of the 30 T*.md files in cardgame's specs/tasks/ using the README phase table
2. Create `.taskpad` config with `task-dir: specs/tasks`
3. Run `taskpad init` then `taskpad import --force` in cardgame repo
4. Run `taskpad edit --critical-path "T001,T003,T010,T011,T012,T016,T024"`
5. Run `taskpad status` to verify 30 tasks in 7 phases

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
- [ ] `.taskpad` exists in cardgame repo root
- [ ] `taskpad import` succeeds with no errors
- [ ] `taskpad status` shows 30 tasks across 7 phases
- [ ] `taskpad summary` shows critical path with 7 tasks

## Notes

(filled in during/after implementation)
