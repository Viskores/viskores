# DeviceAdapter Reduction supports differing input and output types

It is common to want to perform a reduction where the input and output types
are of differing types. A basic example would be when the input is `vtkm::UInt8`
but the output is `vtkm::UInt64`. This has been supported since v1.2, as the input
type can be implicitly convertible to the output type.

What we now support is when the input type is not implicitly convertible to the output type,
such as when the output type is `vtkm::Pair< vtkm::UInt64, vtkm::UInt64>`. For this to work
we require that the custom binary operator implements also an `operator()` which handles
the unary transformation of input to output. 

An example of a custom reduction operator for differing input and output types is:

```cxx

  struct CustomMinAndMax
  {
    using OutputType = vtkm::Pair<vtkm::Float64, vtkm::Float64>;

    VTKM_EXEC_CONT
    OutputType operator()(vtkm::Float64 a) const
    {
    return OutputType(a, a);
    }

    VTKM_EXEC_CONT
    OutputType operator()(vtkm::Float64 a, vtkm::Float64 b) const
    {
      return OutputType(vtkm::Min(a, b), vtkm::Max(a, b));
    }

    VTKM_EXEC_CONT
    OutputType operator()(const OutputType& a, const OutputType& b) const
    {
      return OutputType(vtkm::Min(a.first, b.first), vtkm::Max(a.second, b.second));
    }

    VTKM_EXEC_CONT
    OutputType operator()(vtkm::Float64 a, const OutputType& b) const
    {
      return OutputType(vtkm::Min(a, b.first), vtkm::Max(a, b.second));
    }

    VTKM_EXEC_CONT
    OutputType operator()(const OutputType& a, vtkm::Float64 b) const
    {
      return OutputType(vtkm::Min(a.first, b), vtkm::Max(a.second, b));
    }
  };


```
