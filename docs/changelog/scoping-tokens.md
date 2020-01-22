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
would next use the returned execution environment object and would not
further change the `ArrayHandle` until done with the execution environment
object.

This assumption is broken if multiple threads are running in the control
environment. After one thread has called `PrepareFor*` and is in the
process of using the resulting execution object and another thread attempts
to write to or otherwise change the same array. Perhaps a well designed
program should not share `ArrayHandle`s in this way, but if a mistake is
made that would lead to a very difficult to diagnose intermittent error.

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

## Transport

The dispatch mechanism of worklets internally uses
`vtkm::cont::arg::Transport` objects to automatically move data from the
control environment to the execution environment. These `Transport` object
now take a `Token` when doing the transportation. This all happens under
the covers for most users.

## Control Portals

The calling signatures of `GetPortalControl` and `GetPortalConstControl`
have not changed. That is, they do not require a `Token` object. This is
because these are control-only objects and so the `Token` is embedded
within the return portal object.

The advantage is that the returned portal will always be valid. However, it
is now the case that a control portal can prevent something else from
running. This means that control portals should drop scope as soon as
possible.

## Deadlocks

Now that portal objects from `ArrayHandle`s have finite scope (as opposed
to able to be immediately invalidated), the scopes have the ability to
cause operations to block. This can cause issues if the `ArrayHandle` is
attempted to be used by multiple `Token`s at once.

Care should be taken to ensure that a single thread does not attempt to use
an `ArrayHandle` two ways at the same time.

``` cpp
auto portal = array.GetPortalControl();
for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
{
  portal.Set(index, /* An interesting value */);
}
vtkm::cont::Invoker invoke;
invoke(MyWorklet, array); // Oops. Deadlock here.
```

In this example, the last line deadlocks because `portal` is still holding
onto `array` for writing. When the worklet is invoked, it waits for
everything to stop writing to `array` so that it can be safely be read.
Instead, `portal` should be properly scoped.

``` cpp
{
  auto portal = array.GetPortalControl();
  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    portal.Set(index, /* An interesting value */);
  }
}
vtkm::cont::Invoker invoke;
invoke(MyWorklet, array); // Runs fine because portal left scope
```
