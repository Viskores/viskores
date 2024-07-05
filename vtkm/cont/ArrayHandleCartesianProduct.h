//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleCartesianProduct_h
#define vtk_m_cont_ArrayHandleCartesianProduct_h

#include <vtkm/Assert.h>
#include <vtkm/Range.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/Token.h>

#include <array>

namespace vtkm
{
namespace internal
{

/// \brief An array portal that acts as a 3D cartesian product of 3 arrays.
///
template <typename ValueType_,
          typename PortalTypeFirst_,
          typename PortalTypeSecond_,
          typename PortalTypeThird_>
class VTKM_ALWAYS_EXPORT ArrayPortalCartesianProduct
{
public:
  using ValueType = ValueType_;
  using IteratorType = ValueType_;
  using PortalTypeFirst = PortalTypeFirst_;
  using PortalTypeSecond = PortalTypeSecond_;
  using PortalTypeThird = PortalTypeThird_;

  using set_supported_p1 = vtkm::internal::PortalSupportsSets<PortalTypeFirst>;
  using set_supported_p2 = vtkm::internal::PortalSupportsSets<PortalTypeSecond>;
  using set_supported_p3 = vtkm::internal::PortalSupportsSets<PortalTypeThird>;

  using Writable = std::integral_constant<bool,
                                          set_supported_p1::value && set_supported_p2::value &&
                                            set_supported_p3::value>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalCartesianProduct()
    : PortalFirst()
    , PortalSecond()
    , PortalThird()
  {
  } //needs to be host and device so that cuda can create lvalue of these

  VTKM_CONT
  ArrayPortalCartesianProduct(const PortalTypeFirst& portalfirst,
                              const PortalTypeSecond& portalsecond,
                              const PortalTypeThird& portalthird)
    : PortalFirst(portalfirst)
    , PortalSecond(portalsecond)
    , PortalThird(portalthird)
  {
  }

  /// Copy constructor for any other ArrayPortalCartesianProduct with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///

  template <class OtherV, class OtherP1, class OtherP2, class OtherP3>
  VTKM_CONT ArrayPortalCartesianProduct(
    const ArrayPortalCartesianProduct<OtherV, OtherP1, OtherP2, OtherP3>& src)
    : PortalFirst(src.GetPortalFirst())
    , PortalSecond(src.GetPortalSecond())
    , PortalThird(src.GetPortalThird())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->PortalFirst.GetNumberOfValues() * this->PortalSecond.GetNumberOfValues() *
      this->PortalThird.GetNumberOfValues();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    vtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
    vtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
    vtkm::Id dim12 = dim1 * dim2;
    vtkm::Id idx12 = index % dim12;
    vtkm::Id i1 = idx12 % dim1;
    vtkm::Id i2 = idx12 / dim1;
    vtkm::Id i3 = index / dim12;

    return vtkm::make_Vec(
      this->PortalFirst.Get(i1), this->PortalSecond.Get(i2), this->PortalThird.Get(i3));
  }


  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->GetNumberOfValues());

    vtkm::Id dim1 = this->PortalFirst.GetNumberOfValues();
    vtkm::Id dim2 = this->PortalSecond.GetNumberOfValues();
    vtkm::Id dim12 = dim1 * dim2;
    vtkm::Id idx12 = index % dim12;

    vtkm::Id i1 = idx12 % dim1;
    vtkm::Id i2 = idx12 / dim1;
    vtkm::Id i3 = index / dim12;

    this->PortalFirst.Set(i1, value[0]);
    this->PortalSecond.Set(i2, value[1]);
    this->PortalThird.Set(i3, value[2]);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const PortalTypeFirst& GetFirstPortal() const { return this->PortalFirst; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const PortalTypeSecond& GetSecondPortal() const { return this->PortalSecond; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const PortalTypeThird& GetThirdPortal() const { return this->PortalThird; }

private:
  PortalTypeFirst PortalFirst;
  PortalTypeSecond PortalSecond;
  PortalTypeThird PortalThird;
};
}
} // namespace vtkm::internal

namespace vtkm
{
namespace cont
{

template <typename StorageTag1, typename StorageTag2, typename StorageTag3>
struct VTKM_ALWAYS_EXPORT StorageTagCartesianProduct
{
};

namespace internal
{

/// This helper struct defines the value type for a zip container containing
/// the given two array handles.
///
template <typename AH1, typename AH2, typename AH3>
struct ArrayHandleCartesianProductTraits
{
  VTKM_IS_ARRAY_HANDLE(AH1);
  VTKM_IS_ARRAY_HANDLE(AH2);
  VTKM_IS_ARRAY_HANDLE(AH3);

