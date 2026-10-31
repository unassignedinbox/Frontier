#!/usr/bin/env bash
#====================================================================================================================================
#                                                       SYNCFROMMAIN.SH
#====================================================================================================================================
# 🧩 Fetches the base tree, reports whether HEAD already contains it, and merges it on request.
#
# Why this exists: the owner requires every commit on this branch to be synchronised with the base tree before it is
# reported delivered (see CLAUDE.md, standing instruction 6). Doing that by hand invites two failures — forgetting it,
# and folding unrelated local work into the merge commit. This script refuses a dirty tree and writes the sync-log row
# itself, so the record cannot drift from the merge.
#
# Usage:
#   Automation/SyncFromMain.sh --check   # default. Exit 0 = HEAD contains the base tip, 2 = behind, 1 = error.
#   Automation/SyncFromMain.sh --apply   # fetch, merge, append the sync-log row. Never pushes, never rebases.
#
# Environment:
#   FRONTIER_BASE_URL     default https://github.com/SultanAladin/Frontier.git
#   FRONTIER_BASE_BRANCH  default arena/11f31835-frontier
#   FRONTIER_SYNC_LOG     default Documentation/Records/SyncLog.md

set -uo pipefail

BASE_URL="${FRONTIER_BASE_URL:-https://github.com/SultanAladin/Frontier.git}"
BASE_BRANCH="${FRONTIER_BASE_BRANCH:-arena/11f31835-frontier}"
SYNC_LOG="${FRONTIER_SYNC_LOG:-Documentation/Records/SyncLog.md}"
MODE="--check"

#------------------------------------------------------------------------------------------------------------------------
#                                                       FUNCTIONS
#------------------------------------------------------------------------------------------------------------------------

Usage()
{
    cat <<'USAGE'
Usage:
  Automation/SyncFromMain.sh --check   # default. Exit 0 = HEAD contains the base tip, 2 = behind, 1 = error.
  Automation/SyncFromMain.sh --apply   # fetch, merge, append the sync-log row. Never pushes, never rebases.

Environment:
  FRONTIER_BASE_URL     default https://github.com/SultanAladin/Frontier.git
  FRONTIER_BASE_BRANCH  default arena/11f31835-frontier
  FRONTIER_SYNC_LOG     default Documentation/Records/SyncLog.md
USAGE
}

Fail()
{
    printf 'sync: %s\n' "$1" >&2
    exit 1
}

# 📝 Resolve the repository root from the script location so the script works from any working directory.
RepositoryRoot()
{
    local ScriptDirectory
    ScriptDirectory="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
    cd -- "${ScriptDirectory}/.." && pwd
}

FetchBase()
{
    git fetch --no-tags --quiet "${BASE_URL}" "${BASE_BRANCH}" || Fail "fetch failed: ${BASE_URL} ${BASE_BRANCH}"
    git rev-parse FETCH_HEAD || Fail "cannot resolve FETCH_HEAD"
}

# 📝 A merge is only safe from a tree with no staged or unstaged changes to tracked files. Untracked files are
#    permitted: a merge does not overwrite them unless the base tree carries the same path, and Git refuses that case.
RequireCleanTrackedTree()
{
    if ! git diff-index --quiet HEAD --; then
        git status --short --untracked-files=no
        Fail "tracked changes present; commit or stash them before syncing"
    fi
}

AppendSyncLog()
{
    local ShortCommit="$1" ShortBase="$2" Result="$3" Today
    Today="$(date -u +%Y-%m-%d)"

    mkdir -p -- "$(dirname -- "${SYNC_LOG}")" || Fail "cannot create the directory for ${SYNC_LOG}"

    if [ ! -f "${SYNC_LOG}" ]; then
        printf '# Sync log\n\n## Entries\n' > "${SYNC_LOG}" || Fail "cannot write ${SYNC_LOG}"
    fi

    printf -- '- %s · commit %s · base %s@%s · %s\n' \
        "${Today}" "${ShortCommit}" "${BASE_BRANCH##*/}" "${ShortBase}" "${Result}" >> "${SYNC_LOG}" ||
        Fail "cannot append to ${SYNC_LOG}"

    # 📝 The row is the proof the sync happened. A merge that cannot record itself must not be reported as delivered.
    tail -n 1 -- "${SYNC_LOG}" | grep -q -- "commit ${ShortCommit}" ||
        Fail "sync row missing after append: ${SYNC_LOG}"
}

#------------------------------------------------------------------------------------------------------------------------
#                                                          MAIN
#------------------------------------------------------------------------------------------------------------------------

case "${1:-}" in
    --check) MODE="--check" ;;
    --apply) MODE="--apply" ;;
    -h|--help) Usage; exit 0 ;;
    "") ;;
    *) Usage; Fail "unknown option: $1" ;;
esac

RepositoryRoot > /dev/null
cd -- "$(RepositoryRoot)" || Fail "cannot enter repository root"

git rev-parse --git-dir > /dev/null 2>&1 || Fail "not a git repository"

BASE_TIP="$(FetchBase)"
SHORT_BASE="${BASE_TIP:0:7}"
SHORT_COMMIT="$(git rev-parse --short=7 HEAD 2>/dev/null || printf 'none')"

if git merge-base --is-ancestor "${BASE_TIP}" HEAD 2>/dev/null; then
    printf 'sync: HEAD %s already contains base %s (%s)\n' "${SHORT_COMMIT}" "${SHORT_BASE}" "${BASE_BRANCH}"
    if [ "${MODE}" = "--apply" ]; then
        # 📝 No row when the last row already records this exact commit and tip: repeated checks are not deliveries.
        if [ -f "${SYNC_LOG}" ] && tail -n 1 -- "${SYNC_LOG}" | grep -q -- "commit ${SHORT_COMMIT}.*@${SHORT_BASE}"; then
            printf 'sync: %s already records this commit and tip; no row added\n' "${SYNC_LOG}"
        else
            AppendSyncLog "${SHORT_COMMIT}" "${SHORT_BASE}" "already contained"
            printf 'sync: appended row to %s — commit it with the next delivery\n' "${SYNC_LOG}"
        fi
    fi
    exit 0
fi

printf 'sync: HEAD %s is behind base %s (%s)\n' "${SHORT_COMMIT}" "${SHORT_BASE}" "${BASE_BRANCH}"

if [ "${MODE}" = "--check" ]; then
    exit 2
fi

RequireCleanTrackedTree

# 📝 First sync merges an unrelated history (the base tree was created separately). Retry is deliberate rather than
#    always passing the flag, so ordinary later syncs keep the protection against surprise unrelated merges.
if git merge --no-edit --no-stat FETCH_HEAD > /dev/null 2>&1; then
    RESULT="clean"
elif git merge --allow-unrelated-histories --no-edit --no-stat FETCH_HEAD > /dev/null 2>&1; then
    RESULT="clean (unrelated history)"
else
    CONFLICTS="$(git diff --name-only --diff-filter=U 2>/dev/null | tr '\n' ' ')"
    git merge --abort > /dev/null 2>&1
    Fail "merge produced conflicts and was aborted: ${CONFLICTS:-unknown paths}. Resolve by hand, then archive the tip."
fi

SHORT_COMMIT="$(git rev-parse --short=7 HEAD)"
AppendSyncLog "${SHORT_COMMIT}" "${SHORT_BASE}" "${RESULT}"
printf 'sync: merged base %s, result %s, row appended to %s\n' "${SHORT_BASE}" "${RESULT}" "${SYNC_LOG}"
printf 'sync: the sync-log row is uncommitted — amend or commit it before reporting delivery\n'
exit 0
