//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayRangeComputeTemplate_h
#define vtk_m_cont_ArrayRangeComputeTemplate_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleDecorator.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/internal/ArrayRangeComputeUtils.h>

#include <vtkm/BinaryOperators.h>
#include <vtkm/Deprecated.h>
#include <vtkm/VecFlat.h>
#include <vtkm/VecTraits.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/internal/Instantiations.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

//-------------------------------------------------------------------------------------------------
struct ComputeRangeOptionsDecorator
{
  bool IgnoreInf = false;

  template <typename SrcPortal, typename MaskPortal>
  struct Functor
  {
    SrcPortal Src;
    MaskPortal Mask;
    bool IgnoreInf;

    using InValueType = typename SrcPortal::ValueType;
    using InVecTraits = vtkm::VecTraits<InValueType>;
    using ResultType = vtkm::Vec<vtkm::Vec<vtkm::Float64, InVecTraits::NUM_COMPONENTS>, 2>;

    VTKM_EXEC_CONT
    ResultType operator()(vtkm::Id idx) const
    {
      if ((this->Mask.GetNumberOfValues() != 0) && (this->Mask.Get(idx) == 0))
      {
        return { { vtkm::Range{}.Min }, { vtkm::Range{}.Max } };
      }

      const auto& inVal = this->Src.Get(idx);
      ResultType outVal;
      for (vtkm::IdComponent i = 0; i < InVecTraits::NUM_COMPONENTS; ++i)
      {
        auto val = static_cast<vtkm::Float64>(InVecTraits::GetComponent(inVal, i));
        if (vtkm::IsNan(val) || (this->IgnoreInf && !vtkm::IsFinite(val)))
        {
          outVal[0][i] = vtkm::Range{}.Min;
          outVal[1][i] = vtkm::Range{}.Max;
        }
        else
        {
          outVal[0][i] = outVal[1][i] = val;
        }
      }

      return outVal;
    }
  };

  template <typename SrcPortal, typename GhostPortal>
  Functor<SrcPortal, GhostPortal> CreateFunctor(const SrcPortal& sp, const GhostPortal& gp) const
  {
    return { sp, gp, this->IgnoreInf };
  }
};

template <typename ArrayHandleType>
struct ArrayValueIsNested
{
  static constexpr bool Value =
    !vtkm::internal::IsFlatVec<typename ArrayHandleType::ValueType>::value;
};

template <typename ArrayHandleType, bool IsNested = ArrayValueIsNested<ArrayHandleType>::Value>
struct NestedToFlat;

template <typename ArrayHandleType>
struct NestedToFlat<ArrayHandleType, true>
{
  static auto Transform(const ArrayHandleType& in)
  {
    return vtkm::cont::ArrayHandleCast<vtkm::VecFlat<typename ArrayHandleType::ValueType>,
                                       ArrayHandleType>(in);
  }
};

template <typename ArrayHandleType>
struct NestedToFlat<ArrayHandleType, false>
{
  static auto Transform(const ArrayHandleType& in) { return in; }
};

template <typename ArrayHandleType>
inline auto NestedToFlatTransform(const ArrayHandleType& input)
{
  return NestedToFlat<ArrayHandleType>::Transform(input);
}

