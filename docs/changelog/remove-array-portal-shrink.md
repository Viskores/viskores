# Remove ArrayPortalShrink, behavior subsumed by ArrayHandleView

ArrayPortalShrink originaly allowed a user to pass in a delegate array portal
and then shrink the reported array size without actually modifying the
underlying allocation.  An iterator was also provided that would
correctly iterate over the shrunken size of the stored array.

Instead of directly shrinking the original array, it is prefered
to create an ArrayHandleView from an ArrayHandle and then specify the 
number of values to use in the ArrayHandleView constructor.
