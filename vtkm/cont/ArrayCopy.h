//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayCopy_h
#define vtk_m_cont_ArrayCopy_h

#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/internal/MapArrayPermutation.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename S>
struct ArrayCopyConcreteSrc;

template <typename SrcIsArrayHandle>
inline void ArrayCopyImpl(const vtkm::cont::UnknownArrayHandle& source,
                          vtkm::cont::UnknownArrayHandle& destination,
                          SrcIsArrayHandle,
                          std::false_type)
{
  destination.DeepCopyFrom(source);
}
template <typename SrcIsArrayHandle>
inline void ArrayCopyImpl(const vtkm::cont::UnknownArrayHandle& source,
                          const vtkm::cont::UnknownArrayHandle& destination,
                          SrcIsArrayHandle,
                          std::false_type)
{
  destination.DeepCopyFrom(source);
}

template <typename T, typename S>
void ArrayCopyImpl(const vtkm::cont::UnknownArrayHandle& source,
                   vtkm::cont::ArrayHandle<T, S>& destination,
                   std::false_type,
                   std::true_type)
{
  using DestType = vtkm::cont::ArrayHandle<T, S>;
  if (source.CanConvert<DestType>())
  {
    destination.DeepCopyFrom(source.AsArrayHandle<DestType>());
  }
  else
  {
    vtkm::cont::UnknownArrayHandle destWrapper(destination);
    vtkm::cont::detail::ArrayCopyImpl(source, destWrapper, std::false_type{}, std::false_type{});
    // Destination array should not change, but just in case.
    destWrapper.AsArrayHandle(destination);
  }
}

template <typename TS, typename SS, typename TD, typename SD>
void ArrayCopyImpl(const vtkm::cont::ArrayHandle<TS, SS>& source,
                   vtkm::cont::ArrayHandle<TD, SD>& destination,
                   std::true_type,
                   std::true_type)
{
  detail::ArrayCopyConcreteSrc<SS>{}(source, destination);
}

// Special case of copying data when type is the same.
template <typename T, typename S>
void ArrayCopyImpl(const vtkm::cont::ArrayHandle<T, S>& source,
                   vtkm::cont::ArrayHandle<T, S>& destination,
                   std::true_type,
                   std::true_type)
{
  destination.DeepCopyFrom(source);
}

}

/// \brief Does a deep copy from one array to another array.
///
/// Given a source `ArrayHandle` and a destination `ArrayHandle`, this
/// function allocates the destination `ArrayHandle` to the correct size and
/// deeply copies all the values from the source to the destination.
///
/// This method will attempt to copy the data using the device that the input
/// data is already valid on. If the input data is only valid in the control
/// environment, the runtime device tracker is used to try to find another
/// device.
///
/// This should work on some non-writable array handles as well, as long as
/// both \a source and \a destination are the same type.
///
/// This version of array copy uses a precompiled version of copy that is
/// efficient for most standard memory layouts. However, there are some
/// types of fancy `ArrayHandle` that cannot be handled directly, and
/// the fallback for these arrays can be slow. If you see a warning in
/// the log about an inefficient memory copy when extracting a component,
/// pay heed and look for a different way to copy the data (perhaps
/// using `ArrayCopyDevice`).
///
template <typename SourceArrayType, typename DestArrayType>
inline void ArrayCopy(const SourceArrayType& source, DestArrayType& destination)
{
  detail::ArrayCopyImpl(source,
                        destination,
                        typename internal::ArrayHandleCheck<SourceArrayType>::type{},
                        typename internal::ArrayHandleCheck<DestArrayType>::type{});
}

// Special case where we allow a const UnknownArrayHandle as output.
/// @copydoc ArrayCopy
template <typename SourceArrayType>
inline void ArrayCopy(const SourceArrayType& source, vtkm::cont::UnknownArrayHandle& destination)
{
  detail::ArrayCopyImpl(source,
                        destination,
                        typename internal::ArrayHandleCheck<SourceArrayType>::type{},
                        std::false_type{});
}

