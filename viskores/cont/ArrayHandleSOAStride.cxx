//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define viskores_cont_ArrayHandleSOAStride_cxx
#include <viskores/cont/ArrayHandleSOAStride.h>

#include <viskores/cont/UnknownArrayHandle.h>

namespace viskores
{
namespace cont
{

#define VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(Type)                                       \
  template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Vec<Type, 2>, StorageTagSOAStride>; \
  template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Vec<Type, 3>, StorageTagSOAStride>; \
  template class VISKORES_CONT_EXPORT ArrayHandle<viskores::Vec<Type, 4>, StorageTagSOAStride>;

VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(char)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::Int8)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::UInt8)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::Int16)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::UInt16)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::Int32)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::UInt32)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::Int64)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::UInt64)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::Float32)
VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE(viskores::Float64)

#undef VISKORES_ARRAYHANDLE_SOA_STRIDE_INSTANTIATE
}
} // end viskores::cont
