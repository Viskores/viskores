//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_tool_h
#define vtk_m_worklet_zfp_tool_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/AtomicArray.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/worklet/zfp/ZFPEncode3.h>

using ZFPWord = vtkm::UInt64;

#include <stdio.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{
namespace detail
{

class MemTransfer : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  MemTransfer() {}
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename PortalType>
  VTKM_EXEC void operator()(const vtkm::Id id, PortalType& outValue) const
  {
    (void)id;
    (void)outValue;
  }
}; //class MemTransfer

inline size_t CalcMem3d(const vtkm::Id3 dims, const vtkm::UInt32 bits_per_block)
{
  const size_t vals_per_block = 64;
  const size_t size = static_cast<size_t>(dims[0] * dims[1] * dims[2]);
  size_t total_blocks = size / vals_per_block;
  const size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}

inline size_t CalcMem2d(const vtkm::Id2 dims, const vtkm::UInt32 bits_per_block)
{
  constexpr size_t vals_per_block = 16;
  const size_t size = static_cast<size_t>(dims[0] * dims[1]);
  size_t total_blocks = size / vals_per_block;
  constexpr size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}

inline size_t CalcMem1d(const vtkm::Id dims, const vtkm::UInt32 bits_per_block)
{
  constexpr size_t vals_per_block = 4;
  const size_t size = static_cast<size_t>(dims);
  size_t total_blocks = size / vals_per_block;
  constexpr size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}


template <typename T>
T* GetVTKMPointer(vtkm::cont::ArrayHandle<T>& handle)
{
  typedef typename vtkm::cont::ArrayHandle<T> HandleType;
  typedef typename HandleType::template ExecutionTypes<vtkm::cont::DeviceAdapterTagSerial>::Portal
    PortalType;
  typedef typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType IteratorType;
  IteratorType iter =
    vtkm::cont::ArrayPortalToIterators<PortalType>(handle.GetPortalControl()).GetBegin();
  return &(*iter);
}

template <typename T, typename S>
void DataDump(vtkm::cont::ArrayHandle<T, S> handle, std::string fileName)
{

  T* ptr = GetVTKMPointer(handle);
  vtkm::Id osize = handle.GetNumberOfValues();
  FILE* fp = fopen(fileName.c_str(), "wb");
  ;
  if (fp != NULL)
  {
    fwrite(ptr, sizeof(T), static_cast<size_t>(osize), fp);
  }

  fclose(fp);
}


} // namespace detail
} // namespace zfp
} // namespace worklet
} // namespace vtkm
#endif //  vtk_m_worklet_zfp_tools_h
