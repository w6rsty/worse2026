#!/usr/bin/env bash
#
# Make the container-ci `gate` status check REQUIRED to merge into master
# (Phase 11, P11-branch_protection / R50).
#
# PREREQUISITE: GitHub branch protection (and the newer rulesets) is NOT available for a
# PRIVATE repo on the Free plan -- the API returns 403 "Upgrade to GitHub Pro or make this
# repository public". So this script only works once the repo is either:
#   * made public, or
#   * on GitHub Pro / Team / Enterprise.
# It also needs `gh` authenticated with admin on the repo.
#
# Minimal + reversible by design: requires only the single `gate` check, leaves
# enforce_admins=false (so an admin can still merge in an emergency), and sets no review
# or push restrictions. Rollback:
#   gh api -X DELETE repos/<owner>/<repo>/branches/<branch>/protection
#
# Usage: scripts/apply_branch_protection.sh [owner/repo] [branch]
set -euo pipefail

REPO="${1:-w6rsty/worse2026}"
BRANCH="${2:-master}"

echo "Applying branch protection to ${REPO}@${BRANCH}: require the 'gate' check."
gh api -X PUT "repos/${REPO}/branches/${BRANCH}/protection" \
  -H "Accept: application/vnd.github+json" \
  --input - <<'JSON'
{
  "required_status_checks": { "strict": false, "contexts": ["gate"] },
  "enforce_admins": false,
  "required_pull_request_reviews": null,
  "restrictions": null
}
JSON

echo "Done. Read-back:"
gh api "repos/${REPO}/branches/${BRANCH}/protection" --jq '{required_status_checks: .required_status_checks, enforce_admins: .enforce_admins.enabled}'
