# Redesign of ArrayHandle to access data using typeless buffers

The original implementation of `ArrayHandle` is meant to be very generic.
To define an `ArrayHandle`, you actually create a `Storage` class that
maintains the data and provides portals to access it (on the host). Because
the `Storage` can provide any type of data structure it wants, you also
need to define an `ArrayTransfer` that describes how to move the
`ArrayHandle` to and from a device. It also has to be repeated for every
translation unit that uses them.

This is a very powerful mechanism. However, one of the major problems with
this approach is that every `ArrayHandle` type needs to have a separate
compile path for every value type crossed with every device. Because of
this limitation, the `ArrayHandle` for the basic storage has a special
implementation that manages the actual data allocation and movement as
`void *` arrays. In this way all the data management can be compiled once
and put into the `vtkm_cont` library. This has dramatically improved the
VTK-m compile time.

This new design replicates the basic `ArrayHandle`'s success to all other
storage types. The basic idea is to make the implementation of
`ArrayHandle` storage slightly less generic. Instead of requiring it to
manage the data it stores, it instead just builds `ArrayPortal`s from
`void` pointers that it is given. The management of `void` pointers can be
done in non-templated classes that are compiled into a library.

This initial implementation does not convert all `ArrayHandle`s to avoid
making non-backward compatible changes before the next minor revision of
VTK-m. In particular, it would be particularly difficult to convert
`ArrayHandleVirtual`. It could be done, but it would be a lot of work for a
class that will likely be removed.

## Buffer

Key to these changes is the introduction of a
`vtkm::cont::internal::Buffer` object. As the name implies, the `Buffer`
object manages a single block of bytes. `Buffer` is agnostic to the type of
data being stored. It only knows the length of the buffer in bytes. It is
responsible for allocating space on the host and any devices as necessary
and for transferring data among them. (Since `Buffer` knows nothing about
the type of data, a precondition of VTK-m would be that the host and all
devices have to have the same endian.)

The idea of the `Buffer` object is similar in nature to the existing
`vtkm::cont::internal::ExecutionArrayInterfaceBasicBase` except that it
will manage a buffer of data among the control and all devices rather than
in one device through a templated subclass.

As explained below, `ArrayHandle` holds some fixed number of `Buffer`
objects. (The number can be zero for implicit `ArrayHandle`s.) Because all
the interaction with the devices happen through `Buffer`, it will no longer
be necessary to compile any reference to `ArrayHandle` for devices (e.g.
you won’t have to use nvcc just because the code links `ArrayHandle.h`).

## Storage

The `vtkm::cont::internal::Storage` class changes dramatically. Although an
instance will be kept, the intention is for `Storage` itself to be a
stateless object. It will manage its data through `Buffer` objects provided
from the `ArrayHandle`.

That said, it is possible for `Storage` to have some state. For example,
the `Storage` for `ArrayHandleImplicit` must hold on to the instance of the
portal used to manage the state.


## ArrayTransport

The `vtkm::cont::internal::ArrayTransfer` class will be removed completely.
All data transfers will be handled internally with the `Buffer` object

## Portals

A big change for this design is that the type of a portal for an
`ArrayHandle` will be the same for all devices and the host. Thus, we no
longer need specialized versions of portals for each device. We only have
one portal type. And since they are constructed from `void *` pointers, one
method can create them all.


## Advantages

The `ArrayHandle` interface should not change significantly for external
uses, but this redesign offers several advantages.

### Faster Compiles

Because the memory management is contained in a non-templated `Buffer`
class, it can be compiled once in a library and used by all template
instances of `ArrayHandle`. It should have similar compile advantages to
our current specialization of the basic `ArrayHandle`, but applied to all
types of `ArrayHandle`s.

### Fewer Templates

Hand-in-hand with faster compiles, the new design should require fewer
templates and template instances. We have immediately gotten rid of
`ArrayTransport`. `Storage` is also much shorter. Because all
`ArrayPortal`s are the same for every device and the host, we need many
fewer versions of those classes. In the device adapter, we can probably
collapse the three `ArrayManagerExecution` classes into a single, much
simpler class that does simple memory allocation and copy.

