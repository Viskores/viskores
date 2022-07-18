//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleGroupVecVariable_h
#define vtk_m_cont_ArrayHandleGroupVecVariable_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <vtkm/Assert.h>
#include <vtkm/VecFromPortal.h>

namespace vtkm
{
namespace internal
{

template <typename ComponentsPortalType, typename OffsetsPortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalGroupVecVariable
{
public:
  using ComponentType = typename std::remove_const<typename ComponentsPortalType::ValueType>::type;
  using ValueType = vtkm::VecFromPortal<ComponentsPortalType>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalGroupVecVariable()
    : ComponentsPortal()
    , OffsetsPortal()
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalGroupVecVariable(const ComponentsPortalType& componentsPortal,
                              const OffsetsPortalType& offsetsPortal)
    : ComponentsPortal(componentsPortal)
    , OffsetsPortal(offsetsPortal)
  {
  }

  /// Copy constructor for any other ArrayPortalConcatenate with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherComponentsPortalType, typename OtherOffsetsPortalType>
  VTKM_EXEC_CONT ArrayPortalGroupVecVariable(
    const ArrayPortalGroupVecVariable<OtherComponentsPortalType, OtherOffsetsPortalType>& src)
    : ComponentsPortal(src.GetComponentsPortal())
    , OffsetsPortal(src.GetOffsetsPortal())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->OffsetsPortal.GetNumberOfValues() - 1; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    vtkm::Id offsetIndex = this->OffsetsPortal.Get(index);
    vtkm::Id nextOffsetIndex = this->OffsetsPortal.Get(index + 1);

    return ValueType(this->ComponentsPortal,
                     static_cast<vtkm::IdComponent>(nextOffsetIndex - offsetIndex),
                     offsetIndex);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id vtkmNotUsed(index), const ValueType& vtkmNotUsed(value)) const
  {
    // The ValueType (VecFromPortal) operates on demand. Thus, if you set
    // something in the value, it has already been passed to the array. Perhaps
    // we should check to make sure that the value used matches the location
    // you are trying to set in the array, but we don't do that.
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const ComponentsPortalType& GetComponentsPortal() const { return this->ComponentsPortal; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const OffsetsPortalType& GetOffsetsPortal() const { return this->OffsetsPortal; }

private:
  ComponentsPortalType ComponentsPortal;
  OffsetsPortalType OffsetsPortal;
};

}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

template <typename ComponentsStorageTag, typename OffsetsStorageTag>
struct VTKM_ALWAYS_EXPORT StorageTagGroupVecVariable
{
};

namespace internal
{

template <typename ComponentsPortal, typename ComponentsStorageTag, typename OffsetsStorageTag>
class Storage<vtkm::VecFromPortal<ComponentsPortal>,
              vtkm::cont::StorageTagGroupVecVariable<ComponentsStorageTag, OffsetsStorageTag>>
{
  using ComponentType = typename ComponentsPortal::ValueType;
  using ComponentsStorage = vtkm::cont::internal::Storage<ComponentType, ComponentsStorageTag>;
  using OffsetsStorage = vtkm::cont::internal::Storage<vtkm::Id, OffsetsStorageTag>;

  using ComponentsArray = vtkm::cont::ArrayHandle<ComponentType, ComponentsStorageTag>;
  using OffsetsArray = vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>;

  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<ComponentsPortal, typename ComponentsStorage::WritePortalType>::value),
    "Used invalid ComponentsPortal type with expected ComponentsStorageTag.");

  struct Info
  {
    std::size_t OffsetsBuffersOffset;
  };

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> ComponentsBuffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + 1,
                                                     buffers.begin() + info.OffsetsBuffersOffset);
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> OffsetsBuffers(
    const std::vector<vtkm::cont::internal::Buffer> buffers)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() + info.OffsetsBuffersOffset,
                                                     buffers.end());
  }

