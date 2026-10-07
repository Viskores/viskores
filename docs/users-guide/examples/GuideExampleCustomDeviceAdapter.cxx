//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// For pretend configuration.
#define VISKORES_ENABLE_CXX11THREAD

////
//// BEGIN-EXAMPLE DeviceAdapterTagCxx11Thread
////
#include <viskores/cont/DeviceAdapterTag.h>

// If this device adapter were to be contributed to Viskores, then this macro
// declaration should be moved to DeviceAdapterTag.h and given a unique
// number. It also has to be less than VISKORES_MAX_DEVICE_ADAPTER_ID.
//// PAUSE-EXAMPLE
// Normally you would pick a unique number for the numeric id. However,
// because we are not actually adding this to the Viskores source, we cannot
// actually get the RuntimeDeviceInformation class to find our memory
// manager. Thus, instead we just hijack the id for the serial device,
// which should have a compatible memory manager. Still, we want the
// documentation to suggest that this is unique, so fake it.
#define VISKORES_DEVICE_ADAPTER_CXX11_THREAD 1
#if 0
//// RESUME-EXAMPLE
#define VISKORES_DEVICE_ADAPTER_CXX11_THREAD 6
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE

#ifdef VISKORES_ENABLE_CXX11THREAD
VISKORES_VALID_DEVICE_ADAPTER(Cxx11Thread, VISKORES_DEVICE_ADAPTER_CXX11_THREAD);
#else
VISKORES_INVALID_DEVICE_ADAPTER(Cxx11Thread, VISKORES_DEVICE_ADAPTER_CXX11_THREAD);
#endif
////
//// END-EXAMPLE DeviceAdapterTagCxx11Thread
////

#include <viskores/cont/DeviceAdapterAlgorithm.h>

////
//// BEGIN-EXAMPLE DeviceAdapterRuntimeDetectorCxx11Thread
////
namespace viskores
{
namespace cont
{

template<>
class DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagCxx11Thread>
{
public:
  VISKORES_CONT bool Exists() const
  {
    //// LABEL ExistsFlag
    return viskores::cont::DeviceAdapterTagCxx11Thread::IsEnabled;
  }
};

} // namespace cont
} // namespace viskores
////
//// END-EXAMPLE DeviceAdapterRuntimeDetectorCxx11Thread
////

#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>

////
//// BEGIN-EXAMPLE DeviceAdapterMemoryManagerCxx11Thread
////
//// PAUSE-EXAMPLE
// We did not really put the device adapter components in separate header
// files, but for the purposes of an example we are pretending we are.
#if 0
//// RESUME-EXAMPLE
#include <viskores/cont/cxx11/internal/DeviceAdapterTagCxx11Thread.h>
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE

#include <viskores/cont/internal/DeviceAdapterMemoryManager.h>
#include <viskores/cont/internal/DeviceAdapterMemoryManagerShared.h>

namespace viskores
{
namespace cont
{
namespace internal
{

template<>
class DeviceAdapterMemoryManager<viskores::cont::DeviceAdapterTagCxx11Thread>
  : public viskores::cont::internal::DeviceAdapterMemoryManagerShared
{
public:
  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const override
  {
    return viskores::cont::DeviceAdapterTagCxx11Thread{};
  }
};

}
}
} // namespace viskores::cont::internal
////
//// END-EXAMPLE DeviceAdapterMemoryManagerCxx11Thread
////

#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>

////
//// BEGIN-EXAMPLE RuntimeDeviceConfigurationCxx11Thread
////
//// PAUSE-EXAMPLE
// We did not really put the device adapter components in separate header
// files, but for the purposes of an example we are pretending we are.
#if 0
//// RESUME-EXAMPLE
#include <viskores/cont/cxx11/internal/DeviceAdapterTagCxx11Thread.h>
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE

#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>

#include <thread>

namespace viskores
{
namespace cont
{
namespace internal
{

template<>
class RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagCxx11Thread>
  : public viskores::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  VISKORES_CONT RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagCxx11Thread>()
    : NumThreads(std::thread::hardware_concurrency())
  {
  }

  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const override
  {
    return viskores::cont::DeviceAdapterTagCxx11Thread{};
  }

