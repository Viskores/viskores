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
3. Open a merge-request targeting:
   - master, if applies to master.
   - Otherwise, release, if applies to the latest release.
   - Otherwise, the most recent release-specific branch to which this hotfix
     applies.
   - Lastly, if none of above, BASE.
4. (If needed) If the hotfix is a backport (cherry-picked) of an existing merge-requests,
   add a cross-reference each of the existing merge-request with the format of `!1234`
   inside the description of the newly created merge-request.
   - Cherry-pick each of the relevant commits of the existing merge-requests using
     `git cherry-pick -x XXYYZZ`. 
5. At the bottom of the description of the merge-request add: `Backport: branch_name`
   directive for each of the branches that exists between BASE (inclusive) and
   the branch that we target our merge-request (exclusive).

In the case of merge conflicts in any of the backports refer to [Kitware backport guide][1].

## HotFix for latest release and master branch only

For hotfixes that applies to release and master branch, create a branch based
off of the tip of the release branch and create a merge-request targeting master.

If the hotfix branch already exists based off of a commit in the master branch,
you can change the base branch of the hotfix branch from master to latest
release with:

```
# Assuming that both your local master and release branch are updated; and
# assuming that you are currently in your topic branch

git rebase --onto release master
```

Next, you can bring this commit to __release__ by adding the following line to
the bottom of the MR description: `Backport: release`. This directive will later
internally instruct the Kitware Robot to bring the changes of the hotfix branch
creating a merge commit in the release branch with its second parent being the
hotfix branch tip.

Lastly, the master branch history will be automatically connected with
release after the merge-request is merged as explained in
[here](#How-master-and-release-branch-are-connected).

## HotFix for release branch only

For hotfixes that only applies to release branch, whose changes are unneeded in
master, create a branch based off of the __release__ branch and create a
merge-request targeting __release__ branch. Proceed as in a regular MR.

### How master and release branch are connected

Every merge commit in the release branch will be automatically connected to
master branch by our Gitlab robot creating a merge-request using the
`-s ours` strategy, __note__ that `-s ours` strategy does not actually bring any
change to the target branch, it will solely create an empty merge commit in master
connecting release branch..


## Other cases

There are more possible case scenarios that you might encounter while attempting
to bring a hotfix to release, for further cases please refer to the
[Kitware backport guide][1] which this document is based of.

[1]: https://gitlab.kitware.com/utils/git-workflow/-/wikis/Backport-topics
