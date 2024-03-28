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
#include <vtkm/cont/ArrayHandleRuntimeVec.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorInternal.h>
#include <vtkm/cont/UncertainArrayHandle.h>

#include <vtkm/cont/internal/ArrayCopyUnknown.h>

#include <cstring>
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

template <typename T>
using IsBasicStorage = std::is_same<vtkm::cont::StorageTagBasic, T>;
template <typename List>
using RemoveBasicStorage = vtkm::ListRemoveIf<List, IsBasicStorage>;

using UnknownSerializationTypes =
  vtkm::ListAppend<vtkm::TypeListBaseC, AllVec<2>, AllVec<3>, AllVec<4>>;
using UnknownSerializationSpecializedStorage =
  vtkm::ListAppend<RemoveBasicStorage<VTKM_DEFAULT_STORAGE_LIST>,
                   vtkm::List<vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
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
    bool typesEqual = (this->Type == rhs.Type);

    // Could use optimization based on platform. OSX cannot compare typeid across translation
    // units, so we have to also check the names. (Why doesn't the == operator above do that?)
    // Are there other platforms that behave similarly?
    if (!typesEqual)
    {
      typesEqual = (std::strcmp(this->Type.name(), rhs.Type.name()) == 0);
    }

    return typesEqual;
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
  bool typesEqual = (this->Container->ValueType == type);

  // Could use optimization based on platform. OSX cannot compare typeid across translation
  // units, so we have to also check the names. (Why doesn't the == operator above do that?)
  // Are there other platforms that behave similarly?
  if (!typesEqual)
  {
    typesEqual = (std::strcmp(this->Container->ValueType.name(), type.name()) == 0);
  }

  return typesEqual;
}

VTKM_CONT bool UnknownArrayHandle::IsStorageTypeImpl(std::type_index type) const
{
  if (!this->Container)
  {
    return false;
  }

  // Needs optimization based on platform. OSX cannot compare typeid across translation units?
  bool typesEqual = (this->Container->StorageType == type);

  // Could use optimization based on platform. OSX cannot compare typeid across translation
  // units, so we have to also check the names. (Why doesn't the == operator above do that?)
  // Are there other platforms that behave similarly?
  if (!typesEqual)
  {
    typesEqual = (std::strcmp(this->Container->StorageType.name(), type.name()) == 0);
  }

  return typesEqual;
}

VTKM_CONT bool UnknownArrayHandle::IsBaseComponentTypeImpl(
  const detail::UnknownAHComponentInfo& type) const
{
  if (!this->Container)
  {
    return false;
  }

  // Note that detail::UnknownAHComponentInfo has a custom operator==
  return this->Container->BaseComponentType == type;
}

VTKM_CONT bool UnknownArrayHandle::IsValid() const
{
  return static_cast<bool>(this->Container);
}

VTKM_CONT UnknownArrayHandle UnknownArrayHandle::NewInstance() const
{
  if (this->IsStorageType<vtkm::cont::StorageTagRuntimeVec>())
  {
    // Special case for `ArrayHandleRuntimeVec`, which (1) can be used in place of
    // a basic array in `UnknownArrayHandle` and (2) needs a special construction to
    // capture the correct number of components. Also note that we are allowing this
    // special case to be implemented in `NewInstanceBasic` because it has a better
    // fallback (throw an exception rather than create a potentially incompatible
    // with the wrong number of components).
    return this->NewInstanceBasic();
  }
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
  if (this->IsStorageType<vtkm::cont::StorageTagRuntimeVec>())
  {
    // Special case for `ArrayHandleRuntimeVec`, which (1) can be used in place of
    // a basic array in `UnknownArrayHandle` and (2) needs a special construction to
    // capture the correct number of components.
    auto runtimeVecArrayCreator = [&](auto exampleComponent) {
      using ComponentType = decltype(exampleComponent);
      if (!newArray.IsValid() && this->IsBaseComponentType<ComponentType>())
      {
        newArray =
          vtkm::cont::make_ArrayHandleRuntimeVec<ComponentType>(this->GetNumberOfComponentsFlat());
      }
    };
    vtkm::ListForEach(runtimeVecArrayCreator, vtkm::TypeListBaseC{});
    if (newArray.IsValid())
    {
      return newArray;
    }
  }
  if (this->Container)
  {
    newArray.Container = this->Container->NewInstanceBasic(this->Container->ArrayHandlePointer);
  }
  return newArray;
}

