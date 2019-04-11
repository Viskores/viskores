# Add support for BitFields.

BitFields are:
- Stored in memory using a contiguous buffer of bits.
- Accessible via portals, a la ArrayHandle.
- Portals operate on individual bits or words.
- Operations may be atomic for safe use from concurrent kernels.

The new BitFieldToUnorderedSet device algorithm produces an
ArrayHandle containing the indices of all set bits, in no particular
order.

The new AtomicInterface classes provide an abstraction into bitwise
atomic operations across control and execution environments and are
used to implement the BitPortals.

BitFields may be used as boolean-typed ArrayHandles using the
ArrayHandleBitField adapter. ArrayHandleBitField uses atomic operations to read
and write bits in the BitField, and is safe to use in concurrent code.

For example, a simple worklet that merges two arrays based on a boolean
condition is tested in TestingBitField:

```
class ConditionalMergeWorklet : public vtkm::worklet::WorkletMapField
{
public:
using ControlSignature = void(FieldIn cond,
                              FieldIn trueVals,
                              FieldIn falseVals,
                              FieldOut result);
using ExecutionSignature = _4(_1, _2, _3);

template <typename T>
VTKM_EXEC T operator()(bool cond, const T& trueVal, const T& falseVal) const
{
  return cond ? trueVal : falseVal;
}

};

BitField bits = ...;
auto condArray = vtkm::cont::make_ArrayHandleBitField(bits);
auto trueArray = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(20, 2, NUM_BITS);
auto falseArray = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(13, 2, NUM_BITS);
vtkm::cont::ArrayHandle<vtkm::Id> output;

vtkm::worklet::DispatcherMapField<ConditionalMergeWorklet> dispatcher;
dispatcher.Invoke(condArray, trueArray, falseArray, output);

```
