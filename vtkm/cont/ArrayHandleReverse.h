//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_ArrayHandleReverse_h
#define vtk_m_cont_ArrayHandleReverse_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename PortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalReverse
{
public:
  using ValueType = typename PortalType::ValueType;

  VTKM_EXEC_CONT
  ArrayPortalReverse()
    : portal()
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalReverse(const PortalType& p)
    : portal(p)
  {
  }

  template <typename OtherPortal>
  VTKM_EXEC_CONT ArrayPortalReverse(const ArrayPortalReverse<OtherPortal>& src)
    : portal(src.GetPortal())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->portal.GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return this->portal.Get(portal.GetNumberOfValues() - index - 1);
  }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    this->portal.Set(portal.GetNumberOfValues() - index - 1, value);
  }

private:
  PortalType portal;
};
}

template <typename ArrayHandleType>
class StorageTagReverse
{
};

namespace internal
{

template <typename ArrayHandleType>
class Storage<typename ArrayHandleType::ValueType, StorageTagReverse<ArrayHandleType>>
{
public:
  using ValueType = typename ArrayHandleType::ValueType;
  using PortalType = ArrayPortalReverse<typename ArrayHandleType::PortalControl>;
  using PortalConstType = ArrayPortalReverse<typename ArrayHandleType::PortalConstControl>;

  VTKM_CONT
  Storage()
    : Array()
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType& a)
    : Array(a)
  {
  }


  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Array.GetPortalConstControl());
  }

  VTKM_CONT
  PortalType GetPortal() { return PortalType(this->Array.GetPortalControl()); }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues) { return this->Array.Allocate(numberOfValues); }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { return this->Array.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  VTKM_CONT
  const ArrayHandleType& GetArray() const { return this->Array; }

private:
  ArrayHandleType Array;
}; // class storage

template <typename ArrayHandleType, typename Device>
class ArrayTransfer<typename ArrayHandleType::ValueType, StorageTagReverse<ArrayHandleType>, Device>
{
public:
  using ValueType = typename ArrayHandleType::ValueType;

private:
  using StorageTag = StorageTagReverse<ArrayHandleType>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution =
    ArrayPortalReverse<typename ArrayHandleType::template ExecutionTypes<Device>::Portal>;
  using PortalConstExecution =
    ArrayPortalReverse<typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, Device()));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // not need to implement
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
};

} // namespace internal

/// \brief Reverse the order of an array, on demand.
///
/// ArrayHandleReverse is a specialization of ArrayHandle. Given an ArrayHandle,
/// it creates a new handle that returns the elements of the array in reverse
/// order (i.e. from end to beginning).
///
template <typename ArrayHandleType>
class ArrayHandleReverse : public vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                          StorageTagReverse<ArrayHandleType>>

{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleReverse,
                             (ArrayHandleReverse<ArrayHandleType>),
                             (vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                      StorageTagReverse<ArrayHandleType>>));

protected:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  ArrayHandleReverse(const ArrayHandleType& handle)
    : Superclass(handle)
  {
  }
};

/// make_ArrayHandleReverse is convenience function to generate an
/// ArrayHandleReverse.
///
template <typename HandleType>
VTKM_CONT ArrayHandleReverse<HandleType> make_ArrayHandleReverse(const HandleType& handle)
{
  return ArrayHandleReverse<HandleType>(handle);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
namespace vtkm
{
namespace cont
{

template <typename AH>
struct SerializableTypeString<vtkm::cont::ArrayHandleReverse<AH>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Reverse<" + SerializableTypeString<AH>::Get() + ">";
    return name;
  }
};

template <typename AH>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<typename AH::ValueType, vtkm::cont::StorageTagReverse<AH>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleReverse<AH>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH>
struct Serialization<vtkm::cont::ArrayHandleReverse<AH>>
{
private:
  using Type = vtkm::cont::ArrayHandleReverse<AH>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, obj.GetStorage().GetArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    vtkmdiy::load(bb, array);
    obj = vtkm::cont::make_ArrayHandleReverse(array);
  }
};

template <typename AH>
struct Serialization<
  vtkm::cont::ArrayHandle<typename AH::ValueType, vtkm::cont::StorageTagReverse<AH>>>
  : Serialization<vtkm::cont::ArrayHandleReverse<AH>>
{
};

} // diy

#endif // vtk_m_cont_ArrayHandleReverse_h
