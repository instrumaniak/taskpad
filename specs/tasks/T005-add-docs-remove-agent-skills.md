# T005: add docs, remove agent-skills

## Goal

Move the taskpad skill file into a human-facing usage guide and remove the AI agent skill feature entirely (agents will not use this CLI; strict human-in-the-loop workflow).

## Depends On

(None)

## Phase:

0

## Critical:

false

## Spec References

- `specs/spec.main.md` §5 CLI Commands (`taskpad install-skills`)
- `specs/spec.main.md` §8 Implementation Details (Makefile, code structure)
- `specs/spec.main.md` §11 AI Agent Skill System
- `specs/spec.main.md` Appendix C: AI Agent Skill
- `specs/spec.testing.md` E2E table (`install-skills.mjs` row)

## Files to Create/Modify

Create:
- `docs/taskpad.md` (moved + rewritten from `.agents/skills/taskpad/SKILL.md`)

Delete:
- `.agents/skills/taskpad/SKILL.md`
- `.agents/` (directory, now empty)

Modify:
- `src/cli.cpp` (remove `install-skills` subcommand)
- `src/commands.cpp` (remove `Commands::installSkills()`)
- `src/commands.h` (remove declaration)
- `Makefile` (remove skill packaging/flags)
- `specs/spec.main.md` (remove skill sections, update Makefile block & file trees)
- `specs/spec.testing.md` (remove `install-skills.mjs` row)
- `README.md` (link to `docs/taskpad.md`)

## Implementation Steps

1. `git mv .agents/skills/taskpad/SKILL.md docs/taskpad.md`
2. Rewrite doc content for humans: strip YAML frontmatter, drop `/taskpad` slash prefixes (use `taskpad <cmd>`), remove the `install-skills` section (lines 203–209), remove the "Skill files not found" row from the Error Recovery table (line 241), remove all agent/skill language (e.g. "This skill replaces…" at line 248); keep Initial Setup, Command Reference (including `import`), Daily Workflow, Progress Tracking, Error Recovery, Notes
3. Delete the `.agents/` directory
4. `src/cli.cpp`: remove `install-skills` subcommand registration (lines ~221–233)
5. `src/commands.cpp`: remove `Commands::installSkills()` (lines ~1208–1247); confirm `TASKPAD_DATA_DIR` macro has no other uses
6. `src/commands.h`: remove declaration (line 61)
7. `Makefile`: remove `TASKPAD_DATA_DIR` (line 22), the `-DTASKPAD_DATA_DIR` build flag (line 32), `install-skills-data` from `.PHONY` (line 24), `install: install-bin install-skills-data` → `install: install-bin` (line 60), the `install-skills-data` target (lines 66–68), and the `.agents/skills/taskpad` line in `uninstall` (line 73)
8. `specs/spec.main.md`: delete §5 `### taskpad install-skills` (579–600), §11 AI Agent Skill System (999–1071), Appendix C (1147–1193); update the Makefile block in §8 (781–833) — same removals as step 7 plus `install-skills-data` from `.PHONY` (line 783); remove `install-skills.mjs` from code-tree (line 871); update §2 file structure if it references `.agents`; verify §10 Distribution prose doesn't reference skill installation
9. `specs/spec.testing.md`: remove the `install-skills.mjs` row (line 173)
10. `README.md`: add reference to `docs/taskpad.md`
11. Build and verify: `make && make check`

## Constraints

- No behavior changes to any remaining CLI command or task semantics
- No agent/skill references left anywhere in docs, specs, source, Makefile, or README
- Spec files must stay in sync with the actual implementation

## Acceptance Criteria

- [x] `docs/taskpad.md` exists as a human usage guide (setup, command reference including `import`, workflow, error recovery); no YAML frontmatter, no `/taskpad` prefixes, no `install-skills` section, no "Skill files not found" error recovery row, no agent/skill language
- [x] `.agents/` directory removed; no `SKILL.md` remains in repo
- [x] `taskpad install-skills` fully removed: zero references in `src/`, `Makefile` (including `.PHONY`), `specs/`, `README.md`, and tests
- [x] `make` compiles clean and `make check` passes (unit + E2E)
- [x] `specs/spec.main.md` and `specs/spec.testing.md` contain no skill/agent sections, no stale skill-related file-tree entries, Makefile block has no skill targets
- [x] `docs/taskpad.md` and `specs/spec.main.md` are consistent with the actual CLI implementation: all documented flags and command behaviors match `./taskpad --help`, `src/cli.cpp`, and `src/commands.cpp` (verified via empirical testing)

## Notes

- Moved `.agents/skills/taskpad/SKILL.md` → `docs/taskpad.md`; stripped YAML frontmatter and agent/skill language; removed `install-skills` section and "Skill files not found" error row; removed `/taskpad` prefixes. Deleted `.agents/`. Removed `install-skills` from `src/cli.cpp`, `src/commands.cpp`, `src/commands.h`, `Makefile` (incl. `TASKPAD_DATA_DIR`), `specs/spec.main.md`, `specs/spec.testing.md`; added README link. `make && make check` green.
- Follow-up doc/spec consistency pass (verified against `./taskpad --help`, `src/cli.cpp`, `src/commands.cpp`, and spec.main.md):
  - `docs/taskpad.md`: removed bogus `edit --files` / `edit --specs` examples (not in spec, not implemented); added `edit --status` and `edit --no-critical` (implemented and spec'd but previously undocumented).
  - `docs/taskpad.md` + `specs/spec.main.md` `remove`: rewrote to match implementation — `--force` skips the confirmation prompt, `--all` also deletes the task markdown file (previous text said `--force` skips file deletion). Spec now shows exact prompt/output (`Are you sure you want to remove TXXX from status.yaml? [y/N]`).
  - `docs/taskpad.md` + `specs/spec.main.md` `new`/`edit` `--depends`: flag is repeatable (`--depends T001 --depends T002`); comma form (`--depends T001,T002`) fails validation — examples corrected.
  - `specs/spec.testing.md`: `edit.mjs` E2E row now lists `--no-critical`.
  - Renumbered appendices in `specs/spec.main.md` to close gaps left by Appendix C removal: B→A (Example Workflow), D→B (File Naming Conventions), E→C (YAML Schema Validation). Sequence is now A, B, C.
  - `make && make check` re-run: 31 unit tests + 29 E2E tests all pass.
