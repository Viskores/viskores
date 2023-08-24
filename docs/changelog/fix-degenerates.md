# Fix degenerate cell removal

There was a bug in `CleanGrid` when removing degenerate polygons where it
would not detect if the first and last point were the same. This has been
fixed.

There was also an error with function overloading that was causing 0D and
3D cells to enter the wrong computation for degenerate cells. This has also
been fixed.
