# A `ScanExtended` device algorithm has been added.

This new scan algorithm produces an array that contains both an inclusive scan
and an exclusive scan in the same array:

```
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayGetValue.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleView.h>

vtkm::cont::ArrayHandle<T> inputData = ...;
const vtkm::Id size = inputData.GetNumberOfValues();

vtkm::cont::ArrayHandle<T> extendedScan;
vtkm::cont::Algorithm::ScanExtended(inputData, extendedScan);

// The exclusive scan is the first `inputSize` values starting at index 0:
auto exclusiveScan = vtkm::cont::make_ArrayHandleView(extendedScan, 0, size);

// The inclusive scan is the first `inputSize` values starting at index 1:
auto inclusiveScan = vtkm::cont::make_ArrayHandleView(extendedScan, 1, size);

// The total sum of the input data is the last value in the extended scan.
const T totalSum = vtkm::cont::ArrayGetValue(size, extendedScan);
```

This can also be thought of as an exclusive scan that appends the total sum,
rather than returning it.
