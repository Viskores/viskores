# Fixed Flying Edges Crash

There was a bug in VTK-m's flying edges algorithm (in the contour filter
for uniform structured data) that cause the code to read an index from
uninitialized memory. This in turn caused memory reads from an
inappropriate address that could cause bad values, failed assertions, or
segmentation faults.

The problem was caused by a misidentification of edges at the positive z
boundary. Due to a typo, the z index was being compared to the length in
the y dimension. Thus, the problem would only occur in the case where the y
and z dimensions were of different sizes and the contour would go through
the positive z boundary of the data, which was missing our test cases.

