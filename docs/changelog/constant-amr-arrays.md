# Store constant AMR arrays with less memory

The `AmrArrays` filter generates some cell fields that specify information
about the hierarchy, which are constant across all cells in a partition.
These were previously stored as an array with the same value throughout.
Now, the field is stored as an `ArrayHandleConstant`, which does not
require any real storage. Recent changes to VTK-m allow code to extract the
array as a component efficiently without knowing the storage type.
