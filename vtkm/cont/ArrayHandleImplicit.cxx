//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleImplicit.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

BufferMetaDataImplicit::BufferMetaDataImplicit(const BufferMetaDataImplicit& src)
  : Deleter(src.Deleter)
  , Copier(src.Copier)
{
  if (src.Portal)
  {
    VTKM_ASSERT(src.Deleter);
    VTKM_ASSERT(src.Copier);

    this->Portal = src.Copier(src.Portal);
  }
  else
  {
    this->Portal = nullptr;
  }
}

BufferMetaDataImplicit::~BufferMetaDataImplicit()
{
  if (this->Portal)
  {
    VTKM_ASSERT(this->Deleter);
    this->Deleter(this->Portal);
    this->Portal = nullptr;
  }
}

std::unique_ptr<vtkm::cont::internal::BufferMetaData> BufferMetaDataImplicit::DeepCopy() const
{
  return std::unique_ptr<vtkm::cont::internal::BufferMetaData>(new BufferMetaDataImplicit(*this));
}

namespace detail
{

vtkm::cont::internal::BufferMetaDataImplicit* GetImplicitMetaData(
  const vtkm::cont::internal::Buffer& buffer)
{
  vtkm::cont::internal::BufferMetaDataImplicit* metadata =
    dynamic_cast<vtkm::cont::internal::BufferMetaDataImplicit*>(buffer.GetMetaData());
  VTKM_ASSERT(metadata && "Buffer for implicit array does not have correct metadata.");
  return metadata;
}

} // namespace detail

}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace cont
{
}
} // namespace vtkm::cont::detail