public:
  VTKM_STORAGE_NO_RESIZE;

  using ReadPortalType =
    vtkm::internal::ArrayPortalGroupVecVariable<typename ComponentsStorage::ReadPortalType,
                                                typename OffsetsStorage::ReadPortalType>;
  using WritePortalType =
    vtkm::internal::ArrayPortalGroupVecVariable<typename ComponentsStorage::WritePortalType,
                                                typename OffsetsStorage::ReadPortalType>;

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return OffsetsStorage::GetNumberOfValues(OffsetsBuffers(buffers)) - 1;
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>&,
                             const vtkm::VecFromPortal<ComponentsPortal>&,
                             vtkm::Id,
                             vtkm::Id,
                             vtkm::cont::Token&)
  {
    throw vtkm::cont::ErrorBadType("Fill not supported for ArrayHandleGroupVecVariable.");
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(
      ComponentsStorage::CreateReadPortal(ComponentsBuffers(buffers), device, token),
      OffsetsStorage::CreateReadPortal(OffsetsBuffers(buffers), device, token));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return WritePortalType(
      ComponentsStorage::CreateWritePortal(ComponentsBuffers(buffers), device, token),
      OffsetsStorage::CreateReadPortal(OffsetsBuffers(buffers), device, token));
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const ComponentsArray& componentsArray = ComponentsArray{},
    const OffsetsArray& offsetsArray = OffsetsArray{})
  {
    Info info;
    info.OffsetsBuffersOffset = 1 + componentsArray.GetBuffers().size();
    return vtkm::cont::internal::CreateBuffers(info, componentsArray, offsetsArray);
  }

  VTKM_CONT static ComponentsArray GetComponentsArray(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return ComponentsArray(ComponentsBuffers(buffers));
  }

  VTKM_CONT static OffsetsArray GetOffsetsArray(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return OffsetsArray(OffsetsBuffers(buffers));
  }
};

} // namespace internal

/// \brief Fancy array handle that groups values into vectors of different sizes.
///
/// It is sometimes the case that you need to run a worklet with an input or
/// output that has a different number of values per instance. For example, the
/// cells of a CellCetExplicit can have different numbers of points in each
/// cell. If inputting or outputting cells of this type, each instance of the
/// worklet might need a \c Vec of a different length. This fance array handle
/// takes an array of values and an array of offsets and groups the consecutive
/// values in Vec-like objects. The values are treated as tightly packed, so
/// that each Vec contains the values from one offset to the next. The last
/// value contains values from the last offset to the end of the array.
///
/// For example, if you have an array handle with the 9 values
/// 0,1,2,3,4,5,6,7,8 an offsets array handle with the 4 values 0,4,6,9 and give
/// them to an \c ArrayHandleGroupVecVariable, you get an array that looks like
/// it contains three values of Vec-like objects with the data [0,1,2,3],
/// [4,5], and [6,7,8].
///
/// Note that this version of \c ArrayHandle breaks some of the assumptions
/// about \c ArrayHandle a little bit. Typically, there is exactly one type for
/// every value in the array, and this value is also the same between the
/// control and execution environment. However, this class uses \c
/// VecFromPortal it implement a Vec-like class that has a variable number of
/// values, and this type can change between control and execution
/// environments.
///
/// The offsets array is often derived from a list of sizes for each of the
/// entries. You can use the convenience function \c
/// ConvertNumComponentsToOffsets to take an array of sizes (i.e. the number of
/// components for each entry) and get an array of offsets needed for \c
/// ArrayHandleGroupVecVariable.
///
template <typename ComponentsArrayHandleType, typename OffsetsArrayHandleType>
class ArrayHandleGroupVecVariable
  : public vtkm::cont::ArrayHandle<
      vtkm::VecFromPortal<typename ComponentsArrayHandleType::WritePortalType>,
      vtkm::cont::StorageTagGroupVecVariable<typename ComponentsArrayHandleType::StorageTag,
                                             typename OffsetsArrayHandleType::StorageTag>>
{
  VTKM_IS_ARRAY_HANDLE(ComponentsArrayHandleType);
  VTKM_IS_ARRAY_HANDLE(OffsetsArrayHandleType);

  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<vtkm::Id, typename OffsetsArrayHandleType::ValueType>::value),
    "ArrayHandleGroupVecVariable's offsets array must contain vtkm::Id values.");

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleGroupVecVariable,
    (ArrayHandleGroupVecVariable<ComponentsArrayHandleType, OffsetsArrayHandleType>),
    (vtkm::cont::ArrayHandle<
      vtkm::VecFromPortal<typename ComponentsArrayHandleType::WritePortalType>,
      vtkm::cont::StorageTagGroupVecVariable<typename ComponentsArrayHandleType::StorageTag,
                                             typename OffsetsArrayHandleType::StorageTag>>));

  using ComponentType = typename ComponentsArrayHandleType::ValueType;

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleGroupVecVariable(const ComponentsArrayHandleType& componentsArray,
                              const OffsetsArrayHandleType& offsetsArray)
    : Superclass(StorageType::CreateBuffers(componentsArray, offsetsArray))
  {
  }

  VTKM_CONT ComponentsArrayHandleType GetComponentsArray() const
  {
    return StorageType::GetComponentsArray(this->GetBuffers());
  }

  VTKM_CONT OffsetsArrayHandleType GetOffsetsArray() const
  {
    return StorageType::GetOffsetsArray(this->GetBuffers());
  }
};

