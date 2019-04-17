//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleCast_h
#define vtk_m_cont_ArrayHandleCast_h

#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/cont/Logging.h>

#include <vtkm/BaseComponent.h>
#include <vtkm/Range.h>

#include <limits>

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename FromType, typename ToType>
struct VTKM_ALWAYS_EXPORT Cast
{
  VTKM_EXEC_CONT
  ToType operator()(const FromType& val) const { return static_cast<ToType>(val); }
};

} // namespace internal

/// \brief Cast the values of an array to the specified type, on demand.
///
/// ArrayHandleCast is a specialization of ArrayHandleTransform. Given an ArrayHandle
/// and a type, it creates a new handle that returns the elements of the array cast
/// to the specified type.
///
template <typename T, typename ArrayHandleType>
class ArrayHandleCast
  : public vtkm::cont::ArrayHandleTransform<ArrayHandleType,
                                            internal::Cast<typename ArrayHandleType::ValueType, T>>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCast,
    (ArrayHandleCast<T, ArrayHandleType>),
    (vtkm::cont::ArrayHandleTransform<ArrayHandleType,
                                      internal::Cast<typename ArrayHandleType::ValueType, T>>));

  ArrayHandleCast(const ArrayHandleType& handle)
    : Superclass(handle)
  {
    this->ValidateTypeCast<typename ArrayHandleType::ValueType>();
  }

private:
  // Log warnings if type cast is valid but lossy:
  template <typename SrcValueType>
  VTKM_CONT static typename std::enable_if<!std::is_same<T, SrcValueType>::value>::type
  ValidateTypeCast()
  {
#ifdef VTKM_ENABLE_LOGGING
    using DstValueType = T;
    using SrcComp = typename vtkm::BaseComponent<SrcValueType>::Type;
    using DstComp = typename vtkm::BaseComponent<DstValueType>::Type;
    using SrcLimits = std::numeric_limits<SrcComp>;
    using DstLimits = std::numeric_limits<DstComp>;

    const vtkm::Range SrcRange{ SrcLimits::min(), SrcLimits::max() };
    const vtkm::Range DstRange{ DstLimits::min(), DstLimits::max() };

    const bool RangeLoss = (SrcRange.Max > DstRange.Max || SrcRange.Min < DstRange.Min);
    const bool PrecLoss = SrcLimits::digits > DstLimits::digits;

    if (RangeLoss && PrecLoss)
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces range and precision.",
                 vtkm::cont::TypeToString<SrcValueType>().c_str(),
                 vtkm::cont::TypeToString<DstValueType>().c_str());
    }
    else if (RangeLoss)
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces range.",
                 vtkm::cont::TypeToString<SrcValueType>().c_str(),
                 vtkm::cont::TypeToString<DstValueType>().c_str());
    }
    else if (PrecLoss)
    {
      VTKM_LOG_F(vtkm::cont::LogLevel::Warn,
                 "VariantArrayHandle::AsVirtual: Casting ComponentType of "
                 "%s to %s reduces precision.",
                 vtkm::cont::TypeToString<SrcValueType>().c_str(),
                 vtkm::cont::TypeToString<DstValueType>().c_str());
    }
#endif // Logging
  }

  template <typename SrcValueType>
  VTKM_CONT static typename std::enable_if<std::is_same<T, SrcValueType>::value>::type
  ValidateTypeCast()
  {
    //no-op if types match
  }
};

namespace detail
{

template <typename CastType, typename OriginalType, typename ArrayType>
struct MakeArrayHandleCastImpl
{
  using ReturnType = vtkm::cont::ArrayHandleCast<CastType, ArrayType>;

  VTKM_CONT static ReturnType DoMake(const ArrayType& array) { return ReturnType(array); }
};

template <typename T, typename ArrayType>
struct MakeArrayHandleCastImpl<T, T, ArrayType>
{
  using ReturnType = ArrayType;

  VTKM_CONT static ReturnType DoMake(const ArrayType& array) { return array; }
};

} // namespace detail

/// make_ArrayHandleCast is convenience function to generate an
/// ArrayHandleCast.
///
template <typename T, typename ArrayType>
VTKM_CONT
  typename detail::MakeArrayHandleCastImpl<T, typename ArrayType::ValueType, ArrayType>::ReturnType
  make_ArrayHandleCast(const ArrayType& array, const T& = T())
{
  VTKM_IS_ARRAY_HANDLE(ArrayType);
  using MakeImpl = detail::MakeArrayHandleCastImpl<T, typename ArrayType::ValueType, ArrayType>;
  return MakeImpl::DoMake(array);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes

namespace vtkm
{
namespace cont
{

template <typename T1, typename T2>
struct SerializableTypeString<vtkm::cont::internal::Cast<T1, T2>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Cast_Functor<" + SerializableTypeString<T1>::Get() + "," +
      SerializableTypeString<T2>::Get() + ">";
    return name;
  }
};

template <typename T, typename AH>
struct SerializableTypeString<vtkm::cont::ArrayHandleCast<T, AH>>
  : SerializableTypeString<
      vtkm::cont::ArrayHandleTransform<AH, vtkm::cont::internal::Cast<typename AH::ValueType, T>>>
{
};
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

template <typename T1, typename T2>
struct Serialization<vtkm::cont::internal::Cast<T1, T2>>
{
  static VTKM_CONT void save(BinaryBuffer&, const vtkm::cont::internal::Cast<T1, T2>&) {}

  static VTKM_CONT void load(BinaryBuffer&, vtkm::cont::internal::Cast<T1, T2>&) {}
};

template <typename T, typename AH>
struct Serialization<vtkm::cont::ArrayHandleCast<T, AH>>
  : Serialization<
      vtkm::cont::ArrayHandleTransform<AH, vtkm::cont::internal::Cast<typename AH::ValueType, T>>>
{
};

} // diy

#endif // vtk_m_cont_ArrayHandleCast_h
