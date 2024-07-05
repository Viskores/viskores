//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleConstant_h
#define vtk_m_cont_ArrayHandleConstant_h

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandleImplicit.h>

#include <vtkm/cont/internal/ArrayRangeComputeUtils.h>

#include <vtkm/Range.h>
#include <vtkm/VecFlat.h>
#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT StorageTagConstant
{
};

namespace internal
{

template <typename ValueType>
struct VTKM_ALWAYS_EXPORT ConstantFunctor
{
  VTKM_EXEC_CONT
  ConstantFunctor(const ValueType& value = ValueType())
    : Value(value)
  {
  }

  VTKM_EXEC_CONT
  ValueType operator()(vtkm::Id vtkmNotUsed(index)) const { return this->Value; }

private:
  ValueType Value;
};

template <typename T>
using StorageTagConstantSuperclass =
  typename vtkm::cont::ArrayHandleImplicit<ConstantFunctor<T>>::StorageTag;

template <typename T>
struct Storage<T, vtkm::cont::StorageTagConstant> : Storage<T, StorageTagConstantSuperclass<T>>
{
};

} // namespace internal

/// @brief An array handle with a constant value.
///
/// `ArrayHandleConstant` is an implicit array handle with a constant value. A
/// constant array handle is constructed by giving a value and an array length.
/// The resulting array is of the given size with each entry the same value
/// given in the constructor. The array is defined implicitly, so there it
/// takes (almost) no memory.
///
template <typename T>
class ArrayHandleConstant : public vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleConstant,
                             (ArrayHandleConstant<T>),
                             (vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>));

  /// Construct a constant array containing the given value.
  VTKM_CONT
  ArrayHandleConstant(T value, vtkm::Id numberOfValues = 0)
    : Superclass(internal::FunctorToArrayHandleImplicitBuffers(internal::ConstantFunctor<T>(value),
                                                               numberOfValues))
  {
  }

  /// @brief Returns the constant value stored in this array.
  ///
  /// The value set in the constructor of this array is returned even if the number of values is 0.
  ///
  VTKM_CONT T GetValue() const { return this->ReadPortal().GetFunctor()(0); }
};

/// `make_ArrayHandleConstant` is convenience function to generate an
/// ArrayHandleImplicit.
template <typename T>
vtkm::cont::ArrayHandleConstant<T> make_ArrayHandleConstant(T value, vtkm::Id numberOfValues)
{
  return vtkm::cont::ArrayHandleConstant<T>(value, numberOfValues);
}

namespace internal
{

template <>
struct VTKM_CONT_EXPORT ArrayExtractComponentImpl<vtkm::cont::StorageTagConstant>
{
  template <typename T>
  VTKM_CONT auto operator()(const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>& src,
                            vtkm::IdComponent componentIndex,
                            vtkm::CopyFlag allowCopy) const
  {
    if (allowCopy != vtkm::CopyFlag::On)
    {
      throw vtkm::cont::ErrorBadValue(
        "Cannot extract component of ArrayHandleConstant without copying. "
        "(However, the whole array does not need to be copied.)");
    }

    vtkm::cont::ArrayHandleConstant<T> srcArray = src;

    vtkm::VecFlat<T> vecValue{ srcArray.GetValue() };

    // Make a basic array with one entry (the constant value).
    auto basicArray = vtkm::cont::make_ArrayHandle({ vecValue[componentIndex] });

    // Set up a modulo = 1 so all indices go to this one value.
    return vtkm::cont::make_ArrayHandleStride(basicArray, src.GetNumberOfValues(), 1, 0, 1, 1);
  }
};

template <typename S>
struct ArrayRangeComputeImpl;

template <>
struct VTKM_CONT_EXPORT ArrayRangeComputeImpl<vtkm::cont::StorageTagConstant>
{
  template <typename T>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> operator()(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>& input,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
    bool computeFiniteRange,
    vtkm::cont::DeviceAdapterId devId) const
  {
    bool allMasked = false;
    if (maskArray.GetNumberOfValues() != 0)
    {
      // Find if there is atleast one value that is not masked
      auto ids = GetFirstAndLastUnmaskedIndices(maskArray, devId);
      allMasked = (ids[1] < ids[0]);
    }

    auto value = vtkm::make_VecFlat(input.ReadPortal().Get(0));

    vtkm::cont::ArrayHandle<vtkm::Range> result;
    result.Allocate(value.GetNumberOfComponents());
    auto resultPortal = result.WritePortal();
    for (vtkm::IdComponent index = 0; index < value.GetNumberOfComponents(); ++index)
    {
      auto comp = static_cast<vtkm::Float64>(value[index]);
      if (allMasked || (computeFiniteRange && !vtkm::IsFinite(comp)))
      {
        resultPortal.Set(index, vtkm::Range{});
      }
      else
      {
        resultPortal.Set(index, vtkm::Range{ comp, comp });
      }
    }
    return result;
  }
};

template <typename S>
struct ArrayRangeComputeMagnitudeImpl;

template <>
struct VTKM_CONT_EXPORT ArrayRangeComputeMagnitudeImpl<vtkm::cont::StorageTagConstant>
{
  template <typename T>
  VTKM_CONT vtkm::Range operator()(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>& input,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
    bool computeFiniteRange,
    vtkm::cont::DeviceAdapterId devId) const
  {
    if (maskArray.GetNumberOfValues() != 0)
    {
      // Find if there is atleast one value that is not masked
      auto ids = GetFirstAndLastUnmaskedIndices(maskArray, devId);
      if (ids[1] < ids[0])
      {
        return vtkm::Range{};
      }
    }

    auto value = input.ReadPortal().Get(0);
    vtkm::Float64 rangeValue = vtkm::Magnitude(vtkm::make_VecFlat(value));
    return (computeFiniteRange && !vtkm::IsFinite(rangeValue))
      ? vtkm::Range{}
      : vtkm::Range{ rangeValue, rangeValue };
  }
};

} // namespace internal

}
} // vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleConstant<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Constant<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>>
  : SerializableTypeString<vtkm::cont::ArrayHandleConstant<T>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleConstant<T>>
{
private:
  using Type = vtkm::cont::ArrayHandleConstant<T>;
  using BaseType = vtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, obj.GetNumberOfValues());
    vtkmdiy::save(bb, obj.ReadPortal().Get(0));
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::Id count = 0;
    vtkmdiy::load(bb, count);

    T value;
    vtkmdiy::load(bb, value);

    obj = vtkm::cont::make_ArrayHandleConstant(value, count);
  }
};

template <typename T>
struct Serialization<vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagConstant>>
  : Serialization<vtkm::cont::ArrayHandleConstant<T>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleConstant_h
