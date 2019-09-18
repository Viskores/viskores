//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

void ExecutionArrayInterfaceBasicShareWithControl::Allocate(TypelessExecutionArray& execArray,
                                                            vtkm::Id numberOfValues,
                                                            vtkm::UInt64 sizeOfValue) const
{
  this->ControlStorage.AllocateValues(numberOfValues, sizeOfValue);
  execArray.Array = this->ControlStorage.GetBasePointer();
  execArray.ArrayEnd = this->ControlStorage.GetEndPointer(numberOfValues, sizeOfValue);
  execArray.ArrayCapacity = this->ControlStorage.GetCapacityPointer();
}

void ExecutionArrayInterfaceBasicShareWithControl::Free(TypelessExecutionArray& execArray) const
{
  // Just clear the pointers -- control storage will actually free the
  // memory when the time comes.
  execArray.Array = nullptr;
  execArray.ArrayEnd = nullptr;
  execArray.ArrayCapacity = nullptr;
}

void ExecutionArrayInterfaceBasicShareWithControl::CopyFromControl(const void* src,
                                                                   void* dst,
                                                                   vtkm::UInt64 bytes) const
{
  if (src != dst)
  {
    const vtkm::UInt8* srcBegin = static_cast<const vtkm::UInt8*>(src);
    const vtkm::UInt8* srcEnd = srcBegin + bytes;
    vtkm::UInt8* dstBegin = static_cast<vtkm::UInt8*>(dst);
    std::copy(srcBegin, srcEnd, dstBegin);
  }
}

void ExecutionArrayInterfaceBasicShareWithControl::CopyToControl(const void* src,
                                                                 void* dst,
                                                                 vtkm::UInt64 bytes) const
{
  if (src != dst)
  {
    const vtkm::UInt8* srcBegin = static_cast<const vtkm::UInt8*>(src);
    const vtkm::UInt8* srcEnd = srcBegin + bytes;
    vtkm::UInt8* dstBegin = static_cast<vtkm::UInt8*>(dst);
    std::copy(srcBegin, srcEnd, dstBegin);
  }
}

void ExecutionArrayInterfaceBasicShareWithControl::UsingForRead(const void*,
                                                                const void*,
                                                                vtkm::UInt64) const
{
}
void ExecutionArrayInterfaceBasicShareWithControl::UsingForWrite(const void*,
                                                                 const void*,
                                                                 vtkm::UInt64) const
{
}
void ExecutionArrayInterfaceBasicShareWithControl::UsingForReadWrite(const void*,
                                                                     const void*,
                                                                     vtkm::UInt64) const
{
}
}
}
} // end namespace vtkm::cont::internal
