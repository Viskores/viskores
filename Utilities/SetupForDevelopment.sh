#!/usr/bin/env bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

echo "Setting up useful Git aliases..." &&

# Rebase main by default
git config rebase.stat true
git config branch.main.rebase true

# General aliases that could be global
git config alias.pullall '!bash -c "git pull && git submodule update --init"' &&
git config alias.prepush 'log --graph --stat origin/main..' &&
git config alias.pull-main 'fetch origin main:main' &&
