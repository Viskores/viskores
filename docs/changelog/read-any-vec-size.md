# VTKDataSetReader handles any Vec size.
    
The legacy VTK file reader previously only supported a specific set of Vec
lengths (i.e., 1, 2, 3, 4, 6, and 9). This is because a basic array handle
has to have the vec length compiled in. However, the new
`ArrayHandleRuntimeVec` feature is capable of reading in any vec-length and
can be leveraged to read in arbitrarily sized vectors in field arrays.
