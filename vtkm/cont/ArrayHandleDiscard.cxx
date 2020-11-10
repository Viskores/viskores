//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleDiscard.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

BufferMetaDataDiscard::~BufferMetaDataDiscard() = default;

std::unique_ptr<vtkm::cont::internal::BufferMetaData> BufferMetaDataDiscard::DeepCopy() const
{
  return std::unique_ptr<vtkm::cont::internal::BufferMetaData>(new BufferMetaDataDiscard(*this));
}

vtkm::cont::internal::BufferMetaDataDiscard* GetDiscardMetaData(
  const vtkm::cont::internal::Buffer& buffer)
{
  vtkm::cont::internal::BufferMetaData* generalMetadata = buffer.GetMetaData();
  if (generalMetadata == nullptr)
  {
    buffer.SetMetaData(vtkm::cont::internal::BufferMetaDataDiscard{});
    generalMetadata = buffer.GetMetaData();
  }

  vtkm::cont::internal::BufferMetaDataDiscard* metadata =
    dynamic_cast<vtkm::cont::internal::BufferMetaDataDiscard*>(buffer.GetMetaData());
  VTKM_ASSERT(metadata && "Buffer for discard array does not have correct metadata.");
  return metadata;
}

}
}
} // namespace vtkm::cont::internal
