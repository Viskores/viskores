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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h

#include <vtkm/cont/cuda/ErrorControlCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/Math.h>

// Here are the actual implementation of the algorithms.
#include <vtkm/cont/cuda/internal/DeviceAdapterAlgorithmThrust.h>

#include <cuda.h>

namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {

static
__global__
void DetermineIfValidCudaDevice()
{
  //used only to see if we can launch kernels. It is possible to have a
  //CUDA capable device, but still fail to have CUDA support.
}

}
}
}
}

namespace vtkm {
namespace cont {

template<>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>
    : public vtkm::cont::cuda::internal::DeviceAdapterAlgorithmThrust<
          vtkm::cont::DeviceAdapterTagCuda>
{

  VTKM_CONT_EXPORT static void Synchronize()
  {
    VTKM_CUDA_CALL(cudaDeviceSynchronize());
  }

};

/// CUDA contains its own high resolution timer.
///
template<>
class DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda>
{
public:
  VTKM_CONT_EXPORT DeviceAdapterTimerImplementation()
  {
    VTKM_CUDA_CALL(cudaEventCreate(&this->StartEvent));
    VTKM_CUDA_CALL(cudaEventCreate(&this->EndEvent));
    this->Reset();
  }
  VTKM_CONT_EXPORT ~DeviceAdapterTimerImplementation()
  {
    VTKM_CUDA_CALL(cudaEventDestroy(this->StartEvent));
    VTKM_CUDA_CALL(cudaEventDestroy(this->EndEvent));
  }

  VTKM_CONT_EXPORT void Reset()
  {
    VTKM_CUDA_CALL(cudaEventRecord(this->StartEvent, 0));
    VTKM_CUDA_CALL(cudaEventSynchronize(this->StartEvent));
  }

  VTKM_CONT_EXPORT vtkm::Float64 GetElapsedTime()
  {
    VTKM_CUDA_CALL(cudaEventRecord(this->EndEvent, 0));
    VTKM_CUDA_CALL(cudaEventSynchronize(this->EndEvent));
    float elapsedTimeMilliseconds;
    VTKM_CUDA_CALL(cudaEventElapsedTime(&elapsedTimeMilliseconds,
                                        this->StartEvent,
                                        this->EndEvent));
    return static_cast<vtkm::Float64>(0.001f*elapsedTimeMilliseconds);
  }

private:
  // Copying CUDA events is problematic.
  DeviceAdapterTimerImplementation(const DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda> &);
  void operator=(const DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagCuda> &);

  cudaEvent_t StartEvent;
  cudaEvent_t EndEvent;
};

/// \brief Class providing a CUDA runtime support detector.
///
/// The class provide the actual implementation used by
/// vtkm::cont::RuntimeDeviceInformation for the CUDA backend.
///
/// We will verify at runtime that the machine has at least one CUDA
/// capable device, and said device is from the 'fermi' (SM_20) generation
/// or newer.
///
template<>
class DeviceAdapterRuntimeDetector<vtkm::cont::DeviceAdapterTagCuda>
{
public:
  VTKM_CONT_EXPORT DeviceAdapterRuntimeDetector():
    NumberOfDevices(0),
    HighestArchSupported(0)
  {
    static bool deviceQueryInit = false;
    static int numDevices = 0;
    static int archVersion = 0;

    if(!deviceQueryInit)
      {
      deviceQueryInit = true;

      //first query for the number of devices
      cudaGetDeviceCount(&numDevices);

      for (vtkm::Int32 i = 0; i < numDevices; i++)
      {
        cudaDeviceProp prop;
        VTKM_CUDA_CALL(cudaGetDeviceProperties(&prop, i));
        const vtkm::Int32 arch = (prop.major * 10) + prop.minor;
        archVersion = vtkm::Max(arch, archVersion);
      }

      //Make sure we can actually launch a kernel. This could fail for any
      //of the following reasons:
      //
      // 1. cudaErrorInsufficientDriver, caused by out of data drives
      // 2. cudaErrorDevicesUnavailable, caused by another process locking the
      //    device or somebody disabling cuda support on the device
      // 3. cudaErrorNoKernelImageForDevice we built for a compute version
      //    greater than the device we are running on
      // Most likely others that I don't even know about
      vtkm::cont::cuda::internal::DetermineIfValidCudaDevice <<<1,1>>> ();
      if(cudaSuccess != cudaGetLastError())
        {
        numDevices = 0;
        archVersion = 0;
        }
      }

    this->NumberOfDevices = numDevices;
    this->HighestArchSupported = archVersion;
  }

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  /// Only returns true if we have at-least one CUDA capable device of SM_20 or
  /// greater ( fermi ).
  ///
  VTKM_CONT_EXPORT bool Exists() const
  {
    //
    return this->NumberOfDevices > 0 && this->HighestArchSupported >= 20;
  }

private:
  vtkm::Int32 NumberOfDevices;
  vtkm::Int32 HighestArchSupported;
};

/// CUDA contains its own atomic operations
///
template<typename T>
class DeviceAdapterAtomicArrayImplementation<T,vtkm::cont::DeviceAdapterTagCuda>
{
public:
  VTKM_CONT_EXPORT
  DeviceAdapterAtomicArrayImplementation(
             vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> handle):
    Portal( handle.PrepareForInPlace( vtkm::cont::DeviceAdapterTagCuda()) )
  {
  }

  inline __device__
  T Add(vtkm::Id index, const T& value) const
  {
    T *lockedValue = ::thrust::raw_pointer_cast(this->Portal.GetIteratorBegin() + index);
    return vtkmAtomicAdd(lockedValue, value);
  }

  inline __device__
  T CompareAndSwap(vtkm::Id index, const vtkm::Int64 &newValue, const vtkm::Int64 &oldValue) const
  {
    T *lockedValue = ::thrust::raw_pointer_cast(this->Portal.GetIteratorBegin() + index);
    return vtkmCompareAndSwap(lockedValue, newValue, oldValue);
  }

private:
  typedef typename vtkm::cont::ArrayHandle<T,vtkm::cont::StorageTagBasic>
        ::template ExecutionTypes<vtkm::cont::DeviceAdapterTagCuda>::Portal PortalType;
  PortalType Portal;

  inline __device__
  vtkm::Int64 vtkmAtomicAdd(vtkm::Int64 *address, const vtkm::Int64 &value) const
  {
    return atomicAdd((unsigned long long *)address,(unsigned long long) value);
  }

  inline __device__
  vtkm::Int32 vtkmAtomicAdd(vtkm::Int32 *address, const vtkm::Int32 &value) const
  {
    return atomicAdd(address,value);
  }

  inline __device__
  vtkm::Int32 vtkmCompareAndSwap(vtkm::Int32 *address, const vtkm::Int32 &newValue, const vtkm::Int32 &oldValue) const
  {
    return atomicCAS(address,oldValue,newValue);
  }

  inline __device__
  vtkm::Int64 vtkmCompareAndSwap(vtkm::Int64 *address, const vtkm::Int64 &newValue, const vtkm::Int64 &oldValue) const
  {
    return atomicCAS((unsigned long long int*) address,
                     (unsigned long long int ) oldValue,
                     (unsigned long long int ) newValue);
  }  
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
