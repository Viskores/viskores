//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "viskores_python_bindings.h"

#include <viskores/Bounds.h>

#include <nanobind/stl/string.h>

#include <string>

namespace
{

std::string RangeRepr(const viskores::Range& r)
{
  return "viskores.Range(Min=" + std::to_string(r.Min) + ", Max=" + std::to_string(r.Max) + ")";
}

std::string BoundsRepr(const viskores::Bounds& b)
{
  return "viskores.Bounds(X=" + RangeRepr(b.X) + ", Y=" + RangeRepr(b.Y) +
    ", Z=" + RangeRepr(b.Z) + ")";
}

} // namespace

NB_MODULE(_viskores, m)
{
  m.doc() = "Minimal manual Viskores Python bindings.";

  nb::class_<viskores::Range>(m, "Range")
    .def(nb::init<>())
    .def(nb::init<viskores::Float64, viskores::Float64>(), nb::arg("min"), nb::arg("max"))
    .def_rw("Min", &viskores::Range::Min)
    .def_rw("Max", &viskores::Range::Max)
    .def("IsNonEmpty", &viskores::Range::IsNonEmpty)
    .def("Length", &viskores::Range::Length)
    .def("Center", &viskores::Range::Center)
    .def("__repr__", &RangeRepr);

  nb::class_<viskores::Bounds>(m, "Bounds")
    .def(nb::init<>())
    .def(nb::init<viskores::Range, viskores::Range, viskores::Range>(),
         nb::arg("x"),
         nb::arg("y"),
         nb::arg("z"))
    .def_rw("X", &viskores::Bounds::X)
    .def_rw("Y", &viskores::Bounds::Y)
    .def_rw("Z", &viskores::Bounds::Z)
    .def("IsNonEmpty", &viskores::Bounds::IsNonEmpty)
    .def("__repr__", &BoundsRepr);

  nb::module_ cont = m.def_submodule("cont");
  viskores::python::bindings::BindCont(cont);
}