VTKM_CONT UnknownArrayHandle UnknownArrayHandle::NewInstanceFloatBasic() const
{
  if (this->IsStorageType<vtkm::cont::StorageTagRuntimeVec>())
  {
    // Special case for `ArrayHandleRuntimeVec`, which (1) can be used in place of
    // a basic array in `UnknownArrayHandle` and (2) needs a special construction to
    // capture the correct number of components.
    return vtkm::cont::make_ArrayHandleRuntimeVec<vtkm::FloatDefault>(
      this->GetNumberOfComponentsFlat());
  }
  UnknownArrayHandle newArray;
  if (this->Container)
  {
    newArray.Container =
      this->Container->NewInstanceFloatBasic(this->Container->ArrayHandlePointer);
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

VTKM_CONT std::string UnknownArrayHandle::GetArrayTypeName() const
{
  if (this->Container)
  {
    return "vtkm::cont::ArrayHandle<" + this->GetValueTypeName() + ", " +
      this->GetStorageTypeName() + ">";
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
    return this->Container->NumberOfComponents(this->Container->ArrayHandlePointer);
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
    return this->Container->NumberOfComponentsFlat(this->Container->ArrayHandlePointer);
  }
  else
  {
    return 0;
  }
}

VTKM_CONT void UnknownArrayHandle::Allocate(vtkm::Id numValues,
                                            vtkm::CopyFlag preserve,
                                            vtkm::cont::Token& token) const
{
  if (this->Container)
  {
    this->Container->Allocate(this->Container->ArrayHandlePointer, numValues, preserve, token);
  }
  else
  {
    throw vtkm::cont::ErrorBadAllocation(
      "Cannot allocate UnknownArrayHandle that does not contain an array.");
  }
}

VTKM_CONT void UnknownArrayHandle::Allocate(vtkm::Id numValues, vtkm::CopyFlag preserve) const
{
  vtkm::cont::Token token;
  this->Allocate(numValues, preserve, token);
}

VTKM_CONT void UnknownArrayHandle::DeepCopyFrom(const vtkm::cont::UnknownArrayHandle& source)
{
  if (!this->IsValid())
  {
    *this = source.NewInstance();
  }

  const_cast<const UnknownArrayHandle*>(this)->DeepCopyFrom(source);
}

VTKM_CONT void UnknownArrayHandle::DeepCopyFrom(const vtkm::cont::UnknownArrayHandle& source) const
{
  if (!this->IsValid())
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempty to copy to a constant UnknownArrayHandle with no valid array.");
  }

  if (source.IsValueTypeImpl(this->Container->ValueType) &&
      source.IsStorageTypeImpl(this->Container->StorageType))
  {
    this->Container->DeepCopy(source.Container->ArrayHandlePointer,
                              this->Container->ArrayHandlePointer);
  }
  else
  {
    vtkm::cont::internal::ArrayCopyUnknown(source, *this);
  }
}

VTKM_CONT
void UnknownArrayHandle::CopyShallowIfPossible(const vtkm::cont::UnknownArrayHandle& source)
{
  if (!this->IsValid())
  {
    *this = source;
  }
  else
  {
    const_cast<const UnknownArrayHandle*>(this)->CopyShallowIfPossible(source);
  }
}

VTKM_CONT
void UnknownArrayHandle::CopyShallowIfPossible(const vtkm::cont::UnknownArrayHandle& source) const
{
  if (!this->IsValid())
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempty to copy to a constant UnknownArrayHandle with no valid array.");
  }

  if (source.IsValueTypeImpl(this->Container->ValueType) &&
      source.IsStorageTypeImpl(this->Container->StorageType))
  {
    this->Container->ShallowCopy(source.Container->ArrayHandlePointer,
                                 this->Container->ArrayHandlePointer);
  }
  else
  {
    vtkm::cont::internal::ArrayCopyUnknown(source, *this);
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

namespace internal
{

VTKM_CONT_EXPORT void ThrowCastAndCallException(const vtkm::cont::UnknownArrayHandle& ref,
                                                const std::type_info& type)
{
  std::ostringstream out;
  out << "Could not find appropriate cast for array in CastAndCall.\n"
         "Array: ";
  ref.PrintSummary(out);
  out << "TypeList: " << vtkm::cont::TypeToString(type) << "\n";
  throw vtkm::cont::ErrorBadType(out.str());
}

} // namespace internal
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

namespace
{

enum struct SerializedArrayType : vtkm::UInt8
{
  BasicArray = 0,
  SpecializedStorage
};

struct SaveBasicArray
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType,
                            mangled_diy_namespace::BinaryBuffer& bb,
                            const vtkm::cont::UnknownArrayHandle& obj,
                            bool& saved)
  {
    // Basic arrays and arrays with compatible layouts can be loaed/saved as an
    // ArrayHandleRuntimeVec. Thus, we can load/save them all with one routine.
    using ArrayType = vtkm::cont::ArrayHandleRuntimeVec<ComponentType>;
    if (!saved && obj.CanConvert<ArrayType>())
    {
      ArrayType array = obj.AsArrayHandle<ArrayType>();
      vtkmdiy::save(bb, SerializedArrayType::BasicArray);
      vtkmdiy::save(bb, vtkm::cont::TypeToString<ComponentType>());
      vtkmdiy::save(bb, array);
      saved = true;
    }
  }
};

struct LoadBasicArray
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType,
                            mangled_diy_namespace::BinaryBuffer& bb,
                            vtkm::cont::UnknownArrayHandle& obj,
                            const std::string& componentTypeString,
                            bool& loaded)
  {
    if (!loaded && (componentTypeString == vtkm::cont::TypeToString<ComponentType>()))
    {
      vtkm::cont::ArrayHandleRuntimeVec<ComponentType> array;
      vtkmdiy::load(bb, array);
      obj = array;
      loaded = true;
    }
  }
};

