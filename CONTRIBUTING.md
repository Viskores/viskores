# Contributing to VTK-m #

This page documents how to develop VTK-m through [Git](http://git-scm.com).

Git is an extremely powerful version control tool that supports many
different "workflows" for individual development and collaboration. Here we
document procedures used by the VTK-m development community. In the
interest of simplicity and brevity we do *not* provide an explanation of
why we use this approach.


## Setup ##

Before you begin, perform initial setup:

1.  Register [GitLab Access] to create an account and select a user name.

2.  [Fork VTK-m] into your user's namespace on GitLab.

3.  Use Git to create a local clone of the main VTK repository:

        $ git clone https://gitlab.kitware.com/vtk/vtk-m.git
        $ cd vtk-m

    The main repository will be configured as your `origin` remote.

4.  Run the developer setup script to prepare your VTK-m work tree and
    create Git command aliases used below:

        $ ./Utilities/SetupForDevelopment.sh

    This will prompt for your GitLab user name and configure a remote
    called `gitlab` to refer to it.

5. (Optional but highly recommended.) 
    [Register with the VTK-m dashboard] on Kitware's CDash instance to
    better know how your code performs in regression tests. After
    registering and signing in, click on "All Dashboards" link in the upper
    left corner, scroll down and click "Subscribe to this project" on the
    right of VTK-m.

6.  (Optional but highly recommended.) 
    [Sign up for the VTK-m mailing list] to communicate with other
    developers and users.

[GitLab Access]: https://gitlab.kitware.com/users/sign_in
[Fork VTK-m]: https://gitlab.kitware.com/vtk/vtk-m/forks/new
[Register with the VTK-m dashboard]: https://open.cdash.org/register.php
[Sign up for the VTK-m mailing list]: http://vtk.org/mailman/listinfo/vtkm


## Workflow ##

VTK-m development uses a [branchy workflow] based on topic branches. Our
collaboration workflow consists of three main steps:

1.  Local Development:
      * [Update](#update)
      * [Create a Topic](#create-a-topic)

2.  Code Review (requires [GitLab Access]):
      * [Share a Topic](#share-a-topic)
      * [Create a Merge Request](#create-a-merge-request)
      * [Review a Merge Request](#review-a-merge-request)
      * [Reformat a Topic](#reformat-a-topic)
      * [Revise a Topic](#revise-a-topic)

3.  Integrate Changes:
      * [Merge a Topic](#merge-a-topic) (requires permission in GitLab)

[branchy workflow]: http://public.kitware.com/Wiki/Git/Workflow/Topic


## Update ##

1.  Update your local `master` branch:

        $ git checkout master
        $ git pull

2.  Optionally push `master` to your fork in GitLab:

        $ git push gitlab master


## Create a Topic ##

All new work must be committed on topic branches. Name topics like you
might name functions: concise but precise. A reader should have a general
idea of the feature or fix to be developed given just the branch name.

1.  To start a new topic branch:

        $ git fetch origin

2.  For new development, start the topic from `origin/master`:

        $ git checkout -b my-topic origin/master

3.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

    Caveats:
      * Data files must be placed under a folder explicitly named 'data'.
        This is required as VTK-m uses Git-LFS to efficiently support data
        files.


### Guidelines for Commit Messages ###

Remember to *motivate & summarize*. When writing commit messages. Get into
the habit of creating messages that have enough information for any
developer to read and glean relevant information such as:

1.  Is this change important and why?
2.  If addressing an issue, which issue(s)?
3.  If a new feature, why is it useful and/or necessary?
4.  Are there background references or documentation?

A short description of what the issue being addressed and how will go a
long way towards making the log more readable and the software more
maintainable. VTK-m requires that your message start with a single subject
line, followed by a blank line, followed by the message body which contains
the more detailed explanatory text for the commit. You can consider a
commit message to very similar to an email with the first line being the
subject of an email and the rest of the text as the body.

Style guidelines for commit messages are as follows:

1.   Separate subject from body with a blank line
2.   Limit the subject line to 78 characters
3.   Capitalize the subject line
4.   Use the imperative mood in the subject line e.g. "Refactor foo" or
     "Fix Issue #12322", instead of "Refactoring foo", or "Fixing issue
     #12322".
5.   Wrap the body at 80 characters
6.   Use the body to explain `what` and `why` and if applicable a brief
    `how`.


## Share a Topic ##

When a topic is ready for review and possible inclusion, share it by
pushing to a fork of your repository in GitLab. Be sure you have registered
and signed in for [GitLab Access] and created your fork by visiting the
main [VTK-m GitLab] repository page and using the "Fork" button in the
upper right.

[VTK-m GitLab]: https://gitlab.kitware.com/vtk/vtk-m

1.  Checkout the topic if it is not your current branch:

        $ git checkout my-topic

2.  Push commits in your topic branch to your fork in GitLab:

        $ git gitlab-push

    Notes:
      * If you are revising a previously pushed topic and have rewritten
        the topic history, add `-f` or `--force` to overwrite the
        destination.
      * The `gitlab-push` script also pushes the `master` branch to your
        fork in GitLab to keep it in sync with the upstream `master`.

    The output will include a link to the topic branch in your fork in
    GitLab and a link to a page for creating a Merge Request.


## Create a Merge Request ##

When you [pushed your topic branch](#share-a-topic), it will provide you
with a url of the form

    https://gitlab.kitware.com/<username>/vtk-m/merge_requests/new

You can copy/paste that into your web browser to create a new merge
request. Alternately, you can visit your fork in GitLab, browse to the
"**Merge Requests**" link on the left, and use the "**New Merge Request**"
button in the upper right.

Once at the create merge request page, follow these steps. Many of these
will be filled out for you.

1.  In the "**Source branch**" box select the `<username>/vtk-m` repository
    and the `my-topic` branch.

2.  In the "**Target branch**" box select the `vtk/vtk-m` repository and
    the `master` branch. It should be the default.

3.  Use the "**Compare branches**" button to proceed to the next page and
    fill out the merge request creation form.

4.  In the "**Title**" field provide a one-line summary of the entire
    topic. This will become the title of the Merge Request.

    Example Merge Request Title:

        Add OpenMP Device Adapter

5.  In the "**Description**" field provide a high-level description of the
    change the topic makes and any relevant information about how to try
    it. 
    *   Use `@username` syntax to draw attention of specific developers.
        This syntax may be used anywhere outside literal text and code
        blocks.  Or, wait until the [next step](#review-a-merge-request)
        and add comments to draw attention of developers.
    *   Optionally use a fenced code block with type `message` to specify
        text to be included in the generated merge commit message when the
        topic is [merged](#merge-a-topic).

    Example Merge Request Description:

        This branch adds a new device adapter that uses new OpenMP 4+ features
        including Task groups to better handle unbalanced and irregular domains

        ```message
        Add a OpenMP 4+ task-based device adapter.
        ```

        Cc: @user1 @user2

6.  The "**Assign to**", "**Milestone**", and "**Labels**" fields may be
    left blank.

7.  Use the "**Submit merge request**" button to create the merge request
    and visit its page.

### Guidelines for Merge Requests ###

Remember to *motivate & summarize*. When creating a merge request, consider
the reviewers and future perusers of the software. Provide enough
information to motivate the merge request such as:

1.  Is this merge request important and why?
2.  If addressing an issue, which issue(s)?
3.  If a new feature, why is it useful and/or necessary?
4.  Are there background references or documentation?

Also provide a summary statement expressing what you did and if there is a
choice in implementation or design pattern, the rationale for choosing a
certain path. Notable software or data features should be mentioned as
well.

A well written merge request will motivate your reviewers, and bring them
up to speed faster. Future software developers will be able to understand
the reasons why something was done, and possibly avoid chasing down dead
ends, Although it may take you a little more time to write a good merge
request, you’ll likely see payback in faster reviews and better understood
and maintainable software.


## Review a Merge Request ##

Add comments mentioning specific developers using `@username` syntax to
draw their attention and have the topic reviewed. After typing `@` and some
text, GitLab will offer completions for developers whose real names or user
names match.

Comments use [GitLab Flavored Markdown] for formatting. See GitLab
documentation on [Special GitLab References] to add links to things like
merge requests and commits in other repositories.

[GitLab Flavored Markdown]: https://gitlab.kitware.com/help/markdown/markdown
[Special GitLab References]: https://gitlab.kitware.com/help/markdown/markdown#special-gitlab-references

### Reviews ###

Reviewers may add comments providing feedback or to acknowledge their
approval. All comments use the [GitLab Flavored Markdown][], any line of a
comment may be exactly one of the following votes followed by nothing but
whitespace before the end of the line:

  * `-1` or :-1: (`:-1:`) means "The change is not ready for integration."
  * `+1` or :+1: (`:+1:`) means "The change is ready for integration."

These are used to inform the author that a merge srequest has been approved
for [merging](#merge-a-topic).

#### Fetching Changes ####

One may fetch the changes associated with a merge request by using the `git
fetch` command line shown at the top of the Merge Request page. It is of
the form:

    $ git fetch https://gitlab.kitware.com/$username/vtk-m.git $branch

This updates the local `FETCH_HEAD` to refer to the branch.

There are a few options for checking out the changes in a work tree:

  * One may checkout the branch:

        $ git checkout FETCH_HEAD -b $branch

    or checkout the commit without creating a local branch:

        $ git checkout FETCH_HEAD

  * Or, one may cherry-pick the commits to minimize rebuild time:

        $ git cherry-pick ..FETCH_HEAD

### Robot Reviews ###

The "Kitware Robot" automatically performs basic checks on the commits and
adds a comment acknowledging or rejecting the topic. This will be repeated
automatically whenever the topic is updated. A re-check may be explicitly
requested by adding a comment with a single *trailing* line:

    Do: check

A topic cannot be [merged](#merge-a-topic) until the automatic review
succeeds.

### Testing ###

VTK-m has a [buildbot](http://buildbot.net) instance watching for merge
requests to test. Each time a merge request is updated the buildbot user
(@buildbot) will automatically trigger a new build on all VTK-m buildbot
workers. The buildbot user (@buildbot) will respond with a comment linking
to the CDash results when it schedules builds.

The buildbot user (@buildbot) will also respond to any comment with the
form:

    Do: test

The `Do: test` command accepts the following arguments:

  * `--oneshot` 
        only build the *current* hash of the branch; updates will not be
        built using this command
  * `--stop` 
        clear the list of commands for the merge request
  * `--superbuild` 
        build the superbuilds related to the project
  * `--clear` 
        clear previous commands before adding this command
  * `--regex-include <arg>` or `-i <arg>` 
        only build on builders matching `<arg>` (a Python regular
        expression)
  * `--regex-exclude <arg>` or `-e <arg>` 
        excludes builds on builders matching `<arg>` (a Python regular
        expression)

Multiple `Do: test` commands may be given in separate comments. Buildbot
may skip tests for older branch updates that have not started before a test
for a new update is requested.

Build names always follow this pattern:

        SHA-build#-[os-libtype-buildtype+feature1+feature2]-topic

  * SHA: The shortened 8-digit SHA identifying the git commit being tested
  * build: `build####` with `####` replaced by a unique number for the build
  * os: one of `windows`, `osx`, or `linux`
  * libtype: `shared` or `static`
  * buildtype: `release` or `debug`
  * feature: alphabetical list of features enabled for the build
  * topic: the git topic branch being tested


## Reformat a Topic ##

The "Kitware Robot" automatically performs basic code formatting on the
commits and adds a comment acknowledging or rejecting a merge request based
on the format. You may request "Kitware Robot" to automatically reformat
the remote copy of your branch by issuing the following command in a merge
request page comment:

    Do: reformat

This reformatting of the topic rewrites the commits to fix the formatting
errors, and causes the version on the developers machine to differ from
version on the gitlab server. To resolve this issue you must update the
local version to match the reformatted one on the server if you wish to
extend or revise the topic.

1.  Checkout the topic if it is not your current branch:
        $ git checkout my-topic

2.  Get the new version from gitlab

        $ git gitlab-sync -f


If you do not wish to have the "Kitware Robot" automatically reformat your
branch you can do so manually by running [clang-format] manually on each
commit of your branch. This must be done by [revising each
commit](#revise-a-topic) not as new commits onto the end of the branch.

[clang-format]: https://clang.llvm.org/docs/ClangFormat.html


## Revise a Topic ##

Revising a topic is a special way to modify the commits within a topic.
Normally during a review of a merge request a developer will resolve issues
brought up during review by adding more commits to the topic. While this is
sufficient for most issues, some issues can only be resolved by rewriting
the history of the topic.

If a reviewer has asked explicitly for certain commits to be rewritten, you
will need to revise the commits and force push it back to GitLab for
another review. To revise a topic for another review as follows:

1.  Checkout the topic if it is not your current branch:

        $ git checkout my-topic

2.  To revise the `3`rd commit back on the topic:

        $ git rebase -i HEAD~3

    (Substitute the correct number of commits back, as low as `1`.) Follow
    Git's interactive instructions.

3.  Push commits in your topic branch to your fork in GitLab:

        $ git gitlab-push -f

    Notes:
      * You need to add `-f` or `--force` to overwrite the destination as
        you are revising a previously pushed topic and have rewritten the
        topic history.

## Merge a Topic ##

After a topic has been reviewed and approved in a GitLab Merge Request,
authorized developers may add a comment with a single *trailing* line:

    Do: merge

to ask that the change be merged into the upstream repository. By
convention, only merge if you have recieved `+1` . Do not request a merge
if any `-1` review comments have not been resolved.

### Merge Success ###

If the merge succeeds the topic will appear in the upstream repository
`master` branch and the Merge Request will be closed automatically.

### Merge Failure ###

If the merge fails (likely due to a conflict), a comment will be added
describing the failure. In the case of a conflict, fetch the latest
upstream history and rebase on it:

    $ git fetch origin
    $ git rebase origin/master

Return to the [above step](#share-a-topic) to share the revised topic.
