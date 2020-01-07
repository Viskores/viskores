//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleConcatenate_h
#define vtk_m_cont_ArrayHandleConcatenate_h

#include <vtkm/Deprecated.h>
#include <vtkm/StaticAssert.h>

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename PortalType1, typename PortalType2>
class VTKM_ALWAYS_EXPORT ArrayPortalConcatenate
{
  using WritableP1 = vtkm::internal::PortalSupportsSets<PortalType1>;
  using WritableP2 = vtkm::internal::PortalSupportsSets<PortalType2>;
  using Writable = std::integral_constant<bool, WritableP1::value && WritableP2::value>;

public:
  using ValueType = typename PortalType1::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalConcatenate()
    : portal1()
    , portal2()
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalConcatenate(const PortalType1& p1, const PortalType2& p2)
    : portal1(p1)
    , portal2(p2)
  {
  }

  // Copy constructor
  template <typename OtherP1, typename OtherP2>
  VTKM_EXEC_CONT ArrayPortalConcatenate(const ArrayPortalConcatenate<OtherP1, OtherP2>& src)
    : portal1(src.GetPortal1())
    , portal2(src.GetPortal2())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->portal1.GetNumberOfValues() + this->portal2.GetNumberOfValues();
  }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    if (index < this->portal1.GetNumberOfValues())
    {
      return this->portal1.Get(index);
    }
    else
    {
      return this->portal2.Get(index - this->portal1.GetNumberOfValues());
    }
  }

  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    if (index < this->portal1.GetNumberOfValues())
    {
      this->portal1.Set(index, value);
    }
    else
    {
      this->portal2.Set(index - this->portal1.GetNumberOfValues(), value);
    }
  }

  VTKM_EXEC_CONT
  const PortalType1& GetPortal1() const { return this->portal1; }

  VTKM_EXEC_CONT
  const PortalType2& GetPortal2() const { return this->portal2; }

private:
  PortalType1 portal1;
  PortalType2 portal2;
}; // class ArrayPortalConcatenate

} // namespace internal

template <typename StorageTag1, typename StorageTag2>
class VTKM_ALWAYS_EXPORT StorageTagConcatenate
{
};

namespace internal
{

namespace detail
{

template <typename T, typename ArrayOrStorage, bool IsArrayType>
struct ConcatinateTypeArgImpl;

template <typename T, typename Storage>
struct ConcatinateTypeArgImpl<T, Storage, false>
{
  using StorageTag = Storage;
  using ArrayHandle = vtkm::cont::ArrayHandle<T, StorageTag>;
};

template <typename T, typename Array>
struct ConcatinateTypeArgImpl<T, Array, true>
{
  VTKM_STATIC_ASSERT_MSG((std::is_same<T, typename Array::ValueType>::value),
                         "Used array with wrong type in ArrayHandleConcatinate.");
  using StorageTag VTKM_DEPRECATED(
    1.6,
    "Use storage tags instead of array handles in StorageTagConcatenate.") =
    typename Array::StorageTag;
  using ArrayHandle VTKM_DEPRECATED(
    1.6,
    "Use storage tags instead of array handles in StorageTagConcatenate.") =
    vtkm::cont::ArrayHandle<T, typename Array::StorageTag>;
};

template <typename T, typename ArrayOrStorage>
struct ConcatinateTypeArg
  : ConcatinateTypeArgImpl<T,
                           ArrayOrStorage,
                           vtkm::cont::internal::ArrayHandleCheck<ArrayOrStorage>::type::value>
{
};

} // namespace detail

template <typename T, typename ST1, typename ST2>
class Storage<T, StorageTagConcatenate<ST1, ST2>>
{
  using ArrayHandleType1 = typename detail::ConcatinateTypeArg<T, ST1>::ArrayHandle;
  using ArrayHandleType2 = typename detail::ConcatinateTypeArg<T, ST2>::ArrayHandle;

public:
  using ValueType = T;
  using PortalType = ArrayPortalConcatenate<typename ArrayHandleType1::PortalControl,
                                            typename ArrayHandleType2::PortalControl>;
  using PortalConstType = ArrayPortalConcatenate<typename ArrayHandleType1::PortalConstControl,
                                                 typename ArrayHandleType2::PortalConstControl>;

  VTKM_CONT
  Storage()
    : valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType1& a1, const ArrayHandleType2& a2)
    : array1(a1)
    , array2(a2)
    , valid(true)
  {
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->valid);
    return PortalConstType(this->array1.GetPortalConstControl(),
                           this->array2.GetPortalConstControl());
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->valid);
    return PortalType(this->array1.GetPortalControl(), this->array2.GetPortalControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->valid);
    return this->array1.GetNumberOfValues() + this->array2.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorInternal("ArrayHandleConcatenate should not be allocated explicitly. ");
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->valid);
    if (numberOfValues < this->array1.GetNumberOfValues())
    {
      this->array1.Shrink(numberOfValues);
      this->array2.Shrink(0);
    }
    else
      this->array2.Shrink(numberOfValues - this->array1.GetNumberOfValues());
  }

  VTKM_CONT
  void ReleaseResources()
  {
    VTKM_ASSERT(this->valid);
    this->array1.ReleaseResources();
    this->array2.ReleaseResources();
  }

  VTKM_CONT
  const ArrayHandleType1& GetArray1() const
  {
    VTKM_ASSERT(this->valid);
    return this->array1;
  }

  VTKM_CONT
  const ArrayHandleType2& GetArray2() const
  {
    VTKM_ASSERT(this->valid);
    return this->array2;
  }

