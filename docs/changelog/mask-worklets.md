# Allow masking of worklet invocations

There have recently been use cases where it would be helpful to mask out
some of the invocations of a worklet. The idea is that when invoking a
worklet with a mask array on the input domain, you might implement your
worklet more-or-less like the following.

```cpp
VTKM_EXEC void operator()(bool mask, /* other parameters */)
{
  if (mask)
  {
    // Do interesting stuff
  }
}
```

This works, but what if your mask has mostly false values? In that case,
you are spending tons of time loading data to and from memory where fields
are stored for no reason.

You could potentially get around this problem by adding a scatter to the
worklet. However, that will compress the output arrays to only values that
are active in the mask. That is problematic if you want the masked output
in the appropriate place in the original arrays. You will have to do some
complex (and annoying and possibly expensive) permutations of the output
arrays.

Thus, we would like a new feature similar to scatter that instead masks out
invocations so that the worklet is simply not run on those outputs.

## New Interface

The new "Mask" feature that is similar (and orthogonal) to the existing
"Scatter" feature. Worklet objects now define a `MaskType` that provides on
object that manages the selections of which invocations are skipped. The
following Mask objects are defined.

  * `MaskNone` - This removes any mask of the output. All outputs are
    generated. This is the default if no `MaskType` is explicitly defined.
  * `MaskSelect` - The user to provides an array that specifies whether
    each output is created with a 1 to mean that the output should be
    created an 0 the mean that it should not.
  * `MaskIndices` - The user provides an array with a list of indices for
    all outputs that should be created.
  
It will be straightforward to implement other versions of masks. (For
example, you could make a mask class that selectes every Nth entry.) Those
could be made on an as-needed basis.

## Implementation

The implementation follows the same basic idea of how scatters are
implemented.

### Mask Classes

The mask class is required to implement the following items.

  * `ThreadToOutputType` - A type for an array that maps a thread index (an
    index in the array) to an output index. A reasonable type for this
    could be `vtkm::cont::ArrayHandle<vtkm::Id>`.
  * `GetThreadToOutputMap` - Given the range for the output (e.g. the
    number of items in the output domain), returns an array of type
    `ThreadToOutputType` that is the actual map.
  * `GetThreadRange` - Given a range for the output (e.g. the number of
    items in the output domain), returns the range for the threads (e.g.
    the number of times the worklet will be invoked).

### Dispatching

The `vtkm::worklet::internal::DispatcherBase` manages a mask class in
the same way it manages the scatter class. It gets the `MaskType` from
the worklet it is templated on. It requires a `MaskType` object during
its construction.

Previously the dispatcher (and downstream) had to manage the range and
indices of inputs and threads. They now have to also manage a separate
output range/index as now all three may be different.

The `vtkm::Invocation` is changed to hold the ThreadToOutputMap array from
the mask. It likewises has a templated `ChangeThreadToOutputMap` method
added (similar to those already existing for the arrays from a scatter).
This method is used in `DispatcherBase::InvokeTransportParameters` to add
the mask's array to the invocation before calling `InvokeSchedule`.

### Thread Indices

With the addition of masks, the `ThreadIndices` classes are changed to
manage the actual output index. Previously, the output index was always the
same as the thread index. However, now these two can be different. The
`GetThreadIndices` methods of the worklet base classes have an argument
added that is the portal to the ThreadToOutputMap.

The worklet `GetThreadIndices` is called from the `Task` classes. These
classes are changed to pass in this additional argument. Since the `Task`
classes get an `Invocation` object from the dispatcher, which contains the
`ThreadToOutputMap`, this change is trivial.

## Interaction Between Mask and Scatter

Although it seems weird, it should work fine to mix scatters and masks. The
scatter will first be applied to the input to generate a (potential) list
of output elements. The mask will then be applied to these output elements.
