# Fix compile when testing is turned off

There were some places in VTK-m's code that included test header files even
though they were not tests. As more code goes into libraries, this can
break the build.

Remove VTK-m library dependence on testing code where found. Also added a
CI build that turns off all testing to check for this condition in the
future.
