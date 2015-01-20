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
#ifndef vtk_m_cont_internal_DeviceAdapterAlgorithm_h
#define vtk_m_cont_internal_DeviceAdapterAlgorithm_h

#include <vtkm/Types.h>

#include <vtkm/cont/internal/ArrayManagerExecution.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#ifdef _WIN32
#include <sys/timeb.h>
#include <sys/types.h>
#else //!_WIN32
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace vtkm {
namespace cont {

/// \brief Struct containing device adapter algorithms.
///
/// This struct, templated on the device adapter tag, comprises static methods
/// that implement the algorithms provided by the device adapter. The default
/// struct is not implemented. Device adapter implementations must specialize
/// the template.
///
template<class DeviceAdapterTag>
struct DeviceAdapterAlgorithm
#ifdef VTKM_DOXYGEN_ONLY
{
  /// \brief Copy the contents of one ArrayHandle to another
  ///
  /// Copies the contents of \c input to \c output. The array \c to will be
  /// allocated to the appropriate size.
  ///
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static void Copy(const vtkm::cont::ArrayHandle<T,CIn> &input,
                                    vtkm::cont::ArrayHandle<T, COut> &output);

  /// \brief Output is the first index in input for each item in values that wouldn't alter the ordering of input
  ///
  /// LowerBounds is a vectorized search. From each value in \c values it finds
  /// the first place the item can be inserted in the ordered \c input array and
  /// stores the index in \c output.
  ///
  /// \par Requirements:
  /// \arg \c input must already be sorted
  ///
  template<typename T, class CIn, class CVal, class COut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<T,CVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut>& output);

  /// \brief Output is the first index in input for each item in values that wouldn't alter the ordering of input
  ///
  /// LowerBounds is a vectorized search. From each value in \c values it finds
  /// the first place the item can be inserted in the ordered \c input array and
  /// stores the index in \c output. Uses the custom comparison functor to
  /// determine the correct location for each item.
  ///
  /// \par Requirements:
  /// \arg \c input must already be sorted
  ///
  template<typename T, class CIn, class CVal, class COut, class Compare>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<T,CVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut>& output,
      Compare comp);

  /// \brief A special version of LowerBounds that does an in place operation.
  ///
  /// This version of lower bounds performs an in place operation where each
  /// value in the \c values_output array is replaced by the index in \c input
  /// where it occurs. Because this is an in place operation, the type of the
  /// arrays is limited to vtkm::Id.
  ///
  template<class CIn, class COut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,CIn>& input,
      vtkm::cont::ArrayHandle<vtkm::Id,COut>& values_output);

  /// \brief Compute an inclusive prefix sum operation on the input ArrayHandle.
  ///
  /// Computes an inclusive prefix sum operation on the \c input ArrayHandle,
  /// storing the results in the \c output ArrayHandle. InclusiveScan is
  /// similiar to the stl partial sum function, exception that InclusiveScan
  /// doesn't do a serial sumnation. This means that if you have defined a
  /// custom plus operator for T it must be associative, or you will get
  /// inconsistent results. When the input and output ArrayHandles are the same
  /// ArrayHandle the operation will be done inplace.
  ///
  /// \return The total sum.
  ///
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output);

  /// \brief Compute an exclusive prefix sum operation on the input ArrayHandle.
  ///
  /// Computes an exclusive prefix sum operation on the \c input ArrayHandle,
  /// storing the results in the \c output ArrayHandle. ExclusiveScan is
  /// similiar to the stl partial sum function, exception that ExclusiveScan
  /// doesn't do a serial sumnation. This means that if you have defined a
  /// custom plus operator for T it must be associative, or you will get
  /// inconsistent results. When the input and output ArrayHandles are the same
  /// ArrayHandle the operation will be done inplace.
  ///
  /// \return The total sum.
  ///
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut>& output);

  /// \brief Schedule many instances of a function to run on concurrent threads.
  ///
  /// Calls the \c functor on several threads. This is the function used in the
  /// control environment to spawn activity in the execution environment. \c
  /// functor is a function-like object that can be invoked with the calling
  /// specification <tt>functor(vtkm::Id index)</tt>. It also has a method called
  /// from the control environment to establish the error reporting buffer with
  /// the calling specification <tt>functor.SetErrorMessageBuffer(const
  /// vtkm::exec::internal::ErrorMessageBuffer &errorMessage)</tt>. This object
  /// can be stored in the functor's state such that if RaiseError is called on
  /// it in the execution environment, an ErrorExecution will be thrown from
  /// Schedule.
  ///
  /// The argument of the invoked functor uniquely identifies the thread or
  /// instance of the invocation. There should be one invocation for each index
  /// in the range [0, \c numInstances].
  ///
  template<class Functor>
  VTKM_CONT_EXPORT static void Schedule(Functor functor,
                                        vtkm::Id numInstances);

  /// \brief Schedule many instances of a function to run on concurrent threads.
  ///
  /// Calls the \c functor on several threads. This is the function used in the
  /// control environment to spawn activity in the execution environment. \c
  /// functor is a function-like object that can be invoked with the calling
  /// specification <tt>functor(vtkm::Id3 index)</tt> or <tt>functor(vtkm::Id
  /// index)</tt>. It also has a method called from the control environment to
  /// establish the error reporting buffer with the calling specification
  /// <tt>functor.SetErrorMessageBuffer(const
  /// vtkm::exec::internal::ErrorMessageBuffer &errorMessage)</tt>. This object
  /// can be stored in the functor's state such that if RaiseError is called on
  /// it in the execution environment, an ErrorExecution will be thrown from
  /// Schedule.
  ///
  /// The argument of the invoked functor uniquely identifies the thread or
  /// instance of the invocation. It is at the device adapter's discretion
  /// whether to schedule on 1D or 3D indices, so the functor should have an
  /// operator() overload for each index type. If 3D indices are used, there is
  /// one invocation for every i, j, k value between [0, 0, 0] and \c rangeMax.
  /// If 1D indices are used, this Schedule behaves as if <tt>Schedule(functor,
  /// rangeMax[0]*rangeMax[1]*rangeMax[2])</tt> were called.
  ///
  template<class Functor, class IndiceType>
  VTKM_CONT_EXPORT static void Schedule(Functor functor,
                                        vtkm::Id3 rangeMax);

  /// \brief Unstable ascending sort of input array.
  ///
  /// Sorts the contents of \c values so that they in ascending value. Doesn't
  /// guarantee stability
  ///
  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Sort(vtkm::cont::ArrayHandle<T,Storage> &values);

  /// \brief Unstable ascending sort of input array.
  ///
  /// Sorts the contents of \c values so that they in ascending value based
  /// on the custom compare functor.
  ///
  template<typename T, class Storage, class Compare>
  VTKM_CONT_EXPORT static void Sort(vtkm::cont::ArrayHandle<T,Storage> &values,
                                    Compare comp);

  /// \brief Performs stream compaction to remove unwanted elements in the input array. Output becomes the index values of input that are valid.
  ///
  /// Calls the parallel primitive function of stream compaction on the \c
  /// input to remove unwanted elements. The result of the stream compaction is
  /// placed in \c output. The \c input values are used as the stream
  /// compaction stencil while \c input indices are used as the values to place
  /// into \c ouput. The size of \c output will be modified after this call as
  /// we can't know the number of elements that will be removed by the stream
  /// compaction algorithm.
  ///
  template<typename T, class CStencil, class COut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CStencil> &stencil,
      vtkm::cont::ArrayHandle<vtkm::Id,COut> &output);

  /// \brief Performs stream compaction to remove unwanted elements in the input array.
  ///
  /// Calls the parallel primitive function of stream compaction on the \c
  /// input to remove unwanted elements. The result of the stream compaction is
  /// placed in \c output. The values in \c stencil are used as the stream
  /// compaction stencil while \c input values are placed into \c ouput. The
  /// size of \c output will be modified after this call as we can't know the
  /// number of elements that will be removed by the stream compaction
  /// algorithm.
  ///
  template<typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      const vtkm::cont::ArrayHandle<U,CStencil> &stencil,
      vtkm::cont::ArrayHandle<T,COut> &output);

  /// \brief Completes any asynchronous operations running on the device.
  ///
  /// Waits for any asynchronous operations running on the device to complete.
  ///
  VTKM_CONT_EXPORT static void Synchronize();

  /// \brief Reduce an array to only the unique values it contains
  ///
  /// Removes all duplicate values in \c values that are adjacent to each
  /// other. Which means you should sort the input array unless you want
  /// duplicate values that aren't adjacent. Note the values array size might
  /// be modified by this operation.
  ///
  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage>& values);

  /// \brief Reduce an array to only the unique values it contains
  ///
  /// Removes all duplicate values in \c values that are adjacent to each
  /// other. Which means you should sort the input array unless you want
  /// duplicate values that aren't adjacent. Note the values array size might
  /// be modified by this operation.
  ///
  /// Uses the custom binary predicate Comparison to determine if something
  /// is unique. The predicate must return true if the two items are the same.
  ///
  template<typename T, class Storage, class Compare>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage>& values,
      Compare comp);

  /// \brief Output is the last index in input for each item in values that wouldn't alter the ordering of input
  ///
  /// UpperBounds is a vectorized search. From each value in \c values it finds
  /// the last place the item can be inserted in the ordered \c input array and
  /// stores the index in \c output.
  ///
  /// \par Requirements:
  /// \arg \c input must already be sorted
  ///
  template<typename T, class CIn, class CVal, class COut>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<T,CVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut>& output);

  /// \brief Output is the last index in input for each item in values that wouldn't alter the ordering of input
  ///
  /// LowerBounds is a vectorized search. From each value in \c values it finds
  /// the last place the item can be inserted in the ordered \c input array and
  /// stores the index in \c output. Uses the custom comparison functor to
  /// determine the correct location for each item.
  ///
  /// \par Requirements:
  /// \arg \c input must already be sorted
  ///
  template<typename T, class CIn, class CVal, class COut, class Compare>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,CIn>& input,
      const vtkm::cont::ArrayHandle<T,CVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,COut>& output,
      Compare comp);

  /// \brief A special version of UpperBounds that does an in place operation.
  ///
  /// This version of lower bounds performs an in place operation where each
  /// value in the \c values_output array is replaced by the last index in
  /// \c input where it occurs. Because this is an in place operation, the type
  /// of the arrays is limited to vtkm::Id.
  ///
  template<class CIn, class COut>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,CIn>& input,
      vtkm::cont::ArrayHandle<vtkm::Id,COut>& values_output);
};
#else // VTKM_DOXYGEN_ONLY
    ;
