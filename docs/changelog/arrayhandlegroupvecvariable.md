# `ArrayHandleGroupVecVariable` holds now one more offset.

This change affects the usage of both `ConvertNumComponentsToOffsets` and
 `make_ArrayHandleGroupVecVariable`.

The reason of this change is to remove a branch in
`ArrayHandleGroupVecVariable::Get` which is used to avoid an array overflow, 
this in theory would increases the performance since at the CPU level it will 
remove penalties due to wrong branch predictions.

The change affects `ConvertNumComponentsToOffsets` by both:

 1. Increasing the numbers of elements in `offsetsArray` (its second parameter) 
    by one.

 2. Setting `sourceArraySize` as the sum of all the elements plus the new one 
    in `offsetsArray`

Note that not every specialization of `ConvertNumComponentsToOffsets` does
return `offsetsArray`. Thus, some of them would not be affected.

Similarly, this change affects `make_ArrayHandleGroupVecVariable` since it
expects its second parameter (offsetsArray) to be one element bigger than
before.
