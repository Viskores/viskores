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

#include <sstream>

namespace
{

template <vtkm::IdComponent N, typename ScalarList>
struct AllVecImpl;
template <vtkm::IdComponent N, typename... Scalars>
struct AllVecImpl<N, vtkm::List<Scalars...>>
{
  using type = vtkm::List<vtkm::Vec<Scalars, N>...>;
};

// Normally I would implement this with vtkm::ListTransform, but that is causing an ICE in GCC 4.8.
// This implementation is not much different.
template <vtkm::IdComponent N>
using AllVec = typename AllVecImpl<N, vtkm::TypeListBaseC>::type;

using UnknownSerializationTypes =
  vtkm::ListAppend<vtkm::TypeListBaseC, AllVec<2>, AllVec<3>, AllVec<4>>;
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

} // anonymous namespace

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

bool UnknownAHComponentInfo::operator==(const UnknownAHComponentInfo& rhs)
{
  if (this->IsIntegral || this->IsFloat)
  {
    return ((this->IsIntegral == rhs.IsIntegral) && (this->IsFloat == rhs.IsFloat) &&
            (this->IsSigned == rhs.IsSigned) && (this->Size == rhs.Size));
  }
  else
  {
    // Needs optimization based on platform. OSX cannot compare typeid across translation units?
    return this->Type == rhs.Type;
  }
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

VTKM_CONT bool UnknownArrayHandle::IsBaseComponentTypeImpl(
  const detail::UnknownAHComponentInfo& type) const
{
  if (!this->Container)
  {
    return false;
  }

  // Needs optimization based on platform. OSX cannot compare typeid across translation units?
  return this->Container->BaseComponentType == type;
}

VTKM_CONT bool UnknownArrayHandle::IsValid() const
{
  return static_cast<bool>(this->Container);
}

VTKM_CONT UnknownArrayHandle UnknownArrayHandle::NewInstance() const
{
  UnknownArrayHandle newArray;
  if (this->Container)
  {
    newArray.Container = this->Container->MakeNewInstance();
  }
  return newArray;
}

VTKM_CONT UnknownArrayHandle UnknownArrayHandle::NewInstanceBasic() const
{
  UnknownArrayHandle newArray;
  if (this->Container)
  {
    newArray.Container = this->Container->NewInstanceBasic();
  }
  return newArray;
}

VTKM_CONT UnknownArrayHandle UnknownArrayHandle::NewInstanceFloatBasic() const
{
  UnknownArrayHandle newArray;
  if (this->Container)
  {
    newArray.Container = this->Container->NewInstanceFloatBasic();
  }
  return newArray;
}

VTKM_CONT std::string UnknownArrayHandle::GetValueTypeName() const
{
  if (this->Container)
  {
    return vtkm::cont::TypeToString(this->Container->ValueType);
  }
  else
  {
    return "";
  }
}

VTKM_CONT std::string UnknownArrayHandle::GetBaseComponentTypeName() const
{
  if (this->Container)
  {
    return vtkm::cont::TypeToString(this->Container->BaseComponentType);
  }
  else
  {
    return "";
  }
}

VTKM_CONT std::string UnknownArrayHandle::GetStorageTypeName() const
{
  if (this->Container)
  {
    return vtkm::cont::TypeToString(this->Container->StorageType);
  }
  else
  {
    return "";
  }
}

VTKM_CONT vtkm::Id UnknownArrayHandle::GetNumberOfValues() const
{
  if (this->Container)
  {
    return this->Container->NumberOfValues(this->Container->ArrayHandlePointer);
  }
  else
  {
    return 0;
  }
}

vtkm::IdComponent UnknownArrayHandle::GetNumberOfComponents() const
{
  if (this->Container)
  {
    return this->Container->NumberOfComponents();
  }
  else
  {
    return 0;
  }
}

VTKM_CONT vtkm::IdComponent UnknownArrayHandle::GetNumberOfComponentsFlat() const
{
  if (this->Container)
  {
    return this->Container->NumberOfComponentsFlat();
  }
  else
  {
    return 0;
  }
}

VTKM_CONT void UnknownArrayHandle::Allocate(vtkm::Id numValues) const
{
  if (this->Container)
  {
    this->Container->Allocate(this->Container->ArrayHandlePointer, numValues);
  }
  else
  {
    throw vtkm::cont::ErrorBadAllocation(
      "Cannot allocate UnknownArrayHandle that does not contain an array.");
  }
}

VTKM_CONT void UnknownArrayHandle::ReleaseResourcesExecution() const
{
  if (this->Container)
  {
    this->Container->ReleaseResourcesExecution(this->Container->ArrayHandlePointer);
  }
}

VTKM_CONT void UnknownArrayHandle::ReleaseResources() const
{
  if (this->Container)
  {
    this->Container->ReleaseResources(this->Container->ArrayHandlePointer);
  }
}

VTKM_CONT void UnknownArrayHandle::PrintSummary(std::ostream& out, bool full) const
{
  if (this->Container)
  {
    this->Container->PrintSummary(this->Container->ArrayHandlePointer, out, full);
  }
  else
  {
    out << "null UnknownArrayHandle" << std::endl;
  }
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
  out << "TypeList: " << vtkm::cont::TypeToString(type) << "\n";
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