//-------------------------------------------------------------------------------------------------
/// \brief A generic implementation of `ArrayRangeCompute`. This is the implementation used
/// when `ArrayRangeComputeImpl` is not specialized.
///
template <typename T, typename S>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeGeneric(
  const vtkm::cont::ArrayHandle<T, S>& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange,
  vtkm::cont::DeviceAdapterId device)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "ArrayRangeCompute");

  using VecTraits = vtkm::VecTraits<T>;

  vtkm::cont::ArrayHandle<vtkm::Range> range;
  range.Allocate(VecTraits::NUM_COMPONENTS);

  //We want to minimize the amount of code that we do in try execute as
  //it is repeated for each
  if (input.GetNumberOfValues() < 1)
  {
    range.Fill(vtkm::Range{});
  }
  else
  {
    // if input is an array of nested vectors, transform them to `VecFlat` using ArrayHandleCast
    auto flattened = NestedToFlatTransform(input);
    ComputeRangeOptionsDecorator decorator{ computeFiniteRange };
    auto decorated =
      make_ArrayHandleDecorator(flattened.GetNumberOfValues(), decorator, flattened, maskArray);

    using ResultType = vtkm::Vec<vtkm::Vec<vtkm::Float64, VecTraits::NUM_COMPONENTS>, 2>;
    using MinAndMaxFunctor = vtkm::MinAndMax<typename ResultType::ComponentType>;
    ResultType identity{ { vtkm::Range{}.Min }, { vtkm::Range{}.Max } };

    auto result = vtkm::cont::Algorithm::Reduce(device, decorated, identity, MinAndMaxFunctor{});

    auto portal = range.WritePortal();
    for (vtkm::IdComponent i = 0; i < VecTraits::NUM_COMPONENTS; ++i)
    {
      portal.Set(i, vtkm::Range(result[0][i], result[1][i]));
    }
  }

  return range;
}

//-------------------------------------------------------------------------------------------------
struct ScalarMagnitudeFunctor
{
  template <typename T>
  VTKM_EXEC_CONT vtkm::Float64 operator()(const T& val) const
  {
    // spcilization of `vtkm::Magnitude` for scalars should avoid `sqrt` computation by using `abs`
    // instead
    return static_cast<vtkm::Float64>(vtkm::Magnitude(val));
  }
};

struct MagnitudeSquareFunctor
{
  template <typename T>
  VTKM_EXEC_CONT vtkm::Float64 operator()(const T& val) const
  {
    using VecTraits = vtkm::VecTraits<T>;
    vtkm::Float64 result = 0;
    for (vtkm::IdComponent i = 0; i < VecTraits::GetNumberOfComponents(val); ++i)
    {
      auto comp = static_cast<vtkm::Float64>(VecTraits::GetComponent(val, i));
      result += comp * comp;
    }
    return result;
  }
};

template <typename ArrayHandleType>
vtkm::Range ArrayRangeComputeMagnitudeGenericImpl(
  vtkm::VecTraitsTagSingleComponent,
  const ArrayHandleType& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange,
  vtkm::cont::DeviceAdapterId device)
{
  auto mag = vtkm::cont::make_ArrayHandleTransform(input, ScalarMagnitudeFunctor{});
  auto rangeAH = ArrayRangeComputeGeneric(mag, maskArray, computeFiniteRange, device);
  return rangeAH.ReadPortal().Get(0);
}

template <typename ArrayHandleType>
vtkm::Range ArrayRangeComputeMagnitudeGenericImpl(
  vtkm::VecTraitsTagMultipleComponents,
  const ArrayHandleType& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange,
  vtkm::cont::DeviceAdapterId device)
{
  auto magsqr = vtkm::cont::make_ArrayHandleTransform(input, MagnitudeSquareFunctor{});
  auto rangeAH = ArrayRangeComputeGeneric(magsqr, maskArray, computeFiniteRange, device);
  auto range = rangeAH.ReadPortal().Get(0);
  if (range.IsNonEmpty())
  {
    range.Min = vtkm::Sqrt(range.Min);
    range.Max = vtkm::Sqrt(range.Max);
  }
  return range;
}

/// \brief A generic implementation of `ArrayRangeComputeMagnitude`. This is the implementation used
/// when `ArrayRangeComputeMagnitudeImpl` is not specialized.
///
template <typename T, typename S>
inline vtkm::Range ArrayRangeComputeMagnitudeGeneric(
  const vtkm::cont::ArrayHandle<T, S>& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange,
  vtkm::cont::DeviceAdapterId device)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "ArrayRangeComputeMagnitude");

  using VecTraits = vtkm::VecTraits<T>;

  //We want to minimize the amount of code that we do in try execute as
  //it is repeated for each
  if (input.GetNumberOfValues() < 1)
  {
    return vtkm::Range{};
  }

  auto flattened = NestedToFlatTransform(input);
  return ArrayRangeComputeMagnitudeGenericImpl(
    typename VecTraits::HasMultipleComponents{}, flattened, maskArray, computeFiniteRange, device);
}