private:
  ArrayHandleType1 array1;
  ArrayHandleType2 array2;
  bool valid;
}; // class Storage

template <typename T, typename ST1, typename ST2, typename Device>
class ArrayTransfer<T, StorageTagConcatenate<ST1, ST2>, Device>
{
  using ArrayHandleType1 = typename detail::ConcatinateTypeArg<T, ST1>::ArrayHandle;
  using ArrayHandleType2 = typename detail::ConcatinateTypeArg<T, ST2>::ArrayHandle;
  using StorageTag1 = typename detail::ConcatinateTypeArg<T, ST1>::StorageTag;
  using StorageTag2 = typename detail::ConcatinateTypeArg<T, ST2>::StorageTag;

public:
  using ValueType = T;

private:
  using StorageTag = StorageTagConcatenate<StorageTag1, StorageTag2>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution =
    ArrayPortalConcatenate<typename ArrayHandleType1::template ExecutionTypes<Device>::Portal,
                           typename ArrayHandleType2::template ExecutionTypes<Device>::Portal>;
  using PortalConstExecution =
    ArrayPortalConcatenate<typename ArrayHandleType1::template ExecutionTypes<Device>::PortalConst,
                           typename ArrayHandleType2::template ExecutionTypes<Device>::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : array1(storage->GetArray1())
    , array2(storage->GetArray2())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->array1.GetNumberOfValues() + this->array2.GetNumberOfValues();
  }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->array1.PrepareForInput(Device()),
                                this->array2.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->array1.PrepareForInPlace(Device()),
                           this->array2.PrepareForInPlace(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorInternal("ArrayHandleConcatenate is derived and read-only. ");
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // not need to implement
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    if (numberOfValues < this->array1.GetNumberOfValues())
    {
      this->array1.Shrink(numberOfValues);
      this->array2.Shrink(0);
    }
    else
      this->array2.Shrink(numberOfValues - this->array1.GetNumberOfValues());
  }

  VTKM_CONT
  void ReleaseResources()
  {
    this->array1.ReleaseResourcesExecution();
    this->array2.ReleaseResourcesExecution();
  }

private:
  ArrayHandleType1 array1;
  ArrayHandleType2 array2;
};
}
}
} // namespace vtkm::cont::internal

namespace vtkm
{
namespace cont
{

template <typename ArrayHandleType1, typename ArrayHandleType2>
class ArrayHandleConcatenate
  : public vtkm::cont::ArrayHandle<typename ArrayHandleType1::ValueType,
                                   StorageTagConcatenate<typename ArrayHandleType1::StorageTag,
                                                         typename ArrayHandleType2::StorageTag>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleConcatenate,
    (ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>),
    (vtkm::cont::ArrayHandle<typename ArrayHandleType1::ValueType,
                             StorageTagConcatenate<typename ArrayHandleType1::StorageTag,
                                                   typename ArrayHandleType2::StorageTag>>));

protected:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleConcatenate(const ArrayHandleType1& array1, const ArrayHandleType2& array2)
    : Superclass(StorageType(array1, array2))
  {
  }
};

template <typename ArrayHandleType1, typename ArrayHandleType2>
VTKM_CONT ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2> make_ArrayHandleConcatenate(
  const ArrayHandleType1& array1,
  const ArrayHandleType2& array2)
{
  return ArrayHandleConcatenate<ArrayHandleType1, ArrayHandleType2>(array1, array2);
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

template <typename AH1, typename AH2>
struct SerializableTypeString<vtkm::cont::ArrayHandleConcatenate<AH1, AH2>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Concatenate<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST1, typename ST2>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConcatenate<ST1, ST2>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleConcatenate<
      typename internal::detail::ConcatinateTypeArg<T, ST1>::ArrayHandle,
      typename internal::detail::ConcatinateTypeArg<T, ST2>::ArrayHandle>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2>
struct Serialization<vtkm::cont::ArrayHandleConcatenate<AH1, AH2>>
{
private:
  using Type = vtkm::cont::ArrayHandleConcatenate<AH1, AH2>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    vtkmdiy::save(bb, storage.GetArray1());
    vtkmdiy::save(bb, storage.GetArray2());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 array1;
    AH2 array2;

    vtkmdiy::load(bb, array1);
    vtkmdiy::load(bb, array2);

    obj = vtkm::cont::make_ArrayHandleConcatenate(array1, array2);
  }
};

template <typename T, typename ST1, typename ST2>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConcatenate<ST1, ST2>>>
  : Serialization<vtkm::cont::ArrayHandleConcatenate<
      typename vtkm::cont::internal::detail::ConcatinateTypeArg<T, ST1>::ArrayHandle,
      typename vtkm::cont::internal::detail::ConcatinateTypeArg<T, ST2>::ArrayHandle>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleConcatenate_h
