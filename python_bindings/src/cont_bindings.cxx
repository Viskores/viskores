//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "viskores_python_bindings.h"

#include <viskores/interop/python/ArrayHandleToNumPy.h>
#include <viskores/interop/python/NumPyToArrayHandle.h>

#include <viskores/cont/UnknownArrayHandle.h>

#include <string>

namespace vip = viskores::interop::python;

namespace viskores::python::bindings
{
namespace
{

std::string UnknownArrayHandleRepr(const viskores::cont::UnknownArrayHandle& self)
{
  return "viskores.cont.UnknownArrayHandle(values=" +
    std::to_string(self.GetNumberOfValues()) +
    ", components=" + std::to_string(self.GetNumberOfComponentsFlat()) + ")";
}

} // namespace

void BindCont(nb::module_& m)
{
  nb::class_<viskores::cont::UnknownArrayHandle>(m, "UnknownArrayHandle")
    .def(nb::init<>())
    .def("__len__", &viskores::cont::UnknownArrayHandle::GetNumberOfValues)
    .def("__repr__", &UnknownArrayHandleRepr)
    .def("GetNumberOfValues",
         &viskores::cont::UnknownArrayHandle::GetNumberOfValues,
         "Number of elements (tuples) in the array.")
    .def("GetNumberOfComponentsFlat",
         &viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat,
         "Number of scalar components per element, counting nested Vec components.")
    .def("NewInstanceBasic",
         &viskores::cont::UnknownArrayHandle::NewInstanceBasic,
         "Allocate a new Viskores-managed array of the same value type.")
    .def("DeepCopyFrom",
         [](viskores::cont::UnknownArrayHandle& self,
            const viskores::cont::UnknownArrayHandle& source) { self.DeepCopyFrom(source); },
         nb::arg("source"),
         "Deep copy data from source into this array, allocating as needed.")
    .def("asnumpy",
         &vip::ArrayHandleToNumPy,
         "Return a read-only NumPy view of this array. Zero-copy for basic and\n"
         "runtime-vec storage; other layouts are copied element-by-element.\n"
         "The view keeps the source array alive for its entire lifetime.");

  m.def("array_from_numpy",
        &vip::NumPyToArrayHandle,
        nb::arg("array"),
        nb::arg("allow_copy") = false,
        "Wrap a NumPy array as a Viskores UnknownArrayHandle. By default the\n"
        "binding shares storage with the NumPy buffer; this requires the input\n"
        "to be C-contiguous, aligned, and writable. Pass allow_copy=True to\n"
        "let the binding make a contiguous, writable copy when the input\n"
        "layout is not directly shareable. allow_copy is permission, not a\n"
        "command: a directly-shareable input still takes the zero-copy path.");
  m.def("asnumpy",
        &vip::ArrayHandleToNumPy,
        nb::arg("array"),
        "Return a read-only NumPy view of a Viskores UnknownArrayHandle.\n"
        "Equivalent to array.asnumpy().");
}

} // namespace viskores::python::bindings
