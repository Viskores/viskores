//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_functions_h
#define vtk_m_worklet_zfp_functions_h

#include <vtkm/Math.h>
#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPCodec.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

template <typename T>
void PrintBits(T bits)
{
  const int bit_size = sizeof(T) * 8;
  for (int i = bit_size - 1; i >= 0; --i)
  {
    T one = 1;
    T mask = one << i;
    int val = (bits & mask) >> T(i);
    printf("%d", val);
  }
  printf("\n");
}

template <typename T>
inline vtkm::UInt32 MinBits(const vtkm::UInt32 bits)
{
  return bits;
}

template <>
inline vtkm::UInt32 MinBits<vtkm::Float32>(const vtkm::UInt32 bits)
{
  return vtkm::Max(bits, 1 + 8u);
}

template <>
inline vtkm::UInt32 MinBits<vtkm::Float64>(const vtkm::UInt32 bits)
{
  return vtkm::Max(bits, 1 + 11u);
}




} // namespace zfp
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_type_info_h
