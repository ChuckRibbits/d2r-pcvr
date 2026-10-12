# Main branch rules

These JSON files record the intended GitHub rulesets for
`bearstonem/d2r-pcvr`. Committing the files does **not** apply them: GitHub stores
and enforces live rules separately in
[Settings > Rules > Rulesets](https://github.com/bearstonem/d2r-pcvr/settings/rules).
They can be imported there or applied through the GitHub REST API.

## Main integrity

`main-integrity.json` requires a pull request, resolves review conversations
before merge, blocks force pushes and prevents deletion of `main`.
**There are no bypass actors, including the owner.** Normal updates must go
through a PR, even for maintainers.

## Maintainer review

`maintainer-review.json` requires one approving review, requires code-owner
review, and dismisses approvals after new reviewable commits. The code owner
for all files is `@bearstonem`, as listed in [CODEOWNERS](../CODEOWNERS).

GitHub does not let authors approve their own PRs. To keep owner-authored
maintenance possible, this second ruleset gives `@bearstonem` (GitHub user ID
`75600543`) a **pull-request-only** review bypass. The owner should review and
approve outside contributions normally; the exception is intended for their
own PRs. It does not override the separate main-integrity ruleset, permit
direct pushes or allow unresolved review conversations to be merged.

See GitHub's explanation of
[pull-request-only bypass](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/creating-rulesets-for-a-repository#granting-bypass-permissions-for-your-ruleset).

## Maintenance

- Keep both rulesets active. Do not add a bypass to main integrity to land a change.
- Outside contributors fork and open a PR; maintainers use topic branches and PRs.
  This contribution policy is documented in [CONTRIBUTING.md](../../CONTRIBUTING.md).
  GitHub repository access controls prevent outside contributors from creating
  branches or pushing in this repository; branch rules require PRs into `main`.
- Update these files through a PR when intentionally changing the policy, then
  apply and verify the matching live GitHub settings. Do not create duplicate
  rulesets when updating an existing rule.
- If maintainership changes, update CODEOWNERS and the owner review exception.
- No build or test status checks are required by these initial rulesets. Reviewers
  must assess the validation reported in each PR. Add required checks only once
  the corresponding automation is running reliably.