VTKM_CONT void SaveSpecializedArray(mangled_diy_namespace::BinaryBuffer& bb,
                                    const vtkm::cont::UnknownArrayHandle& obj)
{
  vtkm::IdComponent numComponents = obj.GetNumberOfComponents();
  switch (numComponents)
  {
    case 1:
      vtkmdiy::save(bb, SerializedArrayType::SpecializedStorage);
      vtkmdiy::save(bb, numComponents);
      vtkmdiy::save(bb,
                    obj.ResetTypes<vtkm::TypeListBaseC, UnknownSerializationSpecializedStorage>());
      break;
    case 2:
      vtkmdiy::save(bb, SerializedArrayType::SpecializedStorage);
      vtkmdiy::save(bb, numComponents);
      vtkmdiy::save(bb, obj.ResetTypes<AllVec<2>, UnknownSerializationSpecializedStorage>());
      break;
    case 3:
      vtkmdiy::save(bb, SerializedArrayType::SpecializedStorage);
      vtkmdiy::save(bb, numComponents);
      vtkmdiy::save(bb, obj.ResetTypes<AllVec<3>, UnknownSerializationSpecializedStorage>());
      break;
    case 4:
      vtkmdiy::save(bb, SerializedArrayType::SpecializedStorage);
      vtkmdiy::save(bb, numComponents);
      vtkmdiy::save(bb, obj.ResetTypes<AllVec<4>, UnknownSerializationSpecializedStorage>());
      break;
    default:
      throw vtkm::cont::ErrorBadType(
        "Vectors of size " + std::to_string(numComponents) +
        " are not supported for serialization from UnknownArrayHandle. "
        "Try narrowing down possible types with UncertainArrayHandle.");
  }
}

VTKM_CONT void LoadSpecializedArray(mangled_diy_namespace::BinaryBuffer& bb,
                                    vtkm::cont::UnknownArrayHandle& obj)
{
  vtkm::IdComponent numComponents;
  vtkmdiy::load(bb, numComponents);

  vtkm::cont::UncertainArrayHandle<vtkm::TypeListBaseC, UnknownSerializationSpecializedStorage>
    array1;
  vtkm::cont::UncertainArrayHandle<AllVec<2>, UnknownSerializationSpecializedStorage> array2;
  vtkm::cont::UncertainArrayHandle<AllVec<3>, UnknownSerializationSpecializedStorage> array3;
  vtkm::cont::UncertainArrayHandle<AllVec<4>, UnknownSerializationSpecializedStorage> array4;

  switch (numComponents)
  {
    case 1:
      vtkmdiy::load(bb, array1);
      obj = array1;
      break;
    case 2:
      vtkmdiy::load(bb, array2);
      obj = array2;
      break;
    case 3:
      vtkmdiy::load(bb, array3);
      obj = array3;
      break;
    case 4:
      vtkmdiy::load(bb, array4);
      obj = array4;
      break;
    default:
      throw vtkm::cont::ErrorInternal("Unexpected component size when loading UnknownArrayHandle.");
  }
}

} // anonymous namespace

namespace mangled_diy_namespace
{

void Serialization<vtkm::cont::UnknownArrayHandle>::save(BinaryBuffer& bb,
                                                         const vtkm::cont::UnknownArrayHandle& obj)
{
  bool saved = false;

  // First, try serializing basic arrays (which we can do for any Vec size).
  vtkm::ListForEach(SaveBasicArray{}, vtkm::TypeListBaseC{}, bb, obj, saved);

  // If that did not work, try one of the specialized arrays.
  if (!saved)
  {
    SaveSpecializedArray(bb, obj);
  }
}

void Serialization<vtkm::cont::UnknownArrayHandle>::load(BinaryBuffer& bb,
                                                         vtkm::cont::UnknownArrayHandle& obj)
{
  SerializedArrayType arrayType;
  vtkmdiy::load(bb, arrayType);

  switch (arrayType)
  {
    case SerializedArrayType::BasicArray:
    {
      std::string componentTypeString;
      vtkmdiy::load(bb, componentTypeString);
      bool loaded = false;
      vtkm::ListForEach(
        LoadBasicArray{}, vtkm::TypeListBaseC{}, bb, obj, componentTypeString, loaded);
      if (!loaded)
      {
        throw vtkm::cont::ErrorInternal("Failed to load basic array. Unexpected buffer values.");
      }
      break;
    }
    case SerializedArrayType::SpecializedStorage:
      LoadSpecializedArray(bb, obj);
      break;
    default:
      throw vtkm::cont::ErrorInternal("Got inappropriate enumeration value for loading array.");
  }
}

} // namespace mangled_diy_namespace
