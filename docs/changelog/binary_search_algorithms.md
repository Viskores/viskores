# Algorithms for Control and Execution Environments

The `<vtkm/Algorithms.h>` header has been added to provide common STL-style 
generic algorithms that are suitable for use in both the control and execution 
environments. This is necessary as the STL algorithms in the `<algorithm>` 
header are not marked up for use in execution environments such as CUDA. 

In addition to the markup, these algorithms have convenience overloads to 
support ArrayPortals directly, simplifying their usage with VTK-m data 
structures.

Currently, three related algorithms are provided: `LowerBounds`, `UpperBounds`,
and `BinarySearch`. `BinarySearch` differs from the STL `std::binary_search` 
algorithm in that it returns an iterator (or index) to a matching element,
rather than just a boolean indicating whether a or not key is present.

The new algorithm signatures are:

```c++
namespace vtkm
{

template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT 
IterT BinarySearch(IterT first, IterT last, const T& val, Comp comp);

template <typename IterT, typename T>
VTKM_EXEC_CONT 
IterT BinarySearch(IterT first, IterT last, const T& val);

template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT 
vtkm::Id BinarySearch(const PortalT& portal, const T& val, Comp comp);

template <typename PortalT, typename T>
VTKM_EXEC_CONT 
vtkm::Id BinarySearch(const PortalT& portal, const T& val);

template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT 
IterT LowerBound(IterT first, IterT last, const T& val, Comp comp);

template <typename IterT, typename T>
VTKM_EXEC_CONT 
IterT LowerBound(IterT first, IterT last, const T& val);

template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT 
vtkm::Id LowerBound(const PortalT& portal, const T& val, Comp comp);

template <typename PortalT, typename T>
VTKM_EXEC_CONT 
vtkm::Id LowerBound(const PortalT& portal, const T& val);

template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT 
IterT UpperBound(IterT first, IterT last, const T& val, Comp comp);

template <typename IterT, typename T>
VTKM_EXEC_CONT 
IterT UpperBound(IterT first, IterT last, const T& val);

template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT 
vtkm::Id UpperBound(const PortalT& portal, const T& val, Comp comp);

template <typename PortalT, typename T>
VTKM_EXEC_CONT 
vtkm::Id UpperBound(const PortalT& portal, const T& val);

}
```
