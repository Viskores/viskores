//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_internal_DeviceAdapterTag_h
#define vtk_m_cont_internal_DeviceAdapterTag_h

#include <vtkm/StaticAssert.h>
#include <vtkm/Types.h>
#include <vtkm/internal/Configure.h>
#include <vtkm/internal/ExportMacros.h>

#include <string>

#define VTKM_DEVICE_ADAPTER_ERROR -2
#define VTKM_DEVICE_ADAPTER_UNDEFINED -1
#define VTKM_DEVICE_ADAPTER_SERIAL 1
#define VTKM_DEVICE_ADAPTER_CUDA 2
#define VTKM_DEVICE_ADAPTER_TBB 3

namespace vtkm
{
namespace cont
{

using DeviceAdapterId = vtkm::Int8;
using DeviceAdapterNameType = std::string;

template <typename DeviceAdapter>
struct DeviceAdapterTraits;

template <typename DeviceAdapter>
struct DeviceAdapterTagCheck
{
  static const bool Valid = false;
};
}
}

/// Creates a tag named vtkm::cont::DeviceAdapterTagName and associated MPL
/// structures to use this tag. Always use this macro (in the base namespace)
/// when creating a device adapter.
#define VTKM_VALID_DEVICE_ADAPTER(Name, Id)                                                        \
  namespace vtkm                                                                                   \
  {                                                                                                \
  namespace cont                                                                                   \
  {                                                                                                \
  struct VTKM_ALWAYS_EXPORT DeviceAdapterTag##Name                                                 \
  {                                                                                                \
  };                                                                                               \
  template <>                                                                                      \
  struct DeviceAdapterTraits<vtkm::cont::DeviceAdapterTag##Name>                                   \
  {                                                                                                \
    static DeviceAdapterId GetId() { return DeviceAdapterId(Id); }                                 \
    static DeviceAdapterNameType GetName() { return DeviceAdapterNameType(#Name); }                \
    static const bool Valid = true;                                                                \
  };                                                                                               \
  template <>                                                                                      \
  struct DeviceAdapterTagCheck<vtkm::cont::DeviceAdapterTag##Name>                                 \
  {                                                                                                \
    static const bool Valid = true;                                                                \
  };                                                                                               \
  }                                                                                                \
  }

/// Marks the tag named vtkm::cont::DeviceAdapterTagName and associated
/// structures as invalid to use. Always use this macro (in the base namespace)
/// when creating a device adapter.
#define VTKM_INVALID_DEVICE_ADAPTER(Name, Id)                                                      \
  namespace vtkm                                                                                   \
  {                                                                                                \
  namespace cont                                                                                   \
  {                                                                                                \
  struct DeviceAdapterTag##Name                                                                    \
  {                                                                                                \
  };                                                                                               \
  template <>                                                                                      \
  struct DeviceAdapterTraits<vtkm::cont::DeviceAdapterTag##Name>                                   \
  {                                                                                                \
    static DeviceAdapterId GetId() { return DeviceAdapterId(Id); }                                 \
    static DeviceAdapterNameType GetName() { return DeviceAdapterNameType(#Name); }                \
    static const bool Valid = false;                                                               \
  };                                                                                               \
  template <>                                                                                      \
  struct DeviceAdapterTagCheck<vtkm::cont::DeviceAdapterTag##Name>                                 \
  {                                                                                                \
    static const bool Valid = false;                                                               \
  };                                                                                               \
  }                                                                                                \
  }

/// Checks that the argument is a proper device adapter tag. This is a handy
/// concept check for functions and classes to make sure that a template
/// argument is actually a device adapter tag. (You can get weird errors
/// elsewhere in the code when a mistake is made.)
///
#define VTKM_IS_DEVICE_ADAPTER_TAG(tag)                                                            \
  VTKM_STATIC_ASSERT_MSG(::vtkm::cont::DeviceAdapterTagCheck<tag>::Valid,                          \
                         "Provided type is not a valid VTK-m device adapter tag.")

#endif //vtk_m_cont_internal_DeviceAdapterTag_h