// Invalid const ArrayHandle in destination, which is not allowed because it will
// not work in all cases.
template <typename T, typename S>
void ArrayCopy(const vtkm::cont::UnknownArrayHandle&, const vtkm::cont::ArrayHandle<T, S>&)
{
  VTKM_STATIC_ASSERT_MSG(sizeof(T) == 0, "Copying to a constant ArrayHandle is not allowed.");
}

/// \brief Copies from an unknown to a known array type.
///
/// Often times you have an array of an unknown type (likely from a data set),
/// and you need it to be of a particular type (or can make a reasonable but uncertain
/// assumption about it being a particular type). You really just want a shallow
/// copy (a reference in a concrete `ArrayHandle`) if that is possible.
///
/// `ArrayCopyShallowIfPossible` pulls an array of a specific type from an
/// `UnknownArrayHandle`. If the type is compatible, it will perform a shallow copy.
/// If it is not possible, a deep copy is performed to get it to the correct type.
///
template <typename T, typename S>
VTKM_CONT void ArrayCopyShallowIfPossible(const vtkm::cont::UnknownArrayHandle source,
                                          vtkm::cont::ArrayHandle<T, S>& destination)
{
  using DestType = vtkm::cont::ArrayHandle<T, S>;
  if (source.CanConvert<DestType>())
  {
    source.AsArrayHandle(destination);
  }
  else
  {
    vtkm::cont::UnknownArrayHandle destWrapper(destination);
    vtkm::cont::ArrayCopy(source, destWrapper);
    // Destination array should not change, but just in case.
    destWrapper.AsArrayHandle(destination);
  }
}

namespace detail
{

template <typename S>
struct ArrayCopyConcreteSrc
{
  template <typename T, typename DestArray>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& source, DestArray& destination) const
  {
    using ArrayType = vtkm::cont::ArrayHandle<T, S>;
    this->DoIt(
      source, destination, vtkm::cont::internal::ArrayExtractComponentIsInefficient<ArrayType>{});
  }

  template <typename T, typename DestArray>
  void DoIt(const vtkm::cont::ArrayHandle<T, S>& source,
            DestArray& destination,
            std::false_type vtkmNotUsed(isInefficient)) const
  {
    vtkm::cont::ArrayCopy(vtkm::cont::UnknownArrayHandle{ source }, destination);
  }

  template <typename T, typename DestArray>
  void DoIt(const vtkm::cont::ArrayHandle<T, S>& source,
            DestArray& destination,
            std::true_type vtkmNotUsed(isInefficient)) const
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "Attempting to copy from an array of type " +
                 vtkm::cont::TypeToString<vtkm::cont::ArrayHandle<T, S>>() +
                 " with ArrayCopy is inefficient. It is highly recommended you use another method "
                 "such as vtkm::cont::ArrayCopyDevice.");
    // Still call the precompiled `ArrayCopy`. You will get another warning after this,
    // but it will still technically work, albiet slowly.
    vtkm::cont::ArrayCopy(vtkm::cont::UnknownArrayHandle{ source }, destination);
  }
};

// Special case for constant arrays to be efficient.
template <>
struct ArrayCopyConcreteSrc<vtkm::cont::StorageTagConstant>
{
  template <typename T1, typename T2, typename S2>
  void operator()(const vtkm::cont::ArrayHandle<T1, vtkm::cont::StorageTagConstant>& source_,
                  vtkm::cont::ArrayHandle<T2, S2>& destination) const
  {
    vtkm::cont::ArrayHandleConstant<T1> source = source_;
    destination.AllocateAndFill(source.GetNumberOfValues(), static_cast<T2>(source.GetValue()));
  }
};

// Special case for ArrayHandleIndex to be efficient.
template <>
struct ArrayCopyConcreteSrc<vtkm::cont::StorageTagIndex>
{
  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandleIndex& source,
                  vtkm::cont::ArrayHandle<T, S>& destination) const
  {
    // Skip warning about inefficient copy because there is a special case in ArrayCopyUnknown.cxx
    // to copy ArrayHandleIndex efficiently.
    vtkm::cont::ArrayCopy(vtkm::cont::UnknownArrayHandle{ source }, destination);
  }
};