//-------------------------------------------------------------------------------------------------
template <typename S>
struct ArrayRangeComputeImpl
{
  template <typename T>
  vtkm::cont::ArrayHandle<vtkm::Range> operator()(
    const vtkm::cont::ArrayHandle<T, S>& input,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
    bool computeFiniteRange,
    vtkm::cont::DeviceAdapterId device) const
  {
    return vtkm::cont::internal::ArrayRangeComputeGeneric(
      input, maskArray, computeFiniteRange, device);
  }
};

template <typename S>
struct ArrayRangeComputeMagnitudeImpl
{
  template <typename T>
  vtkm::Range operator()(const vtkm::cont::ArrayHandle<T, S>& input,
                         const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
                         bool computeFiniteRange,
                         vtkm::cont::DeviceAdapterId device) const
  {
    return vtkm::cont::internal::ArrayRangeComputeMagnitudeGeneric(
      input, maskArray, computeFiniteRange, device);
  }
};

} // namespace internal

//-------------------------------------------------------------------------------------------------
/// @{
/// \brief Templated version of ArrayRangeCompute
/// \sa ArrayRangeCompute
///
template <typename T, typename S>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeTemplate(
  const vtkm::cont::ArrayHandle<T, S>& input,
  bool computeFiniteRange = false,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{})
{
  return ArrayRangeComputeTemplate(
    input, vtkm::cont::ArrayHandle<vtkm::UInt8>{}, computeFiniteRange, device);
}

template <typename T, typename S>
vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeTemplate(
  const vtkm::cont::ArrayHandle<T, S>& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange = false,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{})
{
  VTKM_ASSERT(maskArray.GetNumberOfValues() == 0 ||
              maskArray.GetNumberOfValues() == input.GetNumberOfValues());
  return internal::ArrayRangeComputeImpl<S>{}(input, maskArray, computeFiniteRange, device);
}

template <typename T, typename S>
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeComputeTemplate(
  const vtkm::cont::ArrayHandle<T, S>& input,
  vtkm::cont::DeviceAdapterId device)
{
  return ArrayRangeComputeTemplate(input, false, device);
}

/// @}

/// @{
/// \brief Templated version of ArrayRangeComputeMagnitude
/// \sa ArrayRangeComputeMagnitude
///
template <typename T, typename S>
inline vtkm::Range ArrayRangeComputeMagnitudeTemplate(
  const vtkm::cont::ArrayHandle<T, S>& input,
  bool computeFiniteRange = false,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{})
{
  return ArrayRangeComputeMagnitudeTemplate(
    input, vtkm::cont::ArrayHandle<vtkm::UInt8>{}, computeFiniteRange, device);
}

template <typename T, typename S>
vtkm::Range ArrayRangeComputeMagnitudeTemplate(
  const vtkm::cont::ArrayHandle<T, S>& input,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& maskArray,
  bool computeFiniteRange = false,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{})
{
  VTKM_ASSERT(maskArray.GetNumberOfValues() == 0 ||
              maskArray.GetNumberOfValues() == input.GetNumberOfValues());
  return internal::ArrayRangeComputeMagnitudeImpl<S>{}(
    input, maskArray, computeFiniteRange, device);
}

template <typename T, typename S>
inline vtkm::Range ArrayRangeComputeMagnitudeTemplate(const vtkm::cont::ArrayHandle<T, S>& input,
                                                      vtkm::cont::DeviceAdapterId device)
{
  return ArrayRangeComputeMagnitudeTemplate(input, false, device);
}
/// @}

//-----------------------------------------------------------------------------
template <typename ArrayHandleType>
VTKM_DEPRECATED(2.1, "Use precompiled ArrayRangeCompute or ArrayRangeComputeTemplate.")
inline vtkm::cont::ArrayHandle<vtkm::Range> ArrayRangeCompute(
  const ArrayHandleType& input,
  vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny{})
{
  return ArrayRangeComputeTemplate(input, false, device);
}

}
} // namespace vtkm::cont

