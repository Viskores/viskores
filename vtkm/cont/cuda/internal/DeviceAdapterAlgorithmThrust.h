//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterThrust_h
#define vtk_m_cont_cuda_internal_DeviceAdapterThrust_h

#include <vtkm/cont/cuda/internal/MakeThrustIterator.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>
#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

// Disable GCC warnings we check Dax for but Thrust does not.
#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif // gcc version >= 4.6
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 2)
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif // gcc version >= 4.2
#endif // gcc && !CUDA

#include <thrust/advance.h>
#include <thrust/binary_search.h>
#include <thrust/copy.h>
#include <thrust/count.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/unique.h>
#include <thrust/system/cuda/vector.h>

#include <thrust/iterator/counting_iterator.h>

#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif // gcc version >= 4.6
#endif // gcc && !CUDA

namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {

/// This class can be subclassed to implement the DeviceAdapterAlgorithm for a
/// device that uses thrust as its implementation. The subclass should pass in
/// the correct device adapter tag as the template parameter.
///
template<class DeviceAdapterTag>
struct DeviceAdapterAlgorithmThrust
{
  // Because of some funny code conversions in nvcc, kernels for devices have to
  // be public.
  #ifndef VTKM_CUDA
private:
  #endif
  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static void CopyPortal(const InputPortal &input,
                                         const OutputPortal &output)
  {
    ::thrust::copy(IteratorBegin(input),
                   IteratorEnd(input),
                   IteratorBegin(output));
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal>
  VTKM_CONT_EXPORT static void LowerBoundsPortal(const InputPortal &input,
                                                const ValuesPortal &values,
                                                const OutputPortal &output)
  {
    ::thrust::lower_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output));
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal,
           class Compare>
  VTKM_CONT_EXPORT static void LowerBoundsPortal(const InputPortal &input,
                                                const ValuesPortal &values,
                                                const OutputPortal &output,
                                                Compare comp)
  {
    ::thrust::lower_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output),
                          comp);
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void LowerBoundsPortal(const InputPortal &input,
                         const OutputPortal &values_output)
  {
    ::thrust::lower_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values_output),
                          IteratorEnd(values_output),
                          IteratorBegin(values_output));
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ScanExclusivePortal(const InputPortal &input,
                                                      const OutputPortal &output)
  {
    // Use iterator to get value so that thrust device_ptr has chance to handle
    // data on device.
    typename InputPortal::ValueType inputEnd = *(IteratorEnd(input) - 1);

    ::thrust::exclusive_scan(IteratorBegin(input),
                             IteratorEnd(input),
                             IteratorBegin(output));

    //return the value at the last index in the array, as that is the sum
    return *(IteratorEnd(output) - 1) + inputEnd;
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ScanInclusivePortal(const InputPortal &input,
                                                      const OutputPortal &output)
  {
    ::thrust::inclusive_scan(IteratorBegin(input),
                             IteratorEnd(input),
                             IteratorBegin(output));

    //return the value at the last index in the array, as that is the sum
    return *(IteratorEnd(output) - 1);
  }

  template<class ValuesPortal>
  VTKM_CONT_EXPORT static void SortPortal(const ValuesPortal &values)
  {
    ::thrust::sort(IteratorBegin(values),
                   IteratorEnd(values));
  }

  template<class ValuesPortal, class Compare>
  VTKM_CONT_EXPORT static void SortPortal(const ValuesPortal &values,
                                         Compare comp)
  {
    ::thrust::sort(IteratorBegin(values),
                   IteratorEnd(values),
                   comp);
  }


  template<class KeysPortal, class ValuesPortal>
  VTKM_CONT_EXPORT static void SortByKeyPortal(const KeysPortal &keys,
                                              const ValuesPortal &values)
  {
    ::thrust::sort_by_key(IteratorBegin(keys),
                          IteratorEnd(keys),
                          IteratorBegin(values));
  }

  template<class KeysPortal, class ValuesPortal, class Compare>
  VTKM_CONT_EXPORT static void SortByKeyPortal(const KeysPortal &keys,
                                              const ValuesPortal &values,
                                              Compare comp)
  {
    ::thrust::sort_by_key(IteratorBegin(keys),
                          IteratorEnd(keys),
                          IteratorBegin(values),
                          comp);
  }



  template<class StencilPortal>
  VTKM_CONT_EXPORT static vtkm::Id CountIfPortal(const StencilPortal &stencil)
  {
    typedef typename StencilPortal::ValueType ValueType;
    return ::thrust::count_if(IteratorBegin(stencil),
                              IteratorEnd(stencil),
                              ::vtkm::not_default_constructor<ValueType>());
  }

  template<class ValueIterator,
           class StencilPortal,
           class OutputPortal>
  VTKM_CONT_EXPORT static void CopyIfPortal(ValueIterator valuesBegin,
                                           ValueIterator valuesEnd,
                                           const StencilPortal &stencil,
                                           const OutputPortal &output)
  {
    typedef typename StencilPortal::ValueType ValueType;
    ::thrust::copy_if(valuesBegin,
                      valuesEnd,
                      IteratorBegin(stencil),
                      IteratorBegin(output),
                      ::vtkm::not_default_constructor<ValueType>());
  }

  template<class ValueIterator,
           class StencilArrayHandle,
           class OutputArrayHandle>
  VTKM_CONT_EXPORT static void RemoveIf(ValueIterator valuesBegin,
                                       ValueIterator valuesEnd,
                                       const StencilArrayHandle& stencil,
                                       OutputArrayHandle& output)
  {
    vtkm::Id numLeft = CountIfPortal(stencil.PrepareForInput(DeviceAdapterTag()));

    CopyIfPortal(valuesBegin,
                 valuesEnd,
                 stencil.PrepareForInput(DeviceAdapterTag()),
                 output.PrepareForOutput(numLeft, DeviceAdapterTag()));
  }

  template<class InputPortal,
           class StencilArrayHandle,
           class OutputArrayHandle>
  VTKM_CONT_EXPORT static
  void StreamCompactPortal(const InputPortal& inputPortal,
                           const StencilArrayHandle &stencil,
                           OutputArrayHandle& output)
  {
    RemoveIf(IteratorBegin(inputPortal),
             IteratorEnd(inputPortal),
             stencil,
             output);
  }

  template<class ValuesPortal>
  VTKM_CONT_EXPORT static
  vtkm::Id UniquePortal(const ValuesPortal values)
  {
    typedef typename detail::IteratorTraits<ValuesPortal>::IteratorType
                                                            IteratorType;
    IteratorType begin = IteratorBegin(values);
    IteratorType newLast = ::thrust::unique(begin, IteratorEnd(values));
    return ::thrust::distance(begin, newLast);
  }

  template<class ValuesPortal, class Compare>
  VTKM_CONT_EXPORT static
  vtkm::Id UniquePortal(const ValuesPortal values, Compare comp)
  {
    typedef typename detail::IteratorTraits<ValuesPortal>::IteratorType
                                                            IteratorType;
    IteratorType begin = IteratorBegin(values);
    IteratorType newLast = ::thrust::unique(begin, IteratorEnd(values), comp);
    return ::thrust::distance(begin, newLast);
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void UpperBoundsPortal(const InputPortal &input,
                         const ValuesPortal &values,
                         const OutputPortal &output)
  {
    ::thrust::upper_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output));
  }


  template<class InputPortal, class ValuesPortal, class OutputPortal,
           class Compare>
  VTKM_CONT_EXPORT static void UpperBoundsPortal(const InputPortal &input,
                                                const ValuesPortal &values,
                                                const OutputPortal &output,
                                                Compare comp)
  {
    ::thrust::upper_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output),
                          comp);
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void UpperBoundsPortal(const InputPortal &input,
                         const OutputPortal &values_output)
  {
    ::thrust::upper_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values_output),
                          IteratorEnd(values_output),
                          IteratorBegin(values_output));
  }

