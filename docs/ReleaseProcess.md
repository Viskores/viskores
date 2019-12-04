Release Process
===============

# High level view
1. Craft Release Branch
    - Generate change log
    - Commit change log
    - Commit update to version.txt
    - Merge release branch
2. Tag release
3. Add Gitlab Release Notes
4. Announce


# Craft Release Branch

Construct a git branch named `release_X.Y`

## Generate change log
Construct a `docs/changelog/X.Y/` folder.
Construct a `docs/changelog/X.Y/release-notes.md` file

Use the following template for `release-notes.md`:

```md
VTK-m N Release Notes
=======================

# Table of Contents
1. [Core](#Core)
    - Core change 1
2. [ArrayHandle](#ArrayHandle)
3. [Control Environment](#Control-Environment)
4. [Execution Environment](#Execution-Environment)
5. [Worklets and Filters](#Worklets-and-Filters)
6. [Build](#Build)
7. [Other](#Other)


# Core

## Core change 1 ##

changes in core 1

# ArrayHandle

# Control Enviornment

# Execution Environment

# Execution Environment

# Worklets and Filters

# Build


# Other
```

For each individual file in `docs/changelog` move them
to the relevant `release-notes` section.

  - Make sure each title and entry in the table of contents use full vtkm names `vtkm::cont::Field` instead of Field
  - Make sure each title and entry DOESNT have a peroid at the end
  - Make sure any sub-heading as part of the changelog is transformed from `##` to `###`.
  - Entries for `Core` are reserved for large changes that significantly improve VTK-m users life, or are
    major breaking changes.

## Commit change log

Remove each individual change log file from the git repository
using `git rm docs/changelog/*.md`

Add the changelog to the git history using `git add changelog/X.Y/release-notes.md`

make a commit with the message:
```
Add release notes for vX.Y.Z
```

## Commit update to version.txt

Add a second commit to the `release_X.Y` branch that contains the update
of the `version.txt` file to the correct version. The commit diff should look like:
```
--- a/version.txt
+++ b/version.txt
@@ -1 +1 @@
-1.4.0
+1.5.0
```

The commit message should read:
```
X.Y.0 is our Nth official release of VTK-m.

The major changes to VTK-m from A.B.C can be found in:
  docs/changelog/X.Y/release-notes.md
```

## Merge release branch

At this point you should be ready to merge this branch.
So push to the `release_X.Y` branch to gitlab and open a merge request.
The dashboards should produce a configure warning that the new release git tag
can't be found. This is to be expected.

Before completing the merge it is a good idea to review the rendered markdown of the release notes to ensure that they display properly.
On the merge request page, click on the Changes tab and then for the `release.X.Y` file click the View file button in the upper right.

# Tag Release

Once the merge request is merged we can add the tag.
VTK-m marks the commit that contains the modifications to `version.txt` as the tag location, not the merge commit.
After the merge this would be the second commit shown by git log as shown below:

```
git checkout master
git pull
git log -n2
  Merge: d3d3e441 f66d980d
  commit f66d980d (HEAD -> release_X.Y.0, gitlab/release_X.Y.0, master)
```

To place the tag at the correct location you will use the following command:
```
git tag -a -f vX.Y.Z SHA1
```

For the above example the `SHA1` would be `f66d980d`

This should prompt you to add a message to the tag. The message should be identical to the one
you used in the commit.

## Push git tags

Now you will need to push the tag back to gitlab so you will need to do the following:

```
git remote add origin_update_tags git@gitlab.kitware.com:vtk/vtk-m.git
git push --tags origin_update_tags
git remote rm origin_update_tags

```

# Add Gitlab Release Notes

Now that the VTK-m release is on gitlab we have to add the associated changelog to the release
entry on gitlab.

Go to `https://gitlab.kitware.com/vtk/vtk-m/-/tags/vX.Y.Z/release/edit` to edit the release page.
Copy the contents of `docs/changelog/X.Y/release-notes.md`

# Email Announcements

Announce the new VTK-m release on the mailing list. You will need to compute
the number of merge requests, changelog entries, and maybe # of authors.

To compute the number of unique committers
```
git log --format="%an" v1.4.0..v1.5.0 | sort -u | wc -l
```

To compute the number of merge requests
```
git log v1.4.0..v1.5.0 | grep 'Merge | wc -l
```

A standard template to use is:


```
Hi All,

VTK-m 1.5.0 is now released, and a special thanks to everyone that has
contributed to VTK-m since our last release. The 1.5.0 release contains
over 100000 merge requests, and 100000 entries to the changelog .

Below are all the entries in the changelog, with more details at (
https://gitlab.kitware.com/vtk/vtk-m/tags/vX.Y.0 ) or in the vtkm
repository at `docs/X.Y/release-notes.md`

1. Core
    - Core change 1
2. ArrayHandle
3. Control Environment
4. Execution Environment
5. Worklets and Filters
6. Build
7. Other
```
