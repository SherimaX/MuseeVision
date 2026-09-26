# The workshop

The tools that carry the build process of `docs/BUILD_PLAN.md` (sections 4 to 8): where work happens, how Unreal is
shared, how problems travel up, how work is checked, and how it lands on `renaissance`. Every tool is a Windows
PowerShell 5.1 script with a header saying what it does and how to call it; two checks are in Python
(`C:\Users\anan9\miniconda3\python`) because they read files too large for PowerShell 5.1's JSON reader.

```
 worktrees.ps1 ──makes──▶ slot C:\mv\wt\<slot> on work/<order>          (sparse: MuseeVision, docs, workshop)
      │                         │ builder commits, then:
      │                         ▼
 register.ps1 ◀──records── verify.ps1 ──runs──▶ unreal.ps1 (build, export, automation by slot)
  C:\mv\register            │  rules from renaissance's tip: Docs/wiring.json, Docs/Craft.md, workshop/
  orders gaps inspections   │  checks: wiring_check.py, craft_lint.py
  handoff verify jobs ...   ▼
      ▲                 stamp verify-<sha>.json
      │                         │ inspector passes the order, then:
      └──────records──── merge.ps1 ── squash onto renaissance in C:\mv\wt\integration, verify, fast-forward,
                                      verify again, revert if red
 status.ps1 ── STATUS.md from the Register's records
```

## The tools

| Tool | What it does |
|---|---|
| `unreal.ps1` | Runs Unreal and UnrealBuildTool by **slot** (`slots.json`): GPU (1), HEAVY (1, `Global\MuseeUnreal_HEAVY`, shared with the bootstrap `C:\mv\bin\with-unreal-lock.ps1`), LIGHT (2, admitted above 40 GB of available RAM). `run` blocks; `submit` returns a job id at once and the job survives the calling tool call; `wait`, `status`, `cancel`; `build -Target Editor\|Game\|VR` (game and VR take HEAVY); `export` (the JsonExport module graph). |
| `register.ps1` | Writes the Register, one file per entry, each committed at once under `Global\MuseeRegister`: `init`, `order`, `mark`, `gap`, `answer`, `inspection`, `handoff`, `stamp`, `job`, `defect`, `critique`, `gate`, `session`, `tokens`, `contract-version`. Refuses to write over hand edits. |
| `status.ps1` | Generates `STATUS.md` in the Register from the order files, gaps, inspections, stamps and job records. |
| `worktrees.ps1` | `create`, `reset`, `remove` and `list` builder slots. A slot is created from the renaissance tip (or resumes the order's existing branch) with `core.longpaths` set before checkout. |
| `verify.ps1` | Steps 1-5 for profile `code` (compile, wiring, ownership, craft lint, automation tests), 6 for `physical`, 7-8 for `measured`. Prints one line per step and the first ten errors; stamps `verify-<sha>.json` into the Register; never stamps a dirty tree. `-Background` for anything long. |
| `merge.ps1` | Merges a passed order: squash candidate on top of renaissance (no working tree touched), verify `-Full`, fast-forward only if green, verify renaissance again, revert and return the order if red. One merge at a time. |

## How an order moves

1. The Architect issues it: `register.ps1 order -Id F-FORM-02 -Title ... -Owner Form -Slot f-form -Profile code -Allowed "MuseeVision/Source/Systems/Form/**"`, then `worktrees.ps1 create -Slot f-form -Order F-FORM-02` (or `reset` to reuse a warm slot).
2. The builder works in its slot, compiles with `unreal.ps1 build -Slot C:\mv\wt\f-form -Target Editor`, runs Unreal with `unreal.ps1 run -Slot LIGHT ...`, files gaps with `register.ps1 gap`, keeps `register.ps1 handoff` current, and marks progress with `register.ps1 mark`.
3. Before `built`: `verify.ps1 -Slot C:\mv\wt\f-form -Order F-FORM-02 -Full -Background`, then `unreal.ps1 wait <job>`.
4. The inspector reads the stamp and the diff, and records `register.ps1 inspection -Verdict pass|return`.
5. The Architect runs `merge.ps1 -Order F-FORM-02 -Background`. `status.ps1` shows the state of everything.

## What protects what

- **The checker can't be weakened by the work it checks.** verify reads `wiring.json` and `Craft.md` from the tip of the base branch in the integration worktree, and hands itself over to the copy of `workshop/` committed there (unpacked under `C:\mv\run\checker\<sha>`). The stamp records which rules and which checker ran.
- **Unreal never outlives its worker, and shared services are never killed.** Each run sits in a Windows job object with kill-on-close and breakaway allowed; the worker is started through WMI, outside any caller's job, so zenserver breaks away. The worker waits for Unreal's ShaderCompileWorkers, and ends any still running after `childGraceSeconds`.
- **The owner's checkout is never touched.** Every git call goes through `Invoke-Git`, which refuses that path and refuses `add -A`/`.`, `stash`, `reset --hard`, `clean`, `checkout .`, `switch` and `push`.
- **One Unreal at a time, until measured.** `slots.json` has `oneUnrealAtATime: true`, so GPU and LIGHT runs also hold HEAVY. The Architect sets it to `false` when the Phase 0a spike "two LIGHT jobs at once" passes.

## Contracts the tools expect from Studio

verify's steps 6-8 run Unreal automation tests named `MuseeCheck.` (physical checks, headless, LIGHT),
`MuseeMeasure.` and `MuseeEvidence.` (rendered, GPU). Studio's check runner exposes each check and measurement
there. A required step with no such tests fails and says so.

## Testing the tools

The tools read their places from environment variables so their own tests never touch the real ones:
`MUSEE_INTEGRATION`, `MUSEE_REGISTER`, `MUSEE_RUN`, `MUSEE_SLOTS`, `MUSEE_SLOT_NAMESPACE`, `MUSEE_SLOT_ROOT`.

```
powershell -NoProfile -File workshop\tests\register.tests.ps1 -Scratch C:\mv\run-selftest\register
powershell -NoProfile -File workshop\tests\slots.tests.ps1 -Scratch C:\mv\run-selftest\slots
C:\Users\anan9\miniconda3\python -m unittest discover -s workshop\tests -p "test_*.py"
```
