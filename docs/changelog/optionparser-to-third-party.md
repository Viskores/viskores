# Wrap third party optionparser.h in vtkm/cont/internal/OptionParser.h

Previously we just took the optionparser.h file and stuck it right in
our source code. That was problematic for a variety of reasons.

1. It incorrectly assigned our license to external code.
2. It made lots of unnecessary changes to the original source (like
   reformatting).
3. It made it near impossible to track patches we make and updates to
   the original software.

Instead, use the third-party system to track changes to optionparser.h
in a different repository and then pull that into ours.
