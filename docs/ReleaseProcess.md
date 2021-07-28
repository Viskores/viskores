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

Also there are times when we want to get _Hotfix_ that affects previous releases
to the release branch, in this case we can also push the merge request commit
with the fix into the release branch.

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

This will make the release branch to only contain tags and _HotFix_ merges as
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
2. At the _issue template_ dropdown menu select: `NewRelease.md`
3. Now remove the comments and substitute the variables surrounded by `@`.
4. Post the issue and follow the steps.
