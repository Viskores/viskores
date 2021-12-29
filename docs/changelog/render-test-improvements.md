# Redesign of Render Regression Tests

The helper functions for creating the render regression tests have been
reformulated. The main changes are outlined here.

## Helper functions are no longer templated

The principle change made is that the `RenderAndRegressionTest` has been
changed to no longer require template arguments (which were used to specify
which rendering components to use). However, using templated arguments
requires each rendering test to entirely recompile the rendering code that
it uses. Since the rendering code currently is itself templated, this leads
to a significant amount of re-compilation.

As a side effect of this, the render helper function is now compiled into a
new library, `vtkm_rendering_testing`. Once again, this allows multiple
tests to use rendering without having to recompile the rendering code.

As part of the change, the name of the `RenderAndRegressionTest` function
has been simplified to `RenderTest`.

## Use a more efficient device

It is common for the testing infrastructure to run the same test multiple
times with different devices. Thus, a filter might be run once with the
Kokkos backend and once with the Serial backend. However, even if the
filter is being tested with the serial backend, there is no real reason to
restrict the rendering to a serial process.

Thus, unless otherwise specified, the rendering will use whatever device is
available regardless of what was requested for the test.

## Consolidate options into a struct

Before these changes, there were several options that could be provided to
the render function, and these changes have added several more. The
previous version of the render function specified each of these options as
arguments to the function. However, that quickly became unwieldy as the
number of options grows. Also, it was impossible to send options to the
image comparison (which is called as a subprocess) such as threshold
values.

## Move general testing methods to library

A side effect of these changes is that some more general testing methods
have been moved to the `vtkm_cont_testing` library. Previously, all methods
in the `vtkm::cont::testing::Testing` class were inlined in the header
file. This makes sense for the methods that are templated, but not so much
for methods that are not templated.

Although this change provides minimal improvements with compile times and
object sizes (maybe). But the real benefit is that some of these methods
declare static objects. When declared in inlined functions, a different
object will be created for each translation unit. This can lead to
unexpected behavior when multiple versions of a supposed singleton static
object exist. In particular, this was causing a failure when the static
objects holding testing directories was created by the test translation
unit but was then unavailable to `vtkm_rendering_testing`.
