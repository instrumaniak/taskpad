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

- [ ] `docs/taskpad.md` exists as a human usage guide (setup, command reference including `import`, workflow, error recovery); no YAML frontmatter, no `/taskpad` prefixes, no `install-skills` section, no "Skill files not found" error recovery row, no agent/skill language
- [ ] `.agents/` directory removed; no `SKILL.md` remains in repo
- [ ] `taskpad install-skills` fully removed: zero references in `src/`, `Makefile` (including `.PHONY`), `specs/`, `README.md`, and tests
- [ ] `make` compiles clean and `make check` passes (unit + E2E)
- [ ] `specs/spec.main.md` and `specs/spec.testing.md` contain no skill/agent sections, no stale file-tree entries, Makefile block has no skill targets

## Notes

(filled in during/after implementation)