#endif //VTKM_DOXYGEN_ONLY

/// \brief Class providing a device-specific timer.
///
/// The class provide the actual implementation used by vtkm::cont::Timer.
/// A default implementation is provided but device adapters should provide
/// one (in conjunction with DeviceAdapterAlgorithm) where appropriate.  The
/// interface for this class is exactly the same as vtkm::cont::Timer.
///
template<class DeviceAdapterTag>
class DeviceAdapterTimerImplementation
{
public:
  /// When a timer is constructed, all threads are synchronized and the
  /// current time is marked so that GetElapsedTime returns the number of
  /// seconds elapsed since the construction.
  VTKM_CONT_EXPORT DeviceAdapterTimerImplementation()
  {
    this->Reset();
  }

  /// Resets the timer. All further calls to GetElapsedTime will report the
  /// number of seconds elapsed since the call to this. This method
  /// synchronizes all asynchronous operations.
  ///
  VTKM_CONT_EXPORT void Reset()
  {
    this->StartTime = this->GetCurrentTime();
  }

  /// Returns the elapsed time in seconds between the construction of this
  /// class or the last call to Reset and the time this function is called. The
  /// time returned is measured in wall time. GetElapsedTime may be called any
  /// number of times to get the progressive time. This method synchronizes all
  /// asynchronous operations.
  ///
  VTKM_CONT_EXPORT vtkm::Float64 GetElapsedTime()
  {
    TimeStamp currentTime = this->GetCurrentTime();

    vtkm::Float64 elapsedTime;
    elapsedTime = vtkm::Float64(currentTime.Seconds - this->StartTime.Seconds);
    elapsedTime +=
      (vtkm::Float64(currentTime.Microseconds - this->StartTime.Microseconds)
       /vtkm::Float64(1000000));

    return elapsedTime;
  }
  struct TimeStamp
  {
    vtkm::Int64 Seconds;
    vtkm::Int64 Microseconds;
  };
  TimeStamp StartTime;

  VTKM_CONT_EXPORT TimeStamp GetCurrentTime()
  {
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>
        ::Synchronize();

    TimeStamp retval;
#ifdef _WIN32
    timeb currentTime;
    ::ftime(&currentTime);
    retval.Seconds = currentTime.time;
    retval.Microseconds = 1000*currentTime.millitm;
#else
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    retval.Seconds = currentTime.tv_sec;
    retval.Microseconds = currentTime.tv_usec;
#endif
    return retval;
  }
};

}
} // namespace vtkm::cont


//-----------------------------------------------------------------------------
// These includes are intentionally placed here after the declaration of the
// DeviceAdapterAlgorithm template prototype, which all the implementations
// need.

#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_SERIAL
#include <vtkm/cont/internal/DeviceAdapterAlgorithmSerial.h>
#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_CUDA
#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmCuda.h>
// #elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_OPENMP
// #include <vtkm/openmp/cont/internal/DeviceAdapterAlgorithmOpenMP.h>
// #elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_TBB
// #include <vtkm/tbb/cont/internal/DeviceAdapterAlgorithmTBB.h>
#endif

#endif //vtk_m_cont_DeviceAdapterAlgorithm_h