  VISKORES_CONT viskores::cont::internal::RuntimeDeviceConfigReturnCode GetThreads(
    viskores::Id& value) const override
  {
    value = this->NumThreads;
    return viskores::cont::internal::RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VISKORES_CONT viskores::cont::internal::RuntimeDeviceConfigReturnCode SetThreads(
    const viskores::Id& value) override
  {
    if ((value <= 0) ||
        (value > static_cast<viskores::Id>(std::thread::hardware_concurrency())))
    {
      this->NumThreads = std::thread::hardware_concurrency();
    }
    else
    {
      this->NumThreads = value;
    }
    return viskores::cont::internal::RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VISKORES_CONT viskores::cont::internal::RuntimeDeviceConfigReturnCode GetMaxThreads(
    viskores::Id& value) const override
  {
    value = std::thread::hardware_concurrency();
    return viskores::cont::internal::RuntimeDeviceConfigReturnCode::SUCCESS;
  }

private:
  viskores::Id NumThreads;
};

}
}
} // namespace viskores::cont::internal
////
//// END-EXAMPLE RuntimeDeviceConfigurationCxx11Thread
////

////
//// BEGIN-EXAMPLE DeviceAdapterAlgorithmCxx11Thread
////
//// PAUSE-EXAMPLE
// We did not really put the device adapter components in separate header
// files, but for the purposes of an example we are pretending we are.
#if 0
//// RESUME-EXAMPLE
#include <viskores/cont/cxx11/internal/DeviceAdapterTagCxx11Thread.h>
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE

#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/ErrorExecution.h>
#include <viskores/cont/internal/DeviceAdapterAlgorithmGeneral.h>

#include <thread>

namespace viskores
{
namespace cont
{

template<>
struct DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCxx11Thread>
  : viskores::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCxx11Thread>,
      viskores::cont::DeviceAdapterTagCxx11Thread>
{
private:
  template<typename FunctorType>
  struct ScheduleKernel1D
  {
    VISKORES_CONT ScheduleKernel1D(const FunctorType& functor)
      : Functor(functor)
    {
    }

    void operator()() const
    {
      try
      {
        for (viskores::Id threadId = this->BeginId; threadId < this->EndId; threadId++)
        {
          this->Functor(threadId);
          // If an error is raised, abort execution.
          if (this->ErrorMessage.IsErrorRaised())
          {
            return;
          }
        }
      }
      catch (const viskores::cont::Error& error)
      {
        this->ErrorMessage.RaiseError(error.GetMessage().c_str());
      }
      catch (const std::exception& error)
      {
        this->ErrorMessage.RaiseError(error.what());
      }
      catch (...)
      {
        this->ErrorMessage.RaiseError("Unknown exception raised.");
      }
    }

    FunctorType Functor;
    viskores::exec::internal::ErrorMessageBuffer ErrorMessage;
    viskores::Id BeginId;
    viskores::Id EndId;
  };

  template<typename FunctorType>
  struct ScheduleKernel3D
  {
    VISKORES_CONT ScheduleKernel3D(const FunctorType& functor, viskores::Id3 maxRange)
      : Functor(functor)
      , MaxRange(maxRange)
    {
    }

    void operator()() const
    {
      viskores::Id3 threadId3D(this->BeginId % this->MaxRange[0],
                               (this->BeginId / this->MaxRange[0]) % this->MaxRange[1],
                               this->BeginId / (this->MaxRange[0] * this->MaxRange[1]));

      try
      {
        for (viskores::Id threadId = this->BeginId; threadId < this->EndId; threadId++)
        {
          this->Functor(threadId3D);
          // If an error is raised, abort execution.
          if (this->ErrorMessage.IsErrorRaised())
          {
            return;
          }

          threadId3D[0]++;
          if (threadId3D[0] >= MaxRange[0])
          {
            threadId3D[0] = 0;
            threadId3D[1]++;
            if (threadId3D[1] >= MaxRange[1])
            {
              threadId3D[1] = 0;
              threadId3D[2]++;
            }
          }
        }
      }
      catch (const viskores::cont::Error& error)
      {
        this->ErrorMessage.RaiseError(error.GetMessage().c_str());
      }
      catch (const std::exception& error)
      {
        this->ErrorMessage.RaiseError(error.what());
      }
      catch (...)
      {
        this->ErrorMessage.RaiseError("Unknown exception raised.");
      }
    }

    FunctorType Functor;
    viskores::exec::internal::ErrorMessageBuffer ErrorMessage;
    viskores::Id BeginId;
    viskores::Id EndId;
    viskores::Id3 MaxRange;
  };

  template<typename KernelType>
  VISKORES_CONT static void DoSchedule(KernelType kernel, viskores::Id numInstances)
  {
    if (numInstances < 1)
    {
      return;
    }

    const viskores::Id MESSAGE_SIZE = 1024;
    char errorString[MESSAGE_SIZE];
    errorString[0] = '\0';
    viskores::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
    kernel.Functor.SetErrorMessageBuffer(errorMessage);
    kernel.ErrorMessage = errorMessage;

    viskores::Id numThreads;

    auto config = internal::RuntimeDeviceConfiguration<
      viskores::cont::DeviceAdapterTagCxx11Thread>();
    config.SetThreads(numInstances);
    config.GetThreads(numThreads);
    viskores::Id numInstancesPerThread = (numInstances + numThreads - 1) / numThreads;

    std::thread* threadPool = new std::thread[numThreads];
    viskores::Id beginId = 0;
    for (viskores::Id threadIndex = 0; threadIndex < numThreads; threadIndex++)
    {
      viskores::Id endId = std::min(beginId + numInstancesPerThread, numInstances);
      KernelType threadKernel = kernel;
      threadKernel.BeginId = beginId;
      threadKernel.EndId = endId;
      std::thread newThread(threadKernel);
      threadPool[threadIndex].swap(newThread);
      beginId = endId;
    }

    for (viskores::Id threadIndex = 0; threadIndex < numThreads; threadIndex++)
    {
      threadPool[threadIndex].join();
    }

    delete[] threadPool;

    if (errorMessage.IsErrorRaised())
    {
      throw viskores::cont::ErrorExecution(errorString);
    }
  }

public:
  template<typename FunctorType>
  VISKORES_CONT static void Schedule(FunctorType functor, viskores::Id numInstances)
  {
    DoSchedule(ScheduleKernel1D<FunctorType>(functor), numInstances);
  }

  template<typename FunctorType>
  VISKORES_CONT static void Schedule(FunctorType functor, viskores::Id3 maxRange)
  {
    viskores::Id numInstances = maxRange[0] * maxRange[1] * maxRange[2];
    DoSchedule(ScheduleKernel3D<FunctorType>(functor, maxRange), numInstances);
  }

  VISKORES_CONT
  static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control threaad is
    // calling this method, then nothing should be running in the execution
    // environment.
  }
};

} // namespace cont
} // namespace viskores
////
//// END-EXAMPLE DeviceAdapterAlgorithmCxx11Thread
////

