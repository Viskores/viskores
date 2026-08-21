==============================
Function Interface Objects
==============================

.. _chap:FunctionInterfaceObjects:

.. index::
   single: function interface

For flexibility's sake, a worklet is free to declare a ``ControlSignature``
with whatever number of arguments are sensible for its operation.
The :class:`viskores::cont::Invoker` is expected to support arguments that
match these arguments, and part of the invocation operation may require these
arguments to be augmented before the worklet is scheduled.
This leaves the invoker with the tricky task of managing some collection of
arguments of unknown size and unknown types.

To simplify this management, |Viskores| has the
:class:`viskores::internal::FunctionInterface` class.
:class:`viskores::internal::FunctionInterface` is a templated class that
manages a generic set of arguments and the return value from a function.
An instance of :class:`viskores::internal::FunctionInterface` holds an
instance of each argument.
You can apply the arguments in a :class:`viskores::internal::FunctionInterface`
object to a functor with a compatible prototype, and the resulting value of
the function call is saved in the
:class:`viskores::internal::FunctionInterface`.

.. doxygenclass:: viskores::internal::FunctionInterface
   :members:


------------------------------
Declaring and Creating
------------------------------

:class:`viskores::internal::FunctionInterface` is a templated class with a
single parameter.
The parameter is the :index:`function signature <signature>` of the function.
A signature is a function type.
The syntax in C++ is the return type followed by the argument types enclosed
in parentheses.

.. load-example:: DefineFunctionInterface
   :file: GuideExampleFunctionInterface.cxx
   :caption: Declaring :class:`viskores::internal::FunctionInterface`.

The :func:`viskores::internal::make_FunctionInterface` function provides an
easy way to create a :class:`viskores::internal::FunctionInterface` and
initialize the state of all the parameters.
:func:`viskores::internal::make_FunctionInterface` takes a variable number of
arguments, one for each parameter.
Since the return type is not specified as an argument, you must always specify
it as a template parameter.

.. doxygenfunction:: viskores::internal::make_FunctionInterface

.. load-example:: UseMakeFunctionInterface
   :file: GuideExampleFunctionInterface.cxx
   :caption: Using :func:`viskores::internal::make_FunctionInterface`.


------------------------------
Parameters
------------------------------

Once created, :class:`viskores::internal::FunctionInterface` contains methods
to query and manage the parameters and objects associated with them.
The number of parameters can be retrieved either with the constant field
:member:`viskores::internal::FunctionInterface::ARITY` or with the
:func:`viskores::internal::FunctionInterface::GetArity` method.

.. load-example:: FunctionInterfaceArity
   :file: GuideExampleFunctionInterface.cxx
   :caption: Getting the arity of a :class:`viskores::internal::FunctionInterface`.

You can use the :func:`viskores::internal::ParameterGet` function to retrieve
a parameter from a :class:`viskores::internal::FunctionInterface`.
When using :func:`viskores::internal::ParameterGet`, you have to specify the
index of the parameter using a template argument.
Note that the parameters in :class:`viskores::internal::FunctionInterface`
start at index 1.
Although this is uncommon in C++, it is customary to number function arguments
starting at 1.

.. doxygenfunction:: viskores::internal::ParameterGet

.. load-example:: FunctionInterfaceGetParameter
   :file: GuideExampleFunctionInterface.cxx
   :caption: Using :func:`viskores::internal::ParameterGet`.


------------------------------
Transformations
------------------------------

Rather than replace a single item in a
:class:`viskores::internal::FunctionInterface`, it is desirable to change them
all in a similar way.
:class:`viskores::internal::FunctionInterface` supports a *static transform*
that replaces all of the arguments with new types defined at compile time.

.. index::
   single: function interface; static transform

The static transform method,
:func:`viskores::internal::FunctionInterface::StaticTransformCont`, operates
by accepting a functor that defines a function with two arguments.
The first argument is the :class:`viskores::internal::FunctionInterface`
parameter to transform.
The second argument is an instance of the
:struct:`viskores::internal::IndexTag` templated class that statically
identifies the parameter index being transformed.
An :struct:`viskores::internal::IndexTag` object has no state, but the class
contains a static integer named :member:`viskores::internal::IndexTag::INDEX`.
The function returns the transformed argument.

.. doxygenstruct:: viskores::internal::IndexTag
   :members:

The functor must also contain a templated class named ``ReturnType`` with an
internal type named ``type`` that defines the return type of the transform for
a given parameter type.
``ReturnType`` must have two template parameters.
The first template parameter is the type of the
:class:`viskores::internal::FunctionInterface` parameter to transform.
It is the same type as passed to the operator.
The second template parameter is a :type:`viskores::IdComponent` specifying
the index.

The transformation is only applied to the parameters of the function.
The return argument is unaffected.

The return type can be determined with the
:type:`viskores::internal::FunctionInterface::StaticTransformType` template in
the :class:`viskores::internal::FunctionInterface` class.
:type:`viskores::internal::FunctionInterface::StaticTransformType` has a
single parameter that is the transform functor and contains a type named
``type`` that is the transformed
:class:`viskores::internal::FunctionInterface`.

In the following example, a static transform is used to convert a
:class:`viskores::internal::FunctionInterface` to a new object that has the
pointers to the parameters rather than the values themselves.
The parameter index is always ignored because all parameters are uniformly
transformed.

.. load-example:: FunctionInterfaceStaticTransform
   :file: GuideExampleFunctionInterface.cxx
   :caption: Using a static transform of the function interface class.
