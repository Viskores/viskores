Contributing to VTK-m
=====================

This page documents at a very high level how to contribute to VTK-m.
Please check our [developer instructions][] for a more detailed guide to
developing and contributing to the project.

1.  Register [GitLab Access] to create an account and select a user name.

2.  [Fork VTK-m][] into your user's namespace on GitLab.

3.  Create a local clone of the main VTK-m repository:

        $ git clone https://gitlab.kitware.com/vtk/vtk-m.git vtkm
        $ cd vtkm
    The main repository will be configured as your `origin` remote.

4.  Associate your GitLab fork of VTK-m VTK-m repository:

        $ git remote add gitlab git@gitlab.com:username/vtk-m.git

    Your fork will be configured as your `gitlab` remote.

5.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

6.  Push commits in your topic branch to your fork in GitLab:

        $ git push gitlab HEAD

7.  Visit your fork in GitLab, browse to the "**Merge Requests**" link on the
    left, and use the "**New Merge Request**" button in the upper right to
    create a Merge Request.


VTK-m uses GitLab for code review and Buildbot to test proposed
patches before they are merged.

Our [Issue Tracker][] is used to document feature requests and technical issues.

Our [Wiki][] is out to propose new infrastructure designs and host other
documentation.

[developer instructions]: http://m.vtk.org/index.php/Contributing_to_VTK-m
[GitLab Access]: https://gitlab.kitware.com/users/sign_in
[Fork VTK-m]: https://gitlab.kitware.com/vtk/vtk/fork/new
[Issue Tracker]: https://gitlab.kitware.com/vtk/vtk-m/issues
[Wiki]: http://m.vtk.org/
