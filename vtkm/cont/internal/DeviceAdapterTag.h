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
#ifndef vtk_m_cont_internal_DeviceAdapterTag_h
#define vtk_m_cont_internal_DeviceAdapterTag_h

#include <vtkm/StaticAssert.h>
#include <vtkm/internal/Configure.h>
#include <vtkm/internal/ExportMacros.h>

#include <string>

#define VTKM_DEVICE_ADAPTER_ERROR     -2
#define VTKM_DEVICE_ADAPTER_UNDEFINED -1
#define VTKM_DEVICE_ADAPTER_SERIAL     1
#define VTKM_DEVICE_ADAPTER_CUDA       2
#define VTKM_DEVICE_ADAPTER_TBB        3

#ifndef VTKM_DEVICE_ADAPTER
#ifdef VTKM_CUDA
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_CUDA
#elif defined(VTKM_OPENMP) // !VTKM_CUDA
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_OPENMP
#elif defined(VTKM_ENABLE_TBB) // !VTKM_CUDA && !VTKM_OPENMP
// Unfortunately, VTKM_ENABLE_TBB does not guarantee that TBB is (or isn't)
// available, but there is no way to check for sure in a header library.
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_TBB
#else // !VTKM_CUDA && !VTKM_OPENMP && !VTKM_ENABLE_TBB
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif // !VTKM_CUDA && !VTKM_OPENMP
#endif // VTKM_DEVICE_ADAPTER

namespace vtkm {
namespace cont {

typedef std::string DeviceAdapterId;

template<typename DeviceAdapter>
struct DeviceAdapterTraits;

template<typename DeviceAdapter>
struct DeviceAdapterTagCheck
{
  static const bool Valid = false;
};

}
}

/// Creates a tag named vtkm::cont::DeviceAdapterTagName and associated MPL
/// structures to use this tag. Always use this macro (in the base namespace)
/// when creating a device adapter.
#define VTKM_VALID_DEVICE_ADAPTER(Name) \
  namespace vtkm { \
  namespace cont { \
  struct DeviceAdapterTag##Name {  }; \
  template<> \
  struct DeviceAdapterTraits<vtkm::cont::DeviceAdapterTag##Name> { \
    static DeviceAdapterId GetId() { \
      return DeviceAdapterId(#Name); \
    } \
    static const bool Valid = true;\
  }; \
  template<> \
  struct DeviceAdapterTagCheck<vtkm::cont::DeviceAdapterTag##Name> { \
    static const bool Valid = true; \
  }; \
  } \
  }

/// Marks the tag named vtkm::cont::DeviceAdapterTagName and associated
/// structures as valid to use. Always use this macro (in the base namespace)
/// when creating a device adapter.
#define VTKM_INVALID_DEVICE_ADAPTER(Name) \
  namespace vtkm { \
  namespace cont { \
  struct DeviceAdapterTag##Name {  }; \
  template<> \
  struct DeviceAdapterTraits<vtkm::cont::DeviceAdapterTag##Name> { \
    static DeviceAdapterId GetId() { \
      return DeviceAdapterId(#Name); \
    } \
    static const bool Valid = false;\
  }; \
  template<> \
  struct DeviceAdapterTagCheck<vtkm::cont::DeviceAdapterTag##Name> { \
    static const bool Valid = false; \
  }; \
  } \
  }



/// Checks that the argument is a proper device adapter tag. This is a handy
/// concept check for functions and classes to make sure that a template
/// argument is actually a device adapter tag. (You can get weird errors
/// elsewhere in the code when a mistake is made.)
///
#define VTKM_IS_DEVICE_ADAPTER_TAG(tag) \
  VTKM_STATIC_ASSERT_MSG( \
      ::vtkm::cont::DeviceAdapterTagCheck<tag>::Valid, \
      "Provided type is not a valid VTK-m device adapter tag.")

//-----------------------------------------------------------------------------
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_SERIAL

#include <vtkm/cont/internal/DeviceAdapterTagSerial.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagSerial

#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_CUDA

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagCuda

#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_TBB

#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagTBB

#elif VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_ERROR

#include <vtkm/cont/internal/DeviceAdapterError.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagError

#elif (VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_UNDEFINED) || !defined(VTKM_DEVICE_ADAPTER)

#ifndef VTKM_DEFAULT_DEVICE_ADAPTER_TAG
#warning If device adapter is undefined, VTKM_DEFAULT_DEVICE_ADAPTER_TAG must be defined.
#endif

#else

#warning Unrecognized device adapter given.

#endif


#endif //vtk_m_cont_internal_DeviceAdapterTag_h
