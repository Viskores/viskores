//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

namespace viskores::python::bindings
{

#if VISKORES_PYTHON_ENABLE_INTEROP
void TransferUnknownArrayToOpenGL(const viskores::cont::UnknownArrayHandle& array,
                                  viskores::interop::BufferState& state)
{
  using InteropValueTypes =
    viskores::List<viskores::UInt8,
                   viskores::Int32,
                   viskores::Int64,
                   viskores::Float32,
                   viskores::Float64,
                   viskores::Vec3f_32,
                   viskores::Vec3f_64,
                   viskores::Vec4ui_8,
                   viskores::Vec4f_32,
                   viskores::Vec4f_64>;
  using InteropStorageTypes =
    viskores::List<viskores::cont::StorageTagBasic,
                   viskores::cont::StorageTagUniformPoints,
                   viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                                              viskores::cont::StorageTagBasic,
                                                              viskores::cont::StorageTagBasic>,
                   viskores::cont::StorageTagSOAStride>;
  array.CastAndCallForTypes<InteropValueTypes, InteropStorageTypes>(
    [&](const auto& concreteArray)
    {
      viskores::interop::TransferToOpenGL(concreteArray, state);
    });
}
#endif

} // namespace viskores::python::bindings