/// \c make_ArrayHandleGroupVecVariable is convenience function to generate an
/// ArrayHandleGroupVecVariable. It takes in an ArrayHandle of values and an
/// array handle of offsets and returns an array handle with consecutive
/// entries grouped in a Vec.
///
template <typename ComponentsArrayHandleType, typename OffsetsArrayHandleType>
VTKM_CONT vtkm::cont::ArrayHandleGroupVecVariable<ComponentsArrayHandleType, OffsetsArrayHandleType>
make_ArrayHandleGroupVecVariable(const ComponentsArrayHandleType& componentsArray,
                                 const OffsetsArrayHandleType& offsetsArray)
{
  return vtkm::cont::ArrayHandleGroupVecVariable<ComponentsArrayHandleType, OffsetsArrayHandleType>(
    componentsArray, offsetsArray);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of worklet arguments using ArrayHandleGropuVecVariable
#include <vtkm/exec/arg/FetchTagArrayDirectOutArrayHandleGroupVecVariable.h>

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename SAH, typename OAH>
struct SerializableTypeString<vtkm::cont::ArrayHandleGroupVecVariable<SAH, OAH>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_GroupVecVariable<" + SerializableTypeString<SAH>::Get() + "," +
      SerializableTypeString<OAH>::Get() + ">";
    return name;
  }
};

template <typename SP, typename SST, typename OST>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::VecFromPortal<SP>,
                          vtkm::cont::StorageTagGroupVecVariable<SST, OST>>>
  : SerializableTypeString<
      vtkm::cont::ArrayHandleGroupVecVariable<vtkm::cont::ArrayHandle<typename SP::ValueType, SST>,
                                              vtkm::cont::ArrayHandle<vtkm::Id, OST>>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename SAH, typename OAH>
struct Serialization<vtkm::cont::ArrayHandleGroupVecVariable<SAH, OAH>>
{
private:
  using Type = vtkm::cont::ArrayHandleGroupVecVariable<SAH, OAH>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, Type(obj).GetComponentsArray());
    vtkmdiy::save(bb, Type(obj).GetOffsetsArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    SAH src;
    OAH off;

    vtkmdiy::load(bb, src);
    vtkmdiy::load(bb, off);

    obj = vtkm::cont::make_ArrayHandleGroupVecVariable(src, off);
  }
};

template <typename SP, typename SST, typename OST>
struct Serialization<vtkm::cont::ArrayHandle<vtkm::VecFromPortal<SP>,
                                             vtkm::cont::StorageTagGroupVecVariable<SST, OST>>>
  : Serialization<
      vtkm::cont::ArrayHandleGroupVecVariable<vtkm::cont::ArrayHandle<typename SP::ValueType, SST>,
                                              vtkm::cont::ArrayHandle<vtkm::Id, OST>>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleGroupVecVariable_h
