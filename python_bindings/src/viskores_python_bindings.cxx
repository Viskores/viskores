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

#include <sstream>
#include <string>

namespace
{

std::string RangeRepr(const viskores::Range& r)
{
  std::ostringstream out;
  out << "viskores.Range" << r;
  return out.str();
}

std::string BoundsRepr(const viskores::Bounds& b)
{
  std::ostringstream out;
  out << "viskores.Bounds" << b;
  return out.str();
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
    .def("Center",
         [](const viskores::Bounds& self)
         {
           viskores::Vec3f_64 center = self.Center();
           return nb::make_tuple(center[0], center[1], center[2]);
         },
         "Return the center of the bounds as an (x, y, z) tuple.")
    .def("__repr__", &BoundsRepr);

  nb::module_ cont = m.def_submodule("cont");
  viskores::python::bindings::BindCont(cont);
}
