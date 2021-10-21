<!--
This template is for tracking a release of VTKm. Please replace the
following strings with the associated values:

  - `@VERSION@` - replace with base version, e.g., 1.6.0
  - `@RC@` - for release candidates, replace with "-rc?". For final, replace with "".
  - `@MAJOR@` - replace with major version number
  - `@MINOR@` - replace with minor version number

Please remove this comment.
-->
## Update VTK-m

  - [ ] Update `release` branch for **vtk-m** and create update branch
```
git fetch origin
git checkout release
git merge --ff-only origin/release
git submodule update --recursive --init
```
## Create update branch

<!-- if @RC@ == "-rc1"
  - [ ] Bring as a second parent the history of master (Solve conflicts always
        taking master's version)
```
	git merge --no-ff origin/master
```
-->

<!-- Do we have new release notes?
  - [ ] Craft or update [changelog](#generate-change-log)
        `docs/changelog/@MAJOR@.@MINOR@/release-notes.md` file.
  - [ ] Create release notes commit.
```
git add docs/changelog/@MAJOR@.@MINOR@/release-notes.md
git rm docs/changelog/*.md
git commit -m 'Add release notes for @VERSION@@RC@'
```
-->

  - [ ] Update the version and date in the LICENSE.md file.
  - [ ] Create update version commit:

```
# Create branch
git checkout -b update-to-v@VERSION@@RC@
echo @VERSION@@RC@ > version.txt

# Create commit with the following template
# Nth is counted by the number of tags
git commit -m '@VERSION@@RC@ is our Nth official release of VTK-m.

The major changes to VTK-m from (previous release) can be found in:
  docs/changelog/@MAJOR@.@MINOR@/release-notes.md' version.txt

```

  - [ ] `git tag -a -m 'VTKm @VERSION@@RC@' v@VERSION@@RC@ HEAD`
  - Integrate changes to `release` branch
    - [ ] Create a MR using the [release-mr script][1]
          (see [notes](#notes-about-update-mr)).
    - [ ] Get +1
    - [ ] `Do: merge`
  - Push tags
    - [ ] `git push origin v@VERSION@@RC@`

## Update Spack
 - [ ] Update Spack package: https://github.com/spack/spack/blob/develop/var/spack/repos/builtin/packages/vtk-m/package.py

## Post-release
  - [ ] Copy the contents of docs/changelog/@MAJOR@.@MINOR@/release-notes.md to
        the GitLab release.
  - [ ] Tag new version of the [VTK-m User Guide][2].
  - [ ] Post an [Email Announcements](#email-announcements) VTK-m mailing list.

---

# Annex 

## Generate change log
Construct a `docs/changelog/@MAJOR@.@MINOR@/` folder.
Construct a `docs/changelog/@MAJOR@.@MINOR@/release-notes.md` file

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

  - Make sure each title and entry in the table of contents use full vtkm names
    `vtkm::cont::Field` instead of Field.
  - Make sure each title and entry DOESN'T have a period at the end.
  - Make sure any sub-heading as part of the changelog is transformed from `##`
    to `###`.
  - Entries for `Core` are reserved for large changes that significantly improve
    VTK-m users life, or are major breaking changes.

## Notes about update-mr

[`update-mr` script][1] has the following requirements to work:

1. It needs a token to for authentication (reach @ben.boeckel for this)
2. It needs `kwrobot.release.vtkm` to have developer perms in your vtk-m repo.

Lastly, `update-mr` can be used multiple times with different commit in the same
branch.

## Email Announcements

Announce the new VTK-m release on the mailing list. You will need to compute
the number of merge requests, changelog entries, and maybe # of authors.

Example to compute the number of unique committers
```
git log --format="%an" v1.4.0..v1.5.0 | sort -u | wc -l
```

Example to compute the number of merge requests
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
https://gitlab.kitware.com/vtk/vtk-m/-/tags/v@VERSION@) or in the vtkm
repository at `docs/@MAJOR@.@MINOR@/release-notes.md`

1. Core
    - Core change 1
2. ArrayHandle
3. Control Environment
4. Execution Environment
5. Worklets and Filters
6. Build
7. Other
```

/cc @ben.boeckel

/cc @vbolea

/label ~"priority:required"

[1]:  https://gitlab.kitware.com/utils/release-utils/-/blob/master/release-mr.py
[2]:  https://gitlab.kitware.com/vtk/vtk-m-user-guide
