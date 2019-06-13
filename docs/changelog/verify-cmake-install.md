# VTK-m now can verify that it installs itself correctly

It was a fairly common occurrence of VTK-m to have a broken install
tree as it had no easy way to verify that all headers would be installed.

Now VTK-m offers a testing infrastructure that creates a temporary installed
version and is able to run tests with that VTK-m installed version. Currently
the only test is to verify that each header listed in VTK-m is also installed,
but this can expand in the future to include compilation tests.
