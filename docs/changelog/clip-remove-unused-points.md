# Clip now doesn't copy unused points from the input to the output

Previously, clip would just copy all the points and point data from the input to the output,
and only append the new points. This would affect the bounds computation of the result.
If the caller wanted to remove the unused points, they had to run the CleanGrid filter
on the result.

With this change, clip now keeps track of which inputs are actually part of the output
and copies only those points.
