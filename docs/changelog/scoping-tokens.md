# Scope ExecObjects with Tokens

When VTK-m's `ArrayHandle` was originally designed, it was assumed that the
control environment would run on a single thread. However, multiple users
have expressed realistic use cases in which they would like to control
VTK-m from multiple threads (for example, to control multiple devices).
Consequently, it is important that VTK-m's control classes work correctly
when used simultaneously from multiple threads.

The original `PrepareFor*` methods of `ArrayHandle` returned an object to
be used in the execution environment on a particular device that pointed to
data in the array. The pointer to the data was contingent on the state of
the `ArrayHandle` not changing. The assumption was that the calling code
would immediately use the returned execution environment object and would
not further change the `ArrayHandle` until done with the execution
environment object.

This assumption is broken if multiple threads are running in the control
environment. For example, if one thread has called `PrepareForInput` to get
an execution array portal, the portal or its data could become invalid if
another thread calls `PrepareForOutput` on the same array. Initially one
would think that a well designed program should not share `ArrayHandle`s in
this way, but there are good reasons to need to do so. For example, when
using `vtkm::cont::PartitionedDataSet` where multiple partitions share a
coordinate system (very common), it becomes unsafe to work on multiple
blocks in parallel on different devices.

What we really want is the code to be able to specify more explicitly when
the execution object is in use. Ideally, the execution object itself would
maintain the resources it is using. However, that will not work in this
case since the object has to pass from control to execution environment and
back. The resource allocation will break when the object is passed to an
offloaded device and back.

Because we cannot use the object itself to manage its own resources, we use
a proxy object we are calling a `Token`. The `Token` object manages the
scope of the return execution object. As long as the `Token` is still in
scope, the execution object will remain valid. When the `Token` is
destroyed (or `DetachFromAll` is called on it), then the execution object
is no longer protected.

When a `Token` is attached to an `ArrayHandle` to protect an execution
object, it's read or write mode is recorded. Multiple `Token`s can be
attached to read the `ArrayHandle` at the same time. However, only one
`Token` can be used to write to the `ArrayHandle`.

## Basic `ArrayHandle` use

The basic use of the `PrepareFor*` methods of `ArrayHandle` remain the
same. The only difference is the addition of a `Token` parameter.

``` cpp
template <typename Device>
void LowLevelArray(vtkm::cont::ArrayHandle<vtkm::Float32> array, Device)
{
  vtkm::cont::Token token;
  auto portal = array.PrepareForOutput(ARRAY_SIZE, Device{}, token);
  // At this point, array is locked from anyone else from reading or modifying
  vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(MyKernel(portal), ARRAY_SIZE);

  // When the function finishes, token goes out of scope and array opens up
  // for other uses.
}
```

## Execution objects

To make sure that execution objects are scoped correctly, many changes
needed to be made to propagate a `Token` reference from the top of the
scope to where the execution object is actually made. The most noticeable
place for this was for implementations of
`vtkm::cont::ExecutionObjectBase`. Most implementations of
`ExecutionObjectBase` create an object that requires data from an
`ArrayHandle`.

Previously, a subclass of `ExecutionObjectBase` was expected to have a
method named `PrepareForExecution` that had a single argument: the device
tag (or id) to make an object for. Now, subclasses of `ExecutionObjectBase`
should have a `PrepareForExecution` that takes two arguments: the device
and a `Token` to use for scoping the execution object.

``` cpp
struct MyExecObject : vtkm::cont::ExecutionObjectBase
{
  vtkm::cont::ArrayHandle<vtkm::Float32> Array;
  
  template <typename Device>
  VTKM_CONT
  MyExec<Device> PrepareForExecution(Device device, vtkm::cont::Token& token)
  {
    MyExec<Device> object;
	object.Portal = this->Array.PrepareForInput(device, token);
	return object;
  }
};
```

