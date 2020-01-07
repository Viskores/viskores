//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleGroupVec_h
#define vtk_m_cont_ArrayHandleGroupVec_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/ErrorBadValue.h>

namespace vtkm
{
namespace exec
{

namespace internal
{

template <typename PortalType, vtkm::IdComponent N_COMPONENTS>
class VTKM_ALWAYS_EXPORT ArrayPortalGroupVec
{
  using Writable = vtkm::internal::PortalSupportsSets<PortalType>;

public:
  static constexpr vtkm::IdComponent NUM_COMPONENTS = N_COMPONENTS;
  using SourcePortalType = PortalType;

  using ComponentType = typename std::remove_const<typename SourcePortalType::ValueType>::type;
  using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalGroupVec()
    : SourcePortal()
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalGroupVec(const SourcePortalType& sourcePortal)
    : SourcePortal(sourcePortal)
  {
  }

  /// Copy constructor for any other ArrayPortalConcatenate with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherSourcePortalType>
  VTKM_EXEC_CONT ArrayPortalGroupVec(
    const ArrayPortalGroupVec<OtherSourcePortalType, NUM_COMPONENTS>& src)
    : SourcePortal(src.GetPortal())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->SourcePortal.GetNumberOfValues() / NUM_COMPONENTS;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    ValueType result;
    vtkm::Id sourceIndex = index * NUM_COMPONENTS;
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
    {
      result[componentIndex] = this->SourcePortal.Get(sourceIndex);
      sourceIndex++;
    }
    return result;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    vtkm::Id sourceIndex = index * NUM_COMPONENTS;
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
    {
      this->SourcePortal.Set(sourceIndex, value[componentIndex]);
      sourceIndex++;
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const SourcePortalType& GetPortal() const { return this->SourcePortal; }

private:
  SourcePortalType SourcePortal;
};
}
}
} // namespace vtkm::exec::internal

namespace vtkm
{
namespace cont
{

template <typename SourceStorageTag, vtkm::IdComponent NUM_COMPONENTS>
struct VTKM_ALWAYS_EXPORT StorageTagGroupVec
{
};

namespace internal
{

template <typename ComponentType, vtkm::IdComponent NUM_COMPONENTS, typename SourceStorageTag>
class Storage<vtkm::Vec<ComponentType, NUM_COMPONENTS>,
              vtkm::cont::StorageTagGroupVec<SourceStorageTag, NUM_COMPONENTS>>
{
  using SourceArrayHandleType = vtkm::cont::ArrayHandle<ComponentType, SourceStorageTag>;

public:
  using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

  using PortalType =
    vtkm::exec::internal::ArrayPortalGroupVec<typename SourceArrayHandleType::PortalControl,
                                              NUM_COMPONENTS>;
  using PortalConstType =
    vtkm::exec::internal::ArrayPortalGroupVec<typename SourceArrayHandleType::PortalConstControl,
                                              NUM_COMPONENTS>;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const SourceArrayHandleType& sourceArray)
    : SourceArray(sourceArray)
    , Valid(true)
  {
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->SourceArray.GetPortalControl());
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(this->SourceArray.GetPortalConstControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    vtkm::Id sourceSize = this->SourceArray.GetNumberOfValues();
    if (sourceSize % NUM_COMPONENTS != 0)
    {
      throw vtkm::cont::ErrorBadValue(
        "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return sourceSize / NUM_COMPONENTS;
  }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->SourceArray.Allocate(numberOfValues * NUM_COMPONENTS);
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->SourceArray.Shrink(numberOfValues * NUM_COMPONENTS);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    if (this->Valid)
    {
      this->SourceArray.ReleaseResources();
    }
  }

  // Required for later use in ArrayTransfer class
  VTKM_CONT
  const SourceArrayHandleType& GetSourceArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->SourceArray;
  }

private:
  SourceArrayHandleType SourceArray;
  bool Valid;
};

template <typename ComponentType,
          vtkm::IdComponent NUM_COMPONENTS,
          typename SourceStorageTag,
          typename Device>
class ArrayTransfer<vtkm::Vec<ComponentType, NUM_COMPONENTS>,
                    vtkm::cont::StorageTagGroupVec<SourceStorageTag, NUM_COMPONENTS>,
                    Device>
{
public:
  using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

private:
  using StorageTag = vtkm::cont::StorageTagGroupVec<SourceStorageTag, NUM_COMPONENTS>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

  using SourceArrayHandleType = vtkm::cont::ArrayHandle<ComponentType, SourceStorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = vtkm::exec::internal::ArrayPortalGroupVec<
    typename SourceArrayHandleType::template ExecutionTypes<Device>::Portal,
    NUM_COMPONENTS>;
  using PortalConstExecution = vtkm::exec::internal::ArrayPortalGroupVec<
    typename SourceArrayHandleType::template ExecutionTypes<Device>::PortalConst,
    NUM_COMPONENTS>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : SourceArray(storage->GetSourceArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    vtkm::Id sourceSize = this->SourceArray.GetNumberOfValues();
    if (sourceSize % NUM_COMPONENTS != 0)
    {
      throw vtkm::cont::ErrorBadValue(
        "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return sourceSize / NUM_COMPONENTS;
  }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    if (this->SourceArray.GetNumberOfValues() % NUM_COMPONENTS != 0)
    {
      throw vtkm::cont::ErrorBadValue(
        "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return PortalConstExecution(this->SourceArray.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    if (this->SourceArray.GetNumberOfValues() % NUM_COMPONENTS != 0)
    {
      throw vtkm::cont::ErrorBadValue(
        "ArrayHandleGroupVec's source array does not divide evenly into Vecs.");
    }
    return PortalExecution(this->SourceArray.PrepareForInPlace(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(
      this->SourceArray.PrepareForOutput(numberOfValues * NUM_COMPONENTS, Device()));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    this->SourceArray.Shrink(numberOfValues * NUM_COMPONENTS);
  }

  VTKM_CONT
  void ReleaseResources() { this->SourceArray.ReleaseResourcesExecution(); }

private:
  SourceArrayHandleType SourceArray;
};

} // namespace internal

/// \brief Fancy array handle that groups values into vectors.
///
/// It is sometimes the case that an array is stored such that consecutive
/// entries are meant to form a group. This fancy array handle takes an array
/// of values and a size of groups and then groups the consecutive values
/// stored in a \c Vec.
///
/// For example, if you have an array handle with the six values 0,1,2,3,4,5
/// and give it to a \c ArrayHandleGroupVec with the number of components set
/// to 3, you get an array that looks like it contains two values of \c Vec
/// values of size 3 with the data [0,1,2], [3,4,5].
///
template <typename SourceArrayHandleType, vtkm::IdComponent NUM_COMPONENTS>
class ArrayHandleGroupVec
  : public vtkm::cont::ArrayHandle<
      vtkm::Vec<typename SourceArrayHandleType::ValueType, NUM_COMPONENTS>,
      vtkm::cont::StorageTagGroupVec<typename SourceArrayHandleType::StorageTag, NUM_COMPONENTS>>
{
  VTKM_IS_ARRAY_HANDLE(SourceArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleGroupVec,
    (ArrayHandleGroupVec<SourceArrayHandleType, NUM_COMPONENTS>),
    (vtkm::cont::ArrayHandle<
      vtkm::Vec<typename SourceArrayHandleType::ValueType, NUM_COMPONENTS>,
      vtkm::cont::StorageTagGroupVec<typename SourceArrayHandleType::StorageTag, NUM_COMPONENTS>>));

  using ComponentType = typename SourceArrayHandleType::ValueType;

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleGroupVec(const SourceArrayHandleType& sourceArray)
    : Superclass(StorageType(sourceArray))
  {
  }
};

/// \c make_ArrayHandleGroupVec is convenience function to generate an
/// ArrayHandleGroupVec. It takes in an ArrayHandle and the number of components
/// (as a specified template parameter), and returns an array handle with
/// consecutive entries grouped in a Vec.
///
template <vtkm::IdComponent NUM_COMPONENTS, typename ArrayHandleType>
VTKM_CONT vtkm::cont::ArrayHandleGroupVec<ArrayHandleType, NUM_COMPONENTS> make_ArrayHandleGroupVec(
  const ArrayHandleType& array)
{
  return vtkm::cont::ArrayHandleGroupVec<ArrayHandleType, NUM_COMPONENTS>(array);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename AH, vtkm::IdComponent NUM_COMPS>
struct SerializableTypeString<vtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "AH_GroupVec<" + SerializableTypeString<AH>::Get() + "," + std::to_string(NUM_COMPS) + ">";
    return name;
  }
};

template <typename T, vtkm::IdComponent NUM_COMPS, typename ST>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, NUM_COMPS>, vtkm::cont::StorageTagGroupVec<ST, NUM_COMPS>>>
  : SerializableTypeString<
      vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<T, ST>, NUM_COMPS>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH, vtkm::IdComponent NUM_COMPS>
struct Serialization<vtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>>
{
private:
  using Type = vtkm::cont::ArrayHandleGroupVec<AH, NUM_COMPS>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, obj.GetStorage().GetSourceArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    vtkmdiy::load(bb, array);

    obj = vtkm::cont::make_ArrayHandleGroupVec<NUM_COMPS>(array);
  }
};

template <typename T, vtkm::IdComponent NUM_COMPS, typename ST>
struct Serialization<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, NUM_COMPS>, vtkm::cont::StorageTagGroupVec<ST, NUM_COMPS>>>
  : Serialization<vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<T, ST>, NUM_COMPS>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleGroupVec_h