////
//// BEGIN-EXAMPLE DeviceAdapterTimerImplementationCxx11Thread
////
#include <chrono>

namespace viskores
{
namespace cont
{

template<>
class DeviceAdapterTimerImplementation<viskores::cont::DeviceAdapterTagCxx11Thread>
{
public:
  VISKORES_CONT
  DeviceAdapterTimerImplementation() { this->Reset(); }

  VISKORES_CONT
  void Reset()
  {
    viskores::cont::DeviceAdapterAlgorithm<
      viskores::cont::DeviceAdapterTagCxx11Thread>::Synchronize();
    this->StartTime = std::chrono::high_resolution_clock::now();
  }

  VISKORES_CONT
  viskores::Float64 GetElapsedTime()
  {
    viskores::cont::DeviceAdapterAlgorithm<
      viskores::cont::DeviceAdapterTagCxx11Thread>::Synchronize();
    std::chrono::high_resolution_clock::time_point endTime =
      std::chrono::high_resolution_clock::now();

    std::chrono::high_resolution_clock::duration elapsedTicks =
      endTime - this->StartTime;

    std::chrono::duration<viskores::Float64> elapsedSeconds(elapsedTicks);

    return elapsedSeconds.count();
  }

private:
  std::chrono::high_resolution_clock::time_point StartTime;
};

} // namespace cont
} // namespace viskores
////
//// END-EXAMPLE DeviceAdapterTimerImplementationCxx11Thread
////

////
//// BEGIN-EXAMPLE UnitTestDeviceAdapterCxx11Thread
////
//// PAUSE-EXAMPLE
// We did not really put the device adapter components in separate header
// files, but for the purposes of an example we are pretending we are.
#if 0
//// RESUME-EXAMPLE
#include <viskores/cont/cxx11/DeviceAdapterCxx11Thread.h>
//// PAUSE-EXAMPLE
#endif
//// RESUME-EXAMPLE

#include <viskores/Types.h>
#include <viskores/cont/testing/TestingDeviceAdapter.h>
#include <viskores/cont/viskores_cont_export.h>

int UnitTestDeviceAdapterCxx11Thread(int argc, char* argv[])
{
  return viskores::cont::testing::TestingDeviceAdapter<
    viskores::cont::DeviceAdapterTagCxx11Thread>::Run(argc, argv);
}
////
//// END-EXAMPLE UnitTestDeviceAdapterCxx11Thread
////

int GuideExampleCustomDeviceAdapter(int argc, char* argv[])
{
  return UnitTestDeviceAdapterCxx11Thread(argc, argv);
}
