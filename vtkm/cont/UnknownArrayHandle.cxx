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

#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/UncertainArrayHandle.h>

using UnknownSerializationTypes = vtkm::TypeListAll;
using UnknownSerializationStorage =
  vtkm::ListAppend<VTKM_DEFAULT_STORAGE_LIST,
                   vtkm::List<vtkm::cont::StorageTagBasic,
                              vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                                                     vtkm::cont::StorageTagBasic,
                                                                     vtkm::cont::StorageTagBasic>,
                              vtkm::cont::StorageTagConstant,
                              vtkm::cont::StorageTagCounting,
                              vtkm::cont::StorageTagIndex,
                              vtkm::cont::StorageTagGroupVec<vtkm::cont::StorageTagBasic, 2>,
                              vtkm::cont::StorageTagGroupVec<vtkm::cont::StorageTagBasic, 3>,
                              vtkm::cont::StorageTagGroupVec<vtkm::cont::StorageTagBasic, 4>,
                              vtkm::cont::StorageTagPermutation<vtkm::cont::StorageTagBasic,
                                                                vtkm::cont::StorageTagBasic>,
                              vtkm::cont::StorageTagReverse<vtkm::cont::StorageTagBasic>,
                              vtkm::cont::StorageTagSOA,
                              vtkm::cont::StorageTagUniformPoints>>;

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


//=============================================================================
// Specializations of serialization related classes

namespace vtkm
{
namespace cont
{

std::string SerializableTypeString<vtkm::cont::UnknownArrayHandle>::Get()
{
  return "UnknownAH";
}
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

void Serialization<vtkm::cont::UnknownArrayHandle>::save(BinaryBuffer& bb,
                                                         const vtkm::cont::UnknownArrayHandle& obj)
{
  vtkmdiy::save(bb, obj.ResetTypes<UnknownSerializationTypes, UnknownSerializationStorage>());
}

void Serialization<vtkm::cont::UnknownArrayHandle>::load(BinaryBuffer& bb,
                                                         vtkm::cont::UnknownArrayHandle& obj)
{
  vtkm::cont::UncertainArrayHandle<UnknownSerializationTypes, UnknownSerializationStorage>
    uncertainArray;
  vtkmdiy::load(bb, uncertainArray);
  obj = uncertainArray;
}

} // namespace mangled_diy_namespace