#define VTK_M_ARRAY_RANGE_COMPUTE_DCLR(...)                                   \
  vtkm::cont::ArrayHandle<vtkm::Range> vtkm::cont::ArrayRangeComputeTemplate( \
    const vtkm::cont::ArrayHandle<__VA_ARGS__>&,                              \
    const vtkm::cont::ArrayHandle<vtkm::UInt8>&,                              \
    bool,                                                                     \
    vtkm::cont::DeviceAdapterId)

#define VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(...)               \
  vtkm::Range vtkm::cont::ArrayRangeComputeMagnitudeTemplate( \
    const vtkm::cont::ArrayHandle<__VA_ARGS__>&,              \
    const vtkm::cont::ArrayHandle<vtkm::UInt8>&,              \
    bool,                                                     \
    vtkm::cont::DeviceAdapterId)

#define VTK_M_ARRAY_RANGE_COMPUTE_INT_SCALARS(modifiers, ...)              \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Int8, __VA_ARGS__);       \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Int8, __VA_ARGS__);   \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::UInt8, __VA_ARGS__);      \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::UInt8, __VA_ARGS__);  \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Int16, __VA_ARGS__);      \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Int16, __VA_ARGS__);  \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::UInt16, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::UInt16, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Int32, __VA_ARGS__);      \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Int32, __VA_ARGS__);  \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::UInt32, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::UInt32, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Int64, __VA_ARGS__);      \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Int64, __VA_ARGS__);  \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::UInt64, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::UInt64, __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_SCALARS(modifiers, ...)             \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Float32, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Float32, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Float64, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Float64, __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_BOOL_SCALARS(modifiers, ...) \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(bool, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(bool, __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_OTHER_SCALARS(modifiers, ...)                           \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(char, __VA_ARGS__);                            \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(char, __VA_ARGS__);                        \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(signed VTKM_UNUSED_INT_TYPE, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(signed VTKM_UNUSED_INT_TYPE, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(unsigned VTKM_UNUSED_INT_TYPE, __VA_ARGS__);   \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(unsigned VTKM_UNUSED_INT_TYPE, __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_ALL_SCALARS(modifiers, ...)      \
  VTK_M_ARRAY_RANGE_COMPUTE_INT_SCALARS(modifiers, __VA_ARGS__);   \
  VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_SCALARS(modifiers, __VA_ARGS__); \
  VTK_M_ARRAY_RANGE_COMPUTE_BOOL_SCALARS(modifiers, __VA_ARGS__);  \
  VTK_M_ARRAY_RANGE_COMPUTE_OTHER_SCALARS(modifiers, __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_INT_VECN(modifiers, N, ...)                            \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::Int8, N>, __VA_ARGS__);       \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::Int8, N>, __VA_ARGS__);   \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::UInt8, N>, __VA_ARGS__);      \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::UInt8, N>, __VA_ARGS__);  \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::Int16, N>, __VA_ARGS__);      \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::Int16, N>, __VA_ARGS__);  \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::UInt16, N>, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::UInt16, N>, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::Int32, N>, __VA_ARGS__);      \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::Int32, N>, __VA_ARGS__);  \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::UInt32, N>, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::UInt32, N>, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::Int64, N>, __VA_ARGS__);      \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::Int64, N>, __VA_ARGS__);  \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::UInt64, N>, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::UInt64, N>, __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_VECN(modifiers, N, ...)                           \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::Float32, N>, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::Float32, N>, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<vtkm::Float64, N>, __VA_ARGS__);     \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<vtkm::Float64, N>, __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_BOOL_VECN(modifiers, N, ...)               \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<bool, N>, __VA_ARGS__); \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<bool, N>, __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_OTHER_VECN(modifiers, N, ...)                             \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<char, N>, __VA_ARGS__);                \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<char, N>, __VA_ARGS__);            \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<signed VTKM_UNUSED_INT_TYPE, N>,       \
                                           __VA_ARGS__);                                    \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<signed VTKM_UNUSED_INT_TYPE, N>,   \
                                               __VA_ARGS__);                                \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_DCLR(vtkm::Vec<unsigned VTKM_UNUSED_INT_TYPE, N>,     \
                                           __VA_ARGS__);                                    \
  modifiers VTK_M_ARRAY_RANGE_COMPUTE_MAG_DCLR(vtkm::Vec<unsigned VTKM_UNUSED_INT_TYPE, N>, \
                                               __VA_ARGS__)

