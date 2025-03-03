#!/usr/bin/env bash

echo "Setting up useful Git aliases..." &&

# Rebase main by default
git config rebase.stat true
git config branch.main.rebase true

# General aliases that could be global
git config alias.pullall '!bash -c "git pull && git submodule update --init"' &&
git config alias.prepush 'log --graph --stat origin/main..' &&
git config alias.pull-main 'fetch origin main:main' &&
