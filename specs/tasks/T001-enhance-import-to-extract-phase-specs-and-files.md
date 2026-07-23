# T001: Enhance import to extract phase specs and files

## Goal

Add `## Phase:`, `## Spec References`, and `## Files to Create/Modify` extraction to `taskpad import` so existing markdown task files are fully imported into status.yaml.

## Depends On

(None)

## Spec References

- `spec.md` → "import" command section
- `src/commands.cpp` → existing import implementation, extractStatusLine, extractDepends

## Files to Create/Modify

- `src/commands.cpp` (MODIFY) — add extractPhase, extractBacktickList, wire into import loop

## Implementation Steps

1. Add `extractPhase(content)` — parse `## Phase: N` from task file, return int (default 0)
2. Add `extractBacktickList(content, sectionHeader)` — extract backtick-quoted items from any `## Section Name` block
3. In the import loop, call these to populate `task.phase`, `task.specs`, `task.files`

## Constraints

- Must handle missing sections gracefully (return defaults/empty)
- Phase must be a non-negative integer

## Acceptance Criteria

- [ ] `extractPhase` correctly reads `## Phase: 3` from task content
- [ ] `extractBacktickList` extracts backtick-quoted paths from `## Spec References`
- [ ] `extractBacktickList` extracts backtick-quoted paths from `## Files to Create/Modify`
- [ ] After import, status.yaml contains phase, specs, and files fields for each task
- [ ] All existing tests still pass

## Notes

(filled in during/after implementation)