// Special case for ArrayHandleCounting to be efficient.
template <>
struct VTKM_CONT_EXPORT ArrayCopyConcreteSrc<vtkm::cont::StorageTagCounting>
{
  template <typename T1, typename T2, typename S2>
  void operator()(const vtkm::cont::ArrayHandle<T1, vtkm::cont::StorageTagCounting>& source,
                  vtkm::cont::ArrayHandle<T2, S2>& destination) const
  {
    vtkm::cont::ArrayHandleCounting<T1> countingSource = source;
    T1 start = countingSource.GetStart();
    T1 step = countingSource.GetStep();
    vtkm::Id size = countingSource.GetNumberOfValues();
    destination.Allocate(size);
    vtkm::cont::UnknownArrayHandle unknownDest = destination;

    using VTraits1 = vtkm::VecTraits<T1>;
    using VTraits2 = vtkm::VecTraits<T2>;
    for (vtkm::IdComponent comp = 0; comp < VTraits1::GetNumberOfComponents(start); ++comp)
    {
      this->CopyCountingFloat(
        static_cast<vtkm::FloatDefault>(VTraits1::GetComponent(start, comp)),
        static_cast<vtkm::FloatDefault>(VTraits1::GetComponent(step, comp)),
        size,
        unknownDest.ExtractComponent<typename VTraits2::BaseComponentType>(comp));
    }
  }

  void operator()(const vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagCounting>& source,
                  vtkm::cont::ArrayHandle<vtkm::Id>& destination) const
  {
    destination = this->CopyCountingId(source);
  }

private:
  void CopyCountingFloat(vtkm::FloatDefault start,
                         vtkm::FloatDefault step,
                         vtkm::Id size,
                         const vtkm::cont::UnknownArrayHandle& result) const;
  vtkm::cont::ArrayHandle<Id> CopyCountingId(
    const vtkm::cont::ArrayHandleCounting<vtkm::Id>& source) const;
};

// Special case for ArrayHandleConcatenate to be efficient
template <typename ST1, typename ST2>
struct ArrayCopyConcreteSrc<vtkm::cont::StorageTagConcatenate<ST1, ST2>>
{
  template <typename SourceArrayType, typename DestArrayType>
  void operator()(const SourceArrayType& source, DestArrayType& destination) const
  {
    auto source1 = source.GetStorage().GetArray1(source.GetBuffers());
    auto source2 = source.GetStorage().GetArray2(source.GetBuffers());

    // Need to preallocate because view decorator will not be able to resize.
    destination.Allocate(source.GetNumberOfValues());
    auto dest1 = vtkm::cont::make_ArrayHandleView(destination, 0, source1.GetNumberOfValues());
    auto dest2 = vtkm::cont::make_ArrayHandleView(
      destination, source1.GetNumberOfValues(), source2.GetNumberOfValues());

    vtkm::cont::ArrayCopy(source1, dest1);
    vtkm::cont::ArrayCopy(source2, dest2);
  }
};

// Special case for ArrayHandlePermutation to be efficient
template <typename SIndex, typename SValue>
struct ArrayCopyConcreteSrc<vtkm::cont::StorageTagPermutation<SIndex, SValue>>
{
  using SourceStorageTag = vtkm::cont::StorageTagPermutation<SIndex, SValue>;
  template <typename T1, typename T2, typename S2>
  void operator()(const vtkm::cont::ArrayHandle<T1, SourceStorageTag>& source,
                  vtkm::cont::ArrayHandle<T2, S2>& destination) const
  {
    auto indexArray = source.GetStorage().GetIndexArray(source.GetBuffers());
    auto valueArray = source.GetStorage().GetValueArray(source.GetBuffers());
    vtkm::cont::UnknownArrayHandle copy =
      vtkm::cont::internal::MapArrayPermutation(valueArray, indexArray);
    vtkm::cont::ArrayCopyShallowIfPossible(copy, destination);
  }
};

} // namespace detail

} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_ArrayCopy_h