  using ComponentType = typename AH1::ValueType;
  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename AH2::ValueType>::value),
    "All arrays for ArrayHandleCartesianProduct must have the same value type. "
    "Use ArrayHandleCast as necessary to make types match.");
  VTKM_STATIC_ASSERT_MSG(
    (std::is_same<ComponentType, typename AH3::ValueType>::value),
    "All arrays for ArrayHandleCartesianProduct must have the same value type. "
    "Use ArrayHandleCast as necessary to make types match.");

  /// The ValueType (a pair containing the value types of the two arrays).
  ///
  using ValueType = vtkm::Vec<ComponentType, 3>;

  /// The appropriately templated tag.
  ///
  using Tag = vtkm::cont::StorageTagCartesianProduct<typename AH1::StorageTag,
                                                     typename AH2::StorageTag,
                                                     typename AH3::StorageTag>;

  /// The superclass for ArrayHandleCartesianProduct.
  ///
  using Superclass = vtkm::cont::ArrayHandle<ValueType, Tag>;
};

template <typename T, typename ST1, typename ST2, typename ST3>
class Storage<vtkm::Vec<T, 3>, vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>
{
  struct Info
  {
    std::array<std::size_t, 4> BufferOffset;
  };

  using Storage1 = vtkm::cont::internal::Storage<T, ST1>;
  using Storage2 = vtkm::cont::internal::Storage<T, ST2>;
  using Storage3 = vtkm::cont::internal::Storage<T, ST3>;

  using Array1 = vtkm::cont::ArrayHandle<T, ST1>;
  using Array2 = vtkm::cont::ArrayHandle<T, ST2>;
  using Array3 = vtkm::cont::ArrayHandle<T, ST3>;

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> GetBuffers(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    std::size_t subArray)
  {
    Info info = buffers[0].GetMetaData<Info>();
    return std::vector<vtkm::cont::internal::Buffer>(buffers.begin() +
                                                       info.BufferOffset[subArray - 1],
                                                     buffers.begin() + info.BufferOffset[subArray]);
  }

public:
  VTKM_STORAGE_NO_RESIZE;

  using ReadPortalType =
    vtkm::internal::ArrayPortalCartesianProduct<vtkm::Vec<T, 3>,
                                                typename Storage1::ReadPortalType,
                                                typename Storage2::ReadPortalType,
                                                typename Storage3::ReadPortalType>;
  using WritePortalType =
    vtkm::internal::ArrayPortalCartesianProduct<vtkm::Vec<T, 3>,
                                                typename Storage1::WritePortalType,
                                                typename Storage2::WritePortalType,
                                                typename Storage3::WritePortalType>;

  VTKM_CONT static vtkm::IdComponent GetNumberOfComponentsFlat(
    const std::vector<vtkm::cont::internal::Buffer>&)
  {
    return vtkm::VecFlat<T>::NUM_COMPONENTS * 3;
  }

  VTKM_CONT static vtkm::Id GetNumberOfValues(
    const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return (Storage1::GetNumberOfValues(GetBuffers(buffers, 1)) *
            Storage2::GetNumberOfValues(GetBuffers(buffers, 2)) *
            Storage3::GetNumberOfValues(GetBuffers(buffers, 3)));
  }

  VTKM_CONT static void Fill(const std::vector<vtkm::cont::internal::Buffer>& buffers,
                             const vtkm::Vec<T, 3>& fillValue,
                             vtkm::Id startIndex,
                             vtkm::Id endIndex,
                             vtkm::cont::Token& token)
  {
    if ((startIndex != 0) || (endIndex != GetNumberOfValues(buffers)))
    {
      throw vtkm::cont::ErrorBadValue(
        "Fill for ArrayHandleCartesianProduct can only be used to fill entire array.");
    }
    auto subBuffers = GetBuffers(buffers, 1);
    Storage1::Fill(subBuffers, fillValue[0], 0, Storage1::GetNumberOfValues(subBuffers), token);
    subBuffers = GetBuffers(buffers, 2);
    Storage2::Fill(subBuffers, fillValue[1], 0, Storage2::GetNumberOfValues(subBuffers), token);
    subBuffers = GetBuffers(buffers, 3);
    Storage3::Fill(subBuffers, fillValue[2], 0, Storage3::GetNumberOfValues(subBuffers), token);
  }