It actually still works to use the old style of `PrepareForExecution`.
However, you will get a deprecation warning (on supported compilers) when
you try to use it.

## Invoke and Dispatcher

The `Dispatcher` classes now internally define a `Token` object during the
call to `Invoke`. (Likewise, `Invoker` will have a `Token` defined during
its invoke.) This internal `Token` is used when preparing `ArrayHandle`s
and `ExecutionObject`s for the execution environment. (Details in the next
section on how that works.)

Because the invoke uses a `Token` to protect its arguments, it will block
the execution of other worklets attempting to access arrays in a way that
could cause read-write hazards. In the following example, the second
worklet will not be able to execute until the first worklet finishes.

``` cpp
vtkm::cont::Invoker invoke;
invoke(Worklet1{}, input, intermediate);
invoke(Worklet2{}, intermediate, output); // Will not execute until Worklet1 finishes.
```

That said, invocations _can_ share arrays if their use will not cause
read-write hazards. In particular, two invocations can both use the same
array if they are both strictly reading from it. In the following example,
both worklets can potentially execute at the same time.

``` cpp
vtkm::cont::Invoker invoke;
invoke(Worklet1{}, input, output1);
invoke(Worklet2{}, input, output2); // Will not block
```

The same `Token` is used for all arguments to the `Worklet`. This deatil is
important to prevent deadlocks if the same object is used in more than one
`Worklet` parameter. As a simple example, if a `Worklet` has a control
signature like

``` cpp
  using ControlSignature = void(FieldIn, FieldOut);
```

it should continue to work to use the same array as both fields.

``` cpp
vtkm::cont::Invoker invoke;
invoke(Worklet1{}, array, array);
```

## Transport

The dispatch mechanism of worklets internally uses
`vtkm::cont::arg::Transport` objects to automatically move data from the
control environment to the execution environment. These `Transport` object
now take a `Token` when doing the transportation. This all happens under
the covers for most users.

## Control Portals

The `GetPortalConstControl` and `GetPortalControl` methods have been
deprecated. Instead, the methods `ReadPortal` and `WritePortal` should be
used. The calling signature is the same as their predecessors, but the
returned portal contains a reference back to the original `ArrayHandle`.
The reference keeps track of whether the memory allocation has changed.

If the `ArrayHandle` is changed while the `ArrayPortal` still exists,
nothing will happen immediately. However, if the portal is subsequently
accessed (i.e. `Set` or `Get` is called on it), then a fatal error will be
reported to the log.

## Deadlocks

Now that portal objects from `ArrayHandle`s have finite scope (as opposed
to able to be immediately invalidated), the scopes have the ability to
cause operations to block. This can cause issues if the `ArrayHandle` is
attempted to be used by multiple `Token`s at once.

The following is a contrived example of causing a deadlock.

``` cpp
vtkm::cont::Token token1;
auto portal1 = array.PrepareForInPlace(Device{}, token1);

vtkm::cont::Token token2;
auto portal2 = array.PrepareForInput(Device{}, token2);
```

The last line will deadlock as `PrepareForInput` waits for `token1` to
detach, which will never happen. To prevent this from happening, if you use
the same `Token` on the array, it will always allow the action. Thus, the
following will work fine.

``` cpp
vtkm::cont::Token token;

auto portal1 = array.PrepareForInPlace(Device{}, token);
auto portal2 = array.PrepareForInput(Device{}, token);
```

This prevents deadlock during the invocation of a worklet (so long as no
intermediate object tries to create its own `Token`, which would be bad
practice).

Deadlocks are more likely when actually running multiple threads in the
control environment, but still pretty unlikely. One way it can occur is if
you have one (or more) worklet that has two output fields. You then try to
run the worklet(s) simultaneously on multiple threads. It could be that one
thread locks the first output array and the other thread locks the second
output array.

However, having multiple threads trying to write to the same output arrays
at the same time without its own coordination is probably a bad idea in itself.
