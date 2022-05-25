Release HotFix
===============

# HotFix from master branch

## On a single MR

You have created a branch from master branch and you have a MR request targeting
the __master__ branch.

You can bring this commit to __release__ by adding the following line to
the bottom of the MR description.

```
Backport: release
```

This will cherry-pick this commit and push it to __release__ after typing `Do:
merge` in a comment.

You must also make sure that there will not be any merge conflict with the 
__release__ branch, thus you need to create an additional commit using the following
command:

```
git merge --no-ff origin/release
```

This will ensure that backport will be able to push your commit to __release__.

## On multiple MRs

1. Create one merge request sourcing your HotFix branch and targeting __master__
and merge.

2. Create one merge request sourcing __master__ and targeting __release__ and merge.

# HotFix from release branch

You have created a branch from the __release__ branch and you have a MR request
targeting __release__, you can proceed as in a regular MR.

Every merge in release will be automatically brought to master by the robot
using `-s ours` strategy. 

__VERY IMPORTANT__: `-s ours` strategy does not actually bring any change to the 
target branch, thus if needed you might want to bring the changes
from the HotFix to __master__ by creating a another MR which cherry-picks
the merge commit in `release` for the given HotFix.

Use the difference to first parent for the cherry-pick commit:

```
git cherry-pick -m1 -x <HASH OF COMMIT>
```