  VTKM_CONT static ReadPortalType CreateReadPortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return ReadPortalType(Storage1::CreateReadPortal(GetBuffers(buffers, 1), device, token),
                          Storage2::CreateReadPortal(GetBuffers(buffers, 2), device, token),
                          Storage3::CreateReadPortal(GetBuffers(buffers, 3), device, token));
  }

  VTKM_CONT static WritePortalType CreateWritePortal(
    const std::vector<vtkm::cont::internal::Buffer>& buffers,
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token)
  {
    return WritePortalType(Storage1::CreateWritePortal(GetBuffers(buffers, 1), device, token),
                           Storage2::CreateWritePortal(GetBuffers(buffers, 2), device, token),
                           Storage3::CreateWritePortal(GetBuffers(buffers, 3), device, token));
  }

  VTKM_CONT static Array1 GetArrayHandle1(const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return Array1(GetBuffers(buffers, 1));
  }
  VTKM_CONT static Array2 GetArrayHandle2(const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return Array2(GetBuffers(buffers, 2));
  }
  VTKM_CONT static Array3 GetArrayHandle3(const std::vector<vtkm::cont::internal::Buffer>& buffers)
  {
    return Array3(GetBuffers(buffers, 3));
  }

  VTKM_CONT static std::vector<vtkm::cont::internal::Buffer> CreateBuffers(
    const Array1& array1 = Array1{},
    const Array2& array2 = Array2{},
    const Array3& array3 = Array3{})
  {
    const std::vector<vtkm::cont::internal::Buffer>& buffers1 = array1.GetBuffers();
    const std::vector<vtkm::cont::internal::Buffer>& buffers2 = array2.GetBuffers();
    const std::vector<vtkm::cont::internal::Buffer>& buffers3 = array3.GetBuffers();

    Info info;
    info.BufferOffset[0] = 1;
    info.BufferOffset[1] = info.BufferOffset[0] + buffers1.size();
    info.BufferOffset[2] = info.BufferOffset[1] + buffers2.size();
    info.BufferOffset[3] = info.BufferOffset[2] + buffers3.size();

    return vtkm::cont::internal::CreateBuffers(info, buffers1, buffers2, buffers3);
  }
};
} // namespace internal

