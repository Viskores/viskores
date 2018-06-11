//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_ArrayHandleExtractComponent_h
#define vtk_m_cont_ArrayHandleExtractComponent_h

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename PortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalExtractComponent
{
public:
  using VectorType = typename PortalType::ValueType;
  using Traits = vtkm::VecTraits<VectorType>;
  using ValueType = typename Traits::ComponentType;

  VTKM_EXEC_CONT
  ArrayPortalExtractComponent()
    : Portal()
    , Component(0)
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalExtractComponent(const PortalType& portal, vtkm::IdComponent component)
    : Portal(portal)
    , Component(component)
  {
  }

  // Copy constructor
  VTKM_EXEC_CONT ArrayPortalExtractComponent(const ArrayPortalExtractComponent<PortalType>& src)
    : Portal(src.Portal)
    , Component(src.Component)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return Traits::GetComponent(this->Portal.Get(index), this->Component);
  }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    VectorType vec = this->Portal.Get(index);
    Traits::SetComponent(vec, this->Component, value);
    this->Portal.Set(index, vec);
  }

  VTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
  vtkm::IdComponent Component;
}; // class ArrayPortalExtractComponent

} // namespace internal

template <typename ArrayHandleType>
class StorageTagExtractComponent
{
};

namespace internal
{

template <typename ArrayHandleType>
class Storage<typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
              StorageTagExtractComponent<ArrayHandleType>>
{
public:
  using PortalType = ArrayPortalExtractComponent<typename ArrayHandleType::PortalControl>;
  using PortalConstType = ArrayPortalExtractComponent<typename ArrayHandleType::PortalConstControl>;
  using ValueType = typename PortalType::ValueType;

  VTKM_CONT
  Storage()
    : Array()
    , Component(0)
    , Valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType& array, vtkm::IdComponent component)
    : Array(array)
    , Component(component)
    , Valid(true)
  {
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->Valid);
    return PortalConstType(this->Array.GetPortalConstControl(), this->Component);
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->Valid);
    return PortalType(this->Array.GetPortalControl(), this->Component);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->Array.Allocate(numberOfValues);
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->Array.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->Valid);
    this->Array.ReleaseResources();
  }

  VTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Array;
  }

  VTKM_CONT
  vtkm::IdComponent GetComponent() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Component;
  }

private:
  ArrayHandleType Array;
  vtkm::IdComponent Component;
  bool Valid;
}; // class Storage

template <typename ArrayHandleType, typename Device>
class ArrayTransfer<typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
                    StorageTagExtractComponent<ArrayHandleType>,
                    Device>
{
public:
  using ValueType = typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType;

private:
  using StorageTag = StorageTagExtractComponent<ArrayHandleType>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;
  using ArrayValueType = typename ArrayHandleType::ValueType;
  using ArrayStorageTag = typename ArrayHandleType::StorageTag;
  using ArrayStorageType =
    vtkm::cont::internal::Storage<typename ArrayHandleType::ValueType, ArrayStorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using ExecutionTypes = typename ArrayHandleType::template ExecutionTypes<Device>;
  using PortalExecution = ArrayPortalExtractComponent<typename ExecutionTypes::Portal>;
  using PortalConstExecution = ArrayPortalExtractComponent<typename ExecutionTypes::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , Component(storage->GetComponent())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()), this->Component);
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(Device()), this->Component);
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, Device()), this->Component);
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
  vtkm::IdComponent Component;
};
}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace cont
{

/// \brief A fancy ArrayHandle that turns a vector array into a scalar array by
/// slicing out a single component of each vector.
///
/// ArrayHandleExtractComponent is a specialization of ArrayHandle. It takes an
/// input ArrayHandle with a vtkm::Vec ValueType and a component index
/// and uses this information to expose a scalar array consisting of the
/// specified component across all vectors in the input ArrayHandle. So for a
/// given index i, ArrayHandleExtractComponent looks up the i-th vtkm::Vec in
/// the index array and reads or writes to the specified component, leave all
/// other components unmodified. This is done on the fly rather than creating a
/// copy of the array.
template <typename ArrayHandleType>
class ArrayHandleExtractComponent
  : public vtkm::cont::ArrayHandle<
      typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
      StorageTagExtractComponent<ArrayHandleType>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleExtractComponent,
    (ArrayHandleExtractComponent<ArrayHandleType>),
    (vtkm::cont::ArrayHandle<
      typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType,
      StorageTagExtractComponent<ArrayHandleType>>));

protected:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleExtractComponent(const ArrayHandleType& array, vtkm::IdComponent component)
    : Superclass(StorageType(array, component))
  {
  }
};

/// make_ArrayHandleExtractComponent is convenience function to generate an
/// ArrayHandleExtractComponent.
template <typename ArrayHandleType>
VTKM_CONT ArrayHandleExtractComponent<ArrayHandleType> make_ArrayHandleExtractComponent(
  const ArrayHandleType& array,
  vtkm::IdComponent component)
{
  return ArrayHandleExtractComponent<ArrayHandleType>(array, component);
}
}
} // namespace vtkm::cont

#endif // vtk_m_cont_ArrayHandleExtractComponent_h