//-----------------------------------------------------------------------------

public:
  template<typename T, class SIn, class SOut>
  VTKM_CONT_EXPORT static void Copy(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut> &output)
  {
    vtkm::Id numberOfValues = input.GetNumberOfValues();
    CopyPortal(input.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SVal, class SOut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SVal, class SOut, class Compare>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output,
      Compare comp)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                      comp);
  }

  template<class SIn, class SOut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,SIn> &input,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut> &values_output)
  {
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values_output.PrepareForInPlace(DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SOut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut>& output)
  {
    vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return 0;
      }

    return ScanExclusivePortal(input.PrepareForInput(DeviceAdapterTag()),
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }
  template<typename T, class SIn, class SOut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut>& output)
  {
    vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return 0;
      }

    return ScanInclusivePortal(input.PrepareForInput(DeviceAdapterTag()),
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

// Because of some funny code conversions in nvcc, kernels for devices have to
// be public.
#ifndef VTKM_CUDA
private:
#endif
  template<class FunctorType>
  class ScheduleKernel
  {
  public:
    VTKM_CONT_EXPORT ScheduleKernel(const FunctorType &functor)
      : Functor(functor)
    {  }

    VTKM_EXEC_EXPORT void operator()(vtkm::Id index) const {
      this->Functor(index);
    }
  private:
    FunctorType Functor;
  };

public:
  template<class Functor>
  VTKM_CONT_EXPORT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    const vtkm::Id ERROR_ARRAY_SIZE = 1024;
    ::thrust::system::cuda::vector<char> errorArray(ERROR_ARRAY_SIZE);
    errorArray[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(
          ::thrust::raw_pointer_cast(&(*errorArray.begin())),
          errorArray.size());

    functor.SetErrorMessageBuffer(errorMessage);

    ScheduleKernel<Functor> kernel(functor);

    ::thrust::for_each(::thrust::make_counting_iterator<vtkm::Id>(0),
                       ::thrust::make_counting_iterator<vtkm::Id>(numInstances),
                       kernel);

    if (errorArray[0] != '\0')
      {
      char errorString[ERROR_ARRAY_SIZE];
      ::thrust::copy(errorArray.begin(), errorArray.end(), errorString);

      throw vtkm::cont::ErrorExecution(errorString);
      }
  }

  template<class FunctorType>
  VTKM_CONT_EXPORT
  static void Schedule(FunctorType functor, const vtkm::Id3& rangeMax)
  {
    //default behavior for the general algorithm is to defer to the default
    //schedule implementation. if you want to customize schedule for certain
    //grid types, you need to specialize this method
    DeviceAdapterAlgorithmThrust<DeviceAdapterTag>::Schedule(functor,
                                                       rangeMax[0] * rangeMax[1] * rangeMax[2] );
  }

  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage>& values)
  {
    SortPortal(values.PrepareForInPlace(DeviceAdapterTag()));
  }

  template<typename T, class Storage, class Compare>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage>& values,
      Compare comp)
  {
    SortPortal(values.PrepareForInPlace(DeviceAdapterTag()),comp);
  }

  template<typename T, typename U,
           class StorageT, class StorageU>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT>& keys,
      vtkm::cont::ArrayHandle<U,StorageU>& values)
  {
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTag()),
                    values.PrepareForInPlace(DeviceAdapterTag()));
  }

  template<typename T, typename U,
           class StorageT, class StorageU,
           class Compare>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT>& keys,
      vtkm::cont::ArrayHandle<U,StorageU>& values,
      Compare comp)
  {
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTag()),
                    values.PrepareForInPlace(DeviceAdapterTag()),
                    comp);
  }


  template<typename T, class SStencil, class SOut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,SStencil>& stencil,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output)
  {
    vtkm::Id stencilSize = stencil.GetNumberOfValues();

    RemoveIf(::thrust::make_counting_iterator<vtkm::Id>(0),
             ::thrust::make_counting_iterator<vtkm::Id>(stencilSize),
             stencil,
             output);
  }

  template<typename T,
           typename U,
           class SIn,
           class SStencil,
           class SOut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<U,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SStencil>& stencil,
      vtkm::cont::ArrayHandle<U,SOut>& output)
  {
    StreamCompactPortal(input.PrepareForInput(DeviceAdapterTag()), stencil, output);
  }

  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values)
  {
    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTag()));

    values.Shrink(newSize);
  }

  template<typename T, class Storage, class Compare>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values,
      Compare comp)
  {
    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTag()),comp);

    values.Shrink(newSize);
  }

  template<typename T, class SIn, class SVal, class SOut>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SVal, class SOut, class Compare>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output,
      Compare comp)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                      comp);
  }

  template<class SIn, class SOut>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,SIn> &input,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut> &values_output)
  {
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values_output.PrepareForInPlace(DeviceAdapterTag()));
  }
};

}
}
}
} // namespace vtkm::cont::cuda::internal

#endif //vtk_m_cont_cuda_internal_DeviceAdapterThrust_h