/// ArrayHandleCartesianProduct is a specialization of ArrayHandle. It takes two delegate
/// array handle and makes a new handle that access the corresponding entries
/// in these arrays as a pair.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
class ArrayHandleCartesianProduct
  : public internal::ArrayHandleCartesianProductTraits<FirstHandleType,
                                                       SecondHandleType,
                                                       ThirdHandleType>::Superclass
{
  // If the following line gives a compile error, then the FirstHandleType
  // template argument is not a valid ArrayHandle type.
  VTKM_IS_ARRAY_HANDLE(FirstHandleType);
  VTKM_IS_ARRAY_HANDLE(SecondHandleType);
  VTKM_IS_ARRAY_HANDLE(ThirdHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCartesianProduct,
    (ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>),
    (typename internal::ArrayHandleCartesianProductTraits<FirstHandleType,
                                                          SecondHandleType,
                                                          ThirdHandleType>::Superclass));

  /// Construct an `ArrayHandleCartesianProduct` given arrays for the coordinates in
  /// the x, y, and z diretions.
  VTKM_CONT
  ArrayHandleCartesianProduct(const FirstHandleType& firstArray,
                              const SecondHandleType& secondArray,
                              const ThirdHandleType& thirdArray)
    : Superclass(StorageType::CreateBuffers(firstArray, secondArray, thirdArray))
  {
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleCartesianProduct() {}

  /// Get the array for the coordinates in the x direction.
  VTKM_CONT FirstHandleType GetFirstArray() const
  {
    return StorageType::GetArrayHandle1(this->GetBuffers());
  }
  /// Get the array for the coordinates in the y direction.
  VTKM_CONT SecondHandleType GetSecondArray() const
  {
    return StorageType::GetArrayHandle2(this->GetBuffers());
  }
  /// Get the array for the coordinates in the z direction.
  VTKM_CONT ThirdHandleType GetThirdArray() const
  {
    return StorageType::GetArrayHandle3(this->GetBuffers());
  }
};

/// A convenience function for creating an ArrayHandleCartesianProduct. It takes the two
/// arrays to be zipped together.
///
template <typename FirstHandleType, typename SecondHandleType, typename ThirdHandleType>
VTKM_CONT
  vtkm::cont::ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>
  make_ArrayHandleCartesianProduct(const FirstHandleType& first,
                                   const SecondHandleType& second,
                                   const ThirdHandleType& third)
{
  return ArrayHandleCartesianProduct<FirstHandleType, SecondHandleType, ThirdHandleType>(
    first, second, third);
}

//--------------------------------------------------------------------------------
// Specialization of ArrayExtractComponent
namespace internal
{

// Superclass will inherit the ArrayExtractComponentImplInefficient property if any
// of the sub-storage are inefficient (thus making everything inefficient).
template <typename... STs>
struct ArrayExtractComponentImpl<vtkm::cont::StorageTagCartesianProduct<STs...>>
  : vtkm::cont::internal::ArrayExtractComponentImplInherit<STs...>
{
  template <typename T>
  vtkm::cont::ArrayHandleStride<T> AdjustStrideForComponent(
    const vtkm::cont::ArrayHandleStride<T>& componentArray,
    const vtkm::Id3& dims,
    vtkm::IdComponent component,
    vtkm::Id totalNumValues) const
  {
    VTKM_ASSERT(componentArray.GetModulo() == 0);
    VTKM_ASSERT(componentArray.GetDivisor() == 1);

    vtkm::Id modulo = 0;
    if (component < 2)
    {
      modulo = dims[component];
    }

    vtkm::Id divisor = 1;
    for (vtkm::IdComponent c = 0; c < component; ++c)
    {
      divisor *= dims[c];
    }

    return vtkm::cont::ArrayHandleStride<T>(componentArray.GetBasicArray(),
                                            totalNumValues,
                                            componentArray.GetStride(),
                                            componentArray.GetOffset(),
                                            modulo,
                                            divisor);
  }

  template <typename T, typename ST, typename CartesianArrayType>
  vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType>
  GetStrideForComponentArray(const vtkm::cont::ArrayHandle<T, ST>& componentArray,
                             const CartesianArrayType& cartesianArray,
                             vtkm::IdComponent subIndex,
                             vtkm::IdComponent productIndex,
                             vtkm::CopyFlag allowCopy) const
  {
    vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType> strideArray =
      ArrayExtractComponentImpl<ST>{}(componentArray, subIndex, allowCopy);
    if ((strideArray.GetModulo() != 0) || (strideArray.GetDivisor() != 1))
    {
      // If the sub array has its own modulo and/or divisor, that will likely interfere
      // with this math. Give up and fall back to simple copy.
      constexpr vtkm::IdComponent NUM_SUB_COMPONENTS = vtkm::VecFlat<T>::NUM_COMPONENTS;
      return vtkm::cont::internal::ArrayExtractComponentFallback(
        cartesianArray, (productIndex * NUM_SUB_COMPONENTS) + subIndex, allowCopy);
    }

    vtkm::Id3 dims = { cartesianArray.GetFirstArray().GetNumberOfValues(),
                       cartesianArray.GetSecondArray().GetNumberOfValues(),
                       cartesianArray.GetThirdArray().GetNumberOfValues() };

    return this->AdjustStrideForComponent(
      strideArray, dims, productIndex, cartesianArray.GetNumberOfValues());
  }

  template <typename T>
  vtkm::cont::ArrayHandleStride<typename vtkm::VecTraits<T>::BaseComponentType> operator()(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagCartesianProduct<STs...>>&
      src,
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag allowCopy) const
  {
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T, STs>...> array(src);
    constexpr vtkm::IdComponent NUM_SUB_COMPONENTS = vtkm::VecFlat<T>::NUM_COMPONENTS;
    vtkm::IdComponent subIndex = componentIndex % NUM_SUB_COMPONENTS;
    vtkm::IdComponent productIndex = componentIndex / NUM_SUB_COMPONENTS;

    switch (productIndex)
    {
      case 0:
        return this->GetStrideForComponentArray(
          array.GetFirstArray(), array, subIndex, productIndex, allowCopy);
      case 1:
        return this->GetStrideForComponentArray(
          array.GetSecondArray(), array, subIndex, productIndex, allowCopy);
      case 2:
        return this->GetStrideForComponentArray(
          array.GetThirdArray(), array, subIndex, productIndex, allowCopy);
      default:
        throw vtkm::cont::ErrorBadValue("Invalid component index to ArrayExtractComponent.");
    }
  }
};

template <typename T, typename S>
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeGeneric(
  const vtkm::cont::ArrayHandle<T, S>& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange,
  vtkm::cont::DeviceAdapterId device);

template <typename S>
struct ArrayRangeComputeImpl;

template <typename ST1, typename ST2, typename ST3>
struct VTKM_CONT_EXPORT ArrayRangeComputeImpl<vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>
{
  template <typename T>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> operator()(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>,
                                  vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>& input_,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
    bool computeFiniteRange,
    vtkm::cont::DeviceAdapterId device) const
  {
    if (maskArray.GetNumberOfValues() != 0)
    {
      return vtkm::cont::internal::ArrayRangeComputeGeneric(
        input_, maskArray, computeFiniteRange, device);
    }

    const auto& input =
      static_cast<const vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T, ST1>,
                                                                vtkm::cont::ArrayHandle<T, ST2>,
                                                                vtkm::cont::ArrayHandle<T, ST3>>&>(
        input_);

    vtkm::cont::ArrayHandle<vtkm::Range> ranges[3];
    ranges[0] = vtkm::cont::internal::ArrayRangeComputeImpl<ST1>{}(
      input.GetFirstArray(), maskArray, computeFiniteRange, device);
    ranges[1] = vtkm::cont::internal::ArrayRangeComputeImpl<ST2>{}(
      input.GetSecondArray(), maskArray, computeFiniteRange, device);
    ranges[2] = vtkm::cont::internal::ArrayRangeComputeImpl<ST3>{}(
      input.GetThirdArray(), maskArray, computeFiniteRange, device);

    auto numComponents =
      ranges[0].GetNumberOfValues() + ranges[1].GetNumberOfValues() + ranges[2].GetNumberOfValues();
    vtkm::cont::ArrayHandle<vtkm::Range> result;
    result.Allocate(numComponents);
    auto resultPortal = result.WritePortal();
    for (vtkm::Id i = 0, index = 0; i < 3; ++i)
    {
      auto rangePortal = ranges[i].ReadPortal();
      for (vtkm::Id j = 0; j < rangePortal.GetNumberOfValues(); ++j, ++index)
      {
        resultPortal.Set(index, rangePortal.Get(j));
      }
    }
    return result;
  }
};

} // namespace internal

}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename AH1, typename AH2, typename AH3>
struct SerializableTypeString<vtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_CartesianProduct<" + SerializableTypeString<AH1>::Get() + "," +
      SerializableTypeString<AH2>::Get() + "," + SerializableTypeString<AH3>::Get() + ">";
    return name;
  }
};