#define VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(modifiers, N, ...)      \
  VTK_M_ARRAY_RANGE_COMPUTE_INT_VECN(modifiers, N, __VA_ARGS__);   \
  VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_VECN(modifiers, N, __VA_ARGS__); \
  VTK_M_ARRAY_RANGE_COMPUTE_BOOL_VECN(modifiers, N, __VA_ARGS__);  \
  VTK_M_ARRAY_RANGE_COMPUTE_OTHER_VECN(modifiers, N, __VA_ARGS__)

namespace vtkm
{
namespace cont
{

struct StorageTagSOA;

template <typename ST1, typename ST2, typename ST3>
struct StorageTagCartesianProduct;

struct StorageTagConstant;

struct StorageTagCounting;

struct StorageTagXGCCoordinates;

struct StorageTagStride;

}
} // vtkm::cont

//-------------------------------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_SCALARS(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                      vtkm::cont::StorageTagBasic);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   2,
                                   vtkm::cont::StorageTagBasic);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   3,
                                   vtkm::cont::StorageTagBasic);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   4,
                                   vtkm::cont::StorageTagBasic);
VTKM_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   2,
                                   vtkm::cont::StorageTagSOA);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   3,
                                   vtkm::cont::StorageTagSOA);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   4,
                                   vtkm::cont::StorageTagSOA);
VTKM_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_VECN(
  extern template VTKM_CONT_TEMPLATE_EXPORT,
  3,
  vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic,
                                         vtkm::cont::StorageTagBasic>);
VTKM_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                     3,
                                     StorageTagXGCCoordinates);
VTKM_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_SCALARS(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                      vtkm::cont::StorageTagConstant);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   2,
                                   vtkm::cont::StorageTagConstant);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   3,
                                   vtkm::cont::StorageTagConstant);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   4,
                                   vtkm::cont::StorageTagConstant);
VTKM_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_INT_SCALARS(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                      vtkm::cont::StorageTagCounting);
VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_SCALARS(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                        vtkm::cont::StorageTagCounting);
VTK_M_ARRAY_RANGE_COMPUTE_OTHER_SCALARS(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                        vtkm::cont::StorageTagCounting);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_INT_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   2,
                                   vtkm::cont::StorageTagCounting);
VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                     2,
                                     vtkm::cont::StorageTagCounting);
VTK_M_ARRAY_RANGE_COMPUTE_OTHER_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                     2,
                                     vtkm::cont::StorageTagCounting);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_INT_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   3,
                                   vtkm::cont::StorageTagCounting);
VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                     3,
                                     vtkm::cont::StorageTagCounting);
VTK_M_ARRAY_RANGE_COMPUTE_OTHER_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                     3,
                                     vtkm::cont::StorageTagCounting);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_INT_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                   4,
                                   vtkm::cont::StorageTagCounting);
VTK_M_ARRAY_RANGE_COMPUTE_FLOAT_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                     4,
                                     vtkm::cont::StorageTagCounting);
VTK_M_ARRAY_RANGE_COMPUTE_OTHER_VECN(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                     4,
                                     vtkm::cont::StorageTagCounting);
VTKM_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VTKM_INSTANTIATION_BEGIN
VTK_M_ARRAY_RANGE_COMPUTE_ALL_SCALARS(extern template VTKM_CONT_TEMPLATE_EXPORT,
                                      vtkm::cont::StorageTagStride);
VTKM_INSTANTIATION_END

#endif //vtk_m_cont_ArrayRangeComputeTemplate_h
