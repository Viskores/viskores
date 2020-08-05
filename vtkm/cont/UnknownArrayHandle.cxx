//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/UnknownArrayHandle.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

std::shared_ptr<UnknownAHContainer> UnknownAHContainer::MakeNewInstance() const
{
  // Start by doing an invalid copy to create a new container, then swap out the pointer
  // to the array handle to make sure that each object will delete its own ArrayHandle
  // when they get destroyed.
  std::shared_ptr<UnknownAHContainer> newContainer(new UnknownAHContainer(*this));
  newContainer->ArrayHandlePointer = this->NewInstance();
  return newContainer;
}

} // namespace detail

VTKM_CONT bool UnknownArrayHandle::IsValueTypeImpl(std::type_index type) const
{
  if (!this->Container)
  {
    return false;
  }

  // Needs optimization based on platform. OSX cannot compare typeid across translation units?
  return this->Container->ValueType == type;
}

VTKM_CONT bool UnknownArrayHandle::IsStorageTypeImpl(std::type_index type) const
{
  if (!this->Container)
  {
    return false;
  }

  // Needs optimization based on platform. OSX cannot compare typeid across translation units?
  return this->Container->StorageType == type;
}

namespace detail
{

VTKM_CONT_EXPORT void ThrowCastAndCallException(const vtkm::cont::UnknownArrayHandle& ref,
                                                const std::type_info& type)
{
  std::ostringstream out;
  out << "Could not find appropriate cast for array in CastAndCall.\n"
         "Array: ";
  ref.PrintSummary(out);
  out << "TypeList: " << type.name() << "\n";
  throw vtkm::cont::ErrorBadValue(out.str());
}

} // namespace detail
}
} // namespace vtkm::cont