template <typename T, typename ST1, typename ST2, typename ST3>
struct SerializableTypeString<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>>
  : SerializableTypeString<vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T, ST1>,
                                                                   vtkm::cont::ArrayHandle<T, ST2>,
                                                                   vtkm::cont::ArrayHandle<T, ST3>>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename AH1, typename AH2, typename AH3>
struct Serialization<vtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>>
{
private:
  using Type = typename vtkm::cont::ArrayHandleCartesianProduct<AH1, AH2, AH3>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    Type array = obj;
    vtkmdiy::save(bb, array.GetFirstArray());
    vtkmdiy::save(bb, array.GetSecondArray());
    vtkmdiy::save(bb, array.GetThirdArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH1 array1;
    AH2 array2;
    AH3 array3;

    vtkmdiy::load(bb, array1);
    vtkmdiy::load(bb, array2);
    vtkmdiy::load(bb, array3);

    obj = vtkm::cont::make_ArrayHandleCartesianProduct(array1, array2, array3);
  }
};

template <typename T, typename ST1, typename ST2, typename ST3>
struct Serialization<
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, vtkm::cont::StorageTagCartesianProduct<ST1, ST2, ST3>>>
  : Serialization<vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<T, ST1>,
                                                          vtkm::cont::ArrayHandle<T, ST2>,
                                                          vtkm::cont::ArrayHandle<T, ST3>>>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleCartesianProduct_h
