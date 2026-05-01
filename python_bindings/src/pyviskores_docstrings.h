//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef PYVISKORES_DOCSTRINGS_H
#define PYVISKORES_DOCSTRINGS_H

#include <string_view>

namespace viskores::python::bindings
{
namespace doc
{

const char* GeneratedClassDoc(std::string_view name);

inline const char* ClassDoc(std::string_view name)
{
  if (const char* doc = GeneratedClassDoc(name))
  {
    return doc;
  }

  return R"doc(Viskores Python binding.

This object wraps a Viskores C++ class for use from Python.)doc";
}

inline constexpr const char* Initialize =
  R"doc(Initialize the Viskores runtime and select a device adapter.

Call this once early in a process or notebook kernel when you need to choose a
specific backend. The argv overload accepts Viskores command-line options and
updates the list in place with options consumed by Viskores removed.)doc";

inline constexpr const char* IsInitialized =
  R"doc(Return True if the Viskores runtime has already been initialized.)doc";

inline constexpr const char* MakeDeviceAdapterId =
  R"doc(Create a DeviceAdapterId from a device adapter name or numeric id.)doc";

inline constexpr const char* ArrayFromNumPy =
  R"doc(Create a Viskores UnknownArrayHandle from a NumPy-compatible array.

When copy is False and the input layout is compatible, the Viskores array can
share ownership with the Python array object.)doc";

inline constexpr const char* AsNumPy =
  R"doc(Return a NumPy view or copy for a Viskores array-like object.

The object may be an UnknownArrayHandle, a supported concrete Viskores array, a
Field, or any object accepted by numpy.asarray. Variable-length grouped arrays
are returned as a list of NumPy arrays.)doc";

inline constexpr const char* ArrayCopy =
  R"doc(Copy values between a Viskores array-like source and a Python destination.)doc";

inline constexpr const char* MakeField =
  R"doc(Create a Viskores Field from a name, association, and Python array-like values.)doc";

inline constexpr const char* MakeFieldPoint =
  R"doc(Create a point-associated Viskores Field from Python array-like values.)doc";

inline constexpr const char* MakeFieldCell =
  R"doc(Create a cell-associated Viskores Field from Python array-like values.)doc";

inline constexpr const char* CreateUniformDataSet =
  R"doc(Create a uniform structured DataSet from dimensions, origin, and spacing.)doc";

inline constexpr const char* ExecuteFilter =
  R"doc(Execute the filter on a DataSet or PartitionedDataSet and return the matching result type.)doc";

inline constexpr const char* ExecuteSource = R"doc(Generate and return the source DataSet.)doc";

inline constexpr const char* SetActiveField =
  R"doc(Select the active input field by name and optional association.)doc";

inline constexpr const char* SetFieldsToPass =
  R"doc(Select which input fields are passed through to the output.)doc";

inline constexpr const char* TransferToOpenGL =
  R"doc(Transfer Viskores data into an OpenGL buffer state for interoperation.)doc";

} // namespace doc
} // namespace viskores::python::bindings

#endif
