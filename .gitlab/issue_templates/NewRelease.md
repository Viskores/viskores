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

  - [ ] Create update branch `git checkout -b update-to-v@VERSION@`
<!-- if @RC@ == "-rc1"-->
  - [ ] Bring as a second parent the history of master (Solve conflicts always
        taking master's version)
```
	git merge --no-ff origin/master
```
<!-- endif -->

<!-- if not a patch release -->
  - [ ] Update the major and minor version in `version.txt`:
```
echo "@MAJOR@.@MINOR@.9999" > version.txt
git add version.txt`
```
<!-- endif -->

  - [ ] Update the version (not in patch releases) and date in the LICENSE.md
        file `git add LICENSE.md`.
  - [ ] Create commit that updates the License (and version.txt if modified):
```
git commit -m 'release: update version and License'
```

<!-- Do we have new release notes? -->
  - [ ] Craft or update [changelog](#generate-change-log)
        `docs/changelog/@VERSION@/release-notes.md` file.
  - [ ] Create release notes commit.
```
git add docs/changelog/@VERSION@/release-notes.md
git rm docs/changelog/*.md
git commit -m 'release: @VERSION@@RC@ release notes'
```
<!-- endif -->
  - [ ] Create update version commit:

```
echo @VERSION@@RC@ > version.txt
git add version.txt

# Create commit with the following template
# Nth is counted by the number of final release tags
git commit -m '@VERSION@@RC@ is our Nth official release of VTK-m.

The major changes to VTK-m from (previous release) can be found in:
  docs/changelog/@VERSION@/release-notes.md' version.txt
```

  - [ ] `git tag -a -m 'VTKm @VERSION@@RC@' v@VERSION@@RC@ HEAD`
  - Integrate changes to `release` branch
    - [ ] Create a MR using the [release-mr script][1]
          (see [notes](#notes-about-update-mr)).
<!-- if not patch release -->
    - [ ] Add (or ensure) at the bottom of the description of the merge request:
          `Backport: master:HEAD~1`
<!-- elseif patch release -->
    - [ ] Remove (or ensure) that at the bottom of the description of the merge
          request there is no `Backport` instruction.
<!-- endif -->
    - [ ] Get +1
    - [ ] `Do: merge`
  - Push tags
    - [ ] `git push origin v@VERSION@@RC@`

## Update Spack
 - [ ] Update Spack package: https://github.com/spack/spack/blob/develop/var/spack/repos/builtin/packages/vtk-m/package.py

## Post-release
  - [ ] Copy the contents of docs/changelog/@VERSION@/release-notes.md to
        the GitLab release.
<!-- if not patch release -->
  - [ ] Tag new version of the [VTK-m User Guide][2].
<!-- endif -->
  - [ ] Post an [Email Announcements](#email-announcements) VTK-m mailing list.
<!-- if not patch release -->
  - [ ] Ensure that the content of `version.txt` in master is
        `[@MAJOR@ @MINOR@](@MAJOR@.@MINOR@.9999)`.
<!-- endif release -->

---

# Annex

## Generate change log
Construct a `docs/changelog/@VERSION@/` folder.
Construct a `docs/changelog/@VERSION@/release-notes.md` file

Use the following template for `release-notes.md`:

```md
VTK-m N Release Notes
=======================

<!-- if is a patch release -->

| Merge request description                                           | Merge request id |
| ------------------------------------------------------------------- | ---------------- |
| Update the link to register with the VTK-m dashboard                | !2629            |
.
.
.
| CMAKE: CUDA ampere generate sm_80/86                                | !2688            |

<!-- else -->

# Table of Contents
1. [Core](#Core)
    - Core change 1
2. [ArrayHandle](#ArrayHandle)
3. [Control Environment](#Control-Environment)
4. [Execution Environment](#Execution-Environment)
5. [Worklets and Filters](#Worklets-and-Filters)
6. [Build](#Build)
7. [Other](#Other)

<!-- endif -->
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

## Notes about version.txt

Master and release branch do not share the same version.txt scheme. In the
release branch the patch and release-candidate version is observed whereas in
master the patch field is fixed to _9999_ indicating that each of its commit is
a developing release.

- Master:  `@MAJOR@.@MINOR@.9999`
- Release: `@MAJOR@.@MINOR@.@PATCH@@RC@`

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
repository at `docs/@VERSION@/release-notes.md`

<!-- if is a patch release -->

- Update the link to register with the VTK-m dashboard
.
.
.
- CMAKE: CUDA ampere generate sm_80/86

<!-- else -->
1. Core
    - Core change 1
2. ArrayHandle
3. Control Environment
4. Execution Environment
5. Worklets and Filters
6. Build
7. Other
<!-- endif -->
```

/cc @ben.boeckel

/cc @vbolea

/label ~"priority:required"

[1]:  https://gitlab.kitware.com/utils/release-utils/-/blob/master/release-mr.py
[2]:  https://gitlab.kitware.com/vtk/vtk-m-user-guide
