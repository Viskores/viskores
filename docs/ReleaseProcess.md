Release Process
===============

## Prologue

This document is divided in two parts:
 - A overview of the branching and release scheme used in VTK-m.
 - A concise instructions to get started with the release process.

The actual release process can be found at
`.gitlab/issue_templates/NewRelease.md`.

# Overview of the branching scheme

Our current scheme is composed of two branches: release and master.

While all of the development is performed in the master branch, once in a while
when we want to do a release, we tag a commit in master and we push it to the
release branch.

Also there are times when we want to get _hotfix_ that affects previous releases
to the release branch, in this case we can also push the merge request commit
with the fix into the release branch.

## Release-specific branches

Sometime we need to keep maintaining an older release which does not sit at the
tip of the release branch. For this purpose we use release-specific branches
with the name of `release-@MAJOR_VER@.@MINOR_VER@`.

To create a new release-specific branch you need someone with push access to
create a release-specific branch pointing at the latest commit of the minor
release of interest, this is, for release-1.7 it will be v1.7.1 as opposed to
v1.7.0.

There can be the case that between release X.Y and X.Y+1 there are hotfixes
commits that do not correspond to a patch tag releases. In this particular case,
create the release-specific branch pointing to the last commit before X.Y+1.

To add a hotfix to a release-specific branch, follow the instructions described
in [HotFixes](./ReleaseHotFix.md) noting that you need to adjust the branch
names from release to `release-@MAJOR_VER@.@MINOR_VER@`.


A not so simple example of how the branching scheme looks like can be found
here:

```git
# → git log --graph --decorate --oneline --all

*   2e9230d (master) Merge branch 'update'
|\
| * 59279dd (HEAD -> release, tag: v1.0.1) v1.0.1 2nd release of VTKm
| * b60611b Add release notes for v1.0.1
| *   9d26451 Merge branch 'master' into update
| |\
| |/
|/|
* | 75137e5 Unrelated commit
* | e982be0 Merge branch 'release'
|\|
| *   f2c4eb6 Merge branch 'hotfix' into release
| |\
| | * c1c2655 Hotfix
| |/
* | e53df9e Unrelated commit
* | ec6b481 Unrelated commit
|/
* 0742a47 (tag: v1.0.0) v1.0.0 1st release of VTKm
* 4fe993c Add release notes for v1.0.0
```

This will make the release branch to only contain tags and _hotfix_ merges as
shown in here:

```git
# git log --graph --decorate --oneline --first-parent release

* 59279dd (HEAD -> release, tag: v1.0.1) v1.0.1 2nd release of VTKm
* b60611b Add release notes for v1.0.1
* 9d26451 Merge branch 'master' into update
* f2c4eb6 Merge branch 'hotfix' into release
* 0742a47 (tag: v1.0.0) v1.0.0 1st release of VTKm
* 4fe993c Add release notes for v1.0.0
```

# Get started with a new Release

1. Go to `https://gitlab.kitware.com/vtkm/vtk-m/` and open a new issue.
2. Generate and copy to clipboard the release script (-rcN is optional):
```
# Download pyexpander (Available in pip)
expander.py --eval 'version="X.Y.Z-rcN"' docs/NewRelease.md.tmpl | xclip -selection c
```
3. Paste the output in the issue and follow the steps.
