# HotFix Guide

## HotFix general instructions

The following instructions intend to be general case for applying hotfixes in
release branches, for more specific cases, simplified instructions are to be
found in the below sub-sections.

1. Find the oldest relevant release branch BASE to which this hotfix applies.
   - Relevant release branches include: release, and release-specific
     maintained branches.
2. Create a hotfix branch branching from BASE.
   - if the hotfix branch already exists `git rebase --onto BASE`.
3. Open a pull request targeting:
   - main, if applies to main.
   - Otherwise, release, if applies to the latest release.
   - Otherwise, the most recent release-specific branch to which this hotfix
     applies.
   - Lastly, if none of above, BASE.
4. (If needed) If the hotfix is a backport (cherry-picked) of an existing pull
   request, add a cross-reference to the existing pull request with the format
   of `#1234` inside the description of the newly created pull request.
   - Cherry-pick each of the relevant commits of the existing pull request using
     `git cherry-pick -x XXYYZZ`.

## HotFix for latest release and main branch only

For hotfixes that applies to release and main branch, create a branch based
off of the tip of the release branch and create a pull request targeting main.

If the hotfix branch already exists based off of a commit in the main branch,
you can change the base branch of the hotfix branch from main to latest
release with:

```
# Assuming that both your local main and release branch are updated; and
# assuming that you are currently in your topic branch

git rebase --onto release main
```

Next, you can bring this commit to __release__ cherry-pick the merged commit(s)
(`git cherry-pick -x`) into a small pull request targeting __release__ to tick
off that item.

Lastly, the main branch history will be reconnected with release as part of
the next release, as explained in
[here](#How-main-and-release-branch-are-connected).

## HotFix for release branch only

For hotfixes that only applies to release branch, whose changes are unneeded in
main, create a branch based off of the __release__ branch and create a
pull request targeting __release__ branch. Proceed as in a regular PR.

### How main and release branch are connected

Periodically, typically as part of cutting a new release (see
`docs/NewRelease.md.tmpl`), whoever is performing the release connects the
release branch back into main using `git merge -s ours`. __Note__ that the
`-s ours` strategy does not actually bring any change into main, it solely
creates an empty merge commit in main connecting the release branch history,
since the real changes were already brought to main individually as
described above.