### Fewer files need to be compiled for CUDA

Including `ArrayHandle.h` no longer adds code that compiles for a device.
Thus, we should no longer need to compile for a specific device adapter
just because we access an `ArrayHandle`. This should make it much easier to
achieve our goal of a "firewall". That is, code that just calls VTK-m
filters does not need to support all its compilers and flags.

### Simpler ArrayHandle specialization

The newer code should simplify the implementation of special `ArrayHandle`s
a bit. You need only implement an `ArrayPortal` that operates on one or
more `void *` arrays and a simple `Storage` class.

### Out of band memory sharing

With the current version of `ArrayHandle`, if you want to take data from
one `ArrayHandle` you pretty much have to create a special template to wrap
another `ArrayHandle` around that. With this new design, it is possible to
take data from one `ArrayHandle` and give it to another `ArrayHandle` of a
completely different type. You can’t do this willy-nilly since different
`ArrayHandle` types will interpret buffers differently. But there can be
some special important use cases.

One such case could be an `ArrayHandle` that provides strided access to a
buffer. (Let’s call it `ArrayHandleStride`.) The idea is that it interprets
the buffer as an array for a particular type (like a basic `ArrayHandle`)
but also defines a stride, skip, and repeat so that given an index it looks
up the value `((index / skip) % repeat) * stride`. The point is that it can
take an AoS array of tuples and represent an array of one of the
components.

The point would be that if you had a `VariantArrayHandle` or `Field`, you
could pull out an array of one of the components as an `ArrayHandleStride`.
An `ArrayHandleStride<vtkm::Float32>` could be used to represent that data
that comes from any basic `ArrayHandle` with `vtkm::Float32` or a
`vtkm::Vec` of that type. It could also represent data from an
`ArrayHandleCartesianProduct` and `ArrayHandleSoA`. We could even represent
an `ArrayHandleUniformPointCoordinates` by just making a small array. This
allows us to statically access a whole bunch of potential array storage
classes with a single type.

### Potentially faster device transfers

There is currently a fast-path for basic `ArrayHandle`s that does a block
cuda memcpy between host and device. But for other `ArrayHandle`s that do
not defer their `ArrayTransfer` to a sub-array, the transfer first has to
copy the data into a known buffer.

Because this new design stores all data in `Buffer` objects, any of these
can be easily and efficiently copied between devices.

## Disadvantages

This new design gives up some features of the original `ArrayHandle` design.

### Can only interface data that can be represented in a fixed number of buffers

Because the original `ArrayHandle` design required the `Storage` to
completely manage the data, it could represent it in any way possible. In
this redesign, the data need to be stored in some fixed number of memory
buffers.

This is a pretty open requirement. I suspect most data formats will be
storable in this. The user’s guide has an example of data stored in a
`std::deque` that will not be representable. But that is probably not a
particularly practical example.

### VTK-m would only be able to support hosts and devices with the same endian

Because data are transferred as `void *` blocks of memory, there is no way
to correct words if the endian on the two devices does not agree. As far as
I know, there should be no issues with the proposed ECP machines.

If endian becomes an issue, it might be possible to specify a word length
in the `Buffer`. That would assume that all numbers stored in the `Buffer`
have the same word length.

### ArrayPortals must be completely recompiled in each translation unit

We can declare that an `ArrayHandle` does not need to include the device
adapter header files in part because it no longer needs specialized
`ArrayPortal`s for each device. However, that means that a translation unit
compiled with the host compiler (say gcc) will produce different code for
the `ArrayPortal`s than those with the device compiler (say nvcc). This
could lead to numerous linking problems.

To get around these issues, we will probably have to enforce no exporting
of any of the `ArrayPotal` symbols and force them all to be recompiled for
each translation unit. This will serve to increase the compile times a bit.
We will probably also still encounter linking errors as there would be no
way to enforce this requirement.

### Cannot have specialized portals for the control environment

Because the new design unifies `ArrayPortal` types across control and
execution environments, it is no longer possible to have a special version
for the control environment to manage resources. This will require removing
some recent behavior of control portals such as with MR !1988.
