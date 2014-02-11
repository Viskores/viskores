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
#ifndef vtkm_cont_internal_DeviceAdapterTag_h
#define vtkm_cont_internal_DeviceAdapterTag_h

#include <vtkm/internal/Configure.h>
#include <vtkm/internal/ExportMacros.h>

#include <string>

#include <boost/static_assert.hpp>

#define VTKM_DEVICE_ADAPTER_ERROR     -1
#define VTKM_DEVICE_ADAPTER_UNDEFINED  0
#define VTKM_DEVICE_ADAPTER_SERIAL     1

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif // VTKM_DEVICE_ADAPTER

namespace vtkm {
namespace cont {
namespace internal {

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
}

/// Creates a tag named vtkm::cont::DeviceAdapterTagName and associated MPL
/// structures to use this tag. Always use this macro (in the base namespace)
/// when creating a device adapter.
#define VTKM_CREATE_DEVICE_ADAPTER(Name) \
  namespace vtkm { \
  namespace cont { \
  struct DeviceAdapterTag##Name {  }; \
  namespace internal { \
  template<> \
  struct DeviceAdapterTraits<vtkm::cont::DeviceAdapterTag##Name> { \
    static DeviceAdapterId GetId() { \
      return DeviceAdapterId(#Name); \
    } \
  }; \
  template<> \
  struct DeviceAdapterTagCheck<vtkm::cont::DeviceAdapterTag##Name> { \
    static const bool Valid = true; \
  }; \
  } \
  } \
  }

/// Checks that the argument is a proper device adapter tag. This is a handy
/// concept check for functions and classes to make sure that a template
/// argument is actually a device adapter tag. (You can get weird errors
/// elsewhere in the code when a mistake is made.)
#define VTKM_IS_DEVICE_ADAPTER_TAG(tag) \
  BOOST_STATIC_ASSERT_MSG( \
      ::vtkm::cont::internal::DeviceAdapterTagCheck<tag>::Valid, \
      "Provided type is not a valid VTKm device adapter tag.")

//-----------------------------------------------------------------------------
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_SERIAL

#include <vtkm/cont/internal/DeviceAdapterTagSerial.h>
#define VTKM_DEFAULT_DEVICE_ADAPTER_TAG ::vtkm::cont::DeviceAdapterTagSerial

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


#endif //vtkm_cont_internal_DeviceAdapterTag_h
