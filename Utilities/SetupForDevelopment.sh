#!/usr/bin/env bash

cd "${BASH_SOURCE%/*}/.." &&
Utilities/GitSetup/setup-user && echo &&
Utilities/GitSetup/setup-hooks && echo &&


(Utilities/GitSetup/setup-upstream ||
 echo 'Failed to setup origin.  Run this again to retry.') && echo &&
(Utilities/GitSetup/setup-gitlab ||
 echo 'Failed to setup GitLab.  Run this again to retry.') && echo &&
Utilities/GitSetup/tips


echo "Setting up useful Git aliases..." &&

# Rebase master by default
git config rebase.stat true
git config branch.master.rebase true

# General aliases that could be global
git config alias.pullall '!bash -c "git pull && git submodule update --init"' &&
git config alias.prepush 'log --graph --stat origin/master..' &&
git config alias.pull-master 'fetch origin master:master' &&

# Alias to push the current topic branch to GitLab
git config alias.gitlab-push '!bash Utilities/GitSetup/git-gitlab-push' &&
echo "Set up git gitlab-push" &&
git config alias.gitlab-sync '!bash Utilities/GitSetup/git-gitlab-sync' &&
echo "Set up git gitlab-sync" &&
true

# shellcheck disable=SC2034
SetupForDevelopment=1
# shellcheck disable=SC2154
git config hooks.SetupForDevelopment "${SetupForDevelopment_VERSION}"

# Setup VTK-m-specifc LFS config
#


OriginURL="$(git remote get-url origin)"
if [[ "$OriginURL" =~ ^git@gitlab\.kitware\.com:vtk/vtk-m\.git$ ]]
then











fi
