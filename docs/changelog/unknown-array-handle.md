# UnknownArrayHandle and UncertainArrayHandle for runtime-determined types

Two new classes have been added to VTK-m: `UnknownArrayHandle` and
`UncertainArrayHandle`. These classes serve the same purpose as the set of
`VariantArrayHandle` classes and will replace them.

Motivated mostly by the desire to move away from `ArrayHandleVirtual`, we
have multiple reasons to completely refactor the `VariantArrayHandle`
class. These include changing the implementation, some behavior, and even
the name.

## Motivation

We have several reasons that have accumulated to revisit the implementation
of `VariantArrayHandle`.

### Move away from `ArrayHandleVirtual`

The current implementation of `VariantArrayHandle` internally stores the
array wrapped in an `ArrayHandleVirtual`. That makes sense since you might
as well consolidate the hierarchy of virtual objects into one.

Except `ArrayHandleVirtual` is being deprecated, so it no longer makes
sense to use that internally.

So we will transition the class back to managing the data as typeless on
its own. We will consider using function pointers rather than actual
virtual functions because compilers can be slow in creating lots of virtual
subclasses.

### Reintroduce storage tag lists

The original implementation of `VariantArrayHandle` (which at the time was
called `DynamicArrayHandle`) actually had two type lists: one for the array
value type and one for the storage type. The storage type list was removed
soon after `ArrayHandleVirtual` was introduced because whatever the type of
array it could be access as `ArrayHandleVirtual`.

However, with `ArrayHandleVirtual` being deprecated, this feature is no
longer possible. We are in need again for the list of storage types to try.
Thus, we need to reintroduce this template argument to
`VariantArrayHandle`.

### More clear name

The name of this class has always been unsatisfactory. The first name,
`DynamicArrayHandle`, makes it sound like the data is always changing. The
second name, `VariantArrayHandle`, makes it sound like an array that holds
a value type that can vary (like an `std::variant`).

We can use a more clear name that expresses better that it is holding an
`ArrayHandle` of an _unknown_ type.

### Take advantage of default types for less templating

Once upon a time everything in VTK-m was templated header library. Things
have changed quite a bit since then. The most recent development is the
ability to select the "default types" with CMake configuration that allows
you to select a global set of types you care about during compilation. This
is so units like filters can be compiled into a library with all types we
care about, and we don't have to constantly recompile units.

This means that we are becoming less concerned about maintaining type lists
everywhere. Often we can drop the type list and pass data across libraries.

With that in mind, it makes less sense for `VariantArrayHandle` to actually
be a `using` alias for `VariantArrayHandleBase<VTKM_DEFAULT_TYPE_LIST>`.

In response, we can revert the is-a relationship between the two. Have a
completely typeless version as the base class and have a second version
templated version to express when the type of the array has been partially
narrowed down to given type lists.

## New Name and Structure

The ultimate purpose of this class is to store an `ArrayHandle` where the
value and storage types are unknown. Thus, an appropriate name for the
class is `UnknownArrayHandle`.

`UnknownArrayHandle` is _not_ templated. It simply stores an `ArrayHandle`
in a typeless (`void *`) buffer. It does, however, contain many templated
methods that allow you to query whether the contained array matches given
types, to cast to given types, and to cast and call to a given functor
(from either given type lists or default lists).

Rather than have a virtual class structure to manage the typeless array,
the new management will use function pointers. This has shown to sometimes
improve compile times and generate less code.

Sometimes it is the case that the set of potential types can be narrowed. In
this case, the array ceases to be unknown and becomes _uncertain_. Thus,
the companion class to `UnknownArrayHandle` is `UncertainArrayHandle`.

`UncertainArrayHandle` has two template parameters: a list of potential
value types and a list of potential storage types. The behavior of
`UncertainArrayHandle` matches that of `UnknownArrayHandle` (and might
inherit from it). However, for `CastAndCall` operations, it will use the
type lists defined in its template parameters.

## Serializing UnknownArrayHandle

Because `UnknownArrayHandle` is not templated, it contains some
opportunities to compile things into the `vtkm_cont` library. Templated
methods like `CastAndCall` cannot be, but the specializations of DIY's
serialize can be.

And since it only has to be compiled once into a library, we can spend some
extra time compiling for more types. We don't have to restrict ourselves to
`VTKM_DEFAULT_TYPE_LIST`. We can compile for vtkm::TypeListTagAll.
