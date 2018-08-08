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
#define VTKM_DEVICE_ADAPTER_OPENMP 4
//VTKM_DEVICE_ADAPTER_TestAlgorithmGeneral 7
#define VTKM_MAX_DEVICE_ADAPTER_ID 8
#define VTKM_DEVICE_ADAPTER_ANY 127

namespace vtkm
{
namespace cont
{

struct DeviceAdapterId
{
  constexpr explicit DeviceAdapterId(vtkm::Int8 id)
    : Value(id)
  {
  }

  constexpr bool operator==(DeviceAdapterId other) const { return this->Value == other.Value; }
  constexpr bool operator!=(DeviceAdapterId other) const { return this->Value != other.Value; }
  constexpr bool operator<(DeviceAdapterId other) const { return this->Value < other.Value; }

  constexpr bool IsValueValid() const
  {
    return this->Value > 0 && this->Value < VTKM_MAX_DEVICE_ADAPTER_ID;
  }

  constexpr vtkm::Int8 GetValue() const { return this->Value; }

private:
  vtkm::Int8 Value;
};

// Represents when using TryExecute that the functor
// can be executed on any device instead of a specific
// one
struct DeviceAdapterIdAny : DeviceAdapterId
{
  constexpr DeviceAdapterIdAny()
    : DeviceAdapterId(127)
  {
  }
};

struct DeviceAdapterIdUndefined : DeviceAdapterId
{
  constexpr DeviceAdapterIdUndefined()
    : DeviceAdapterId(VTKM_DEVICE_ADAPTER_UNDEFINED)
  {
  }
};

using DeviceAdapterNameType = std::string;

template <typename DeviceAdapter>
struct DeviceAdapterTraits;
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
  struct VTKM_ALWAYS_EXPORT DeviceAdapterTag##Name : DeviceAdapterId                               \
  {                                                                                                \
    constexpr DeviceAdapterTag##Name()                                                             \
      : DeviceAdapterId(Id)                                                                        \
    {                                                                                              \
    }                                                                                              \
    static constexpr bool IsEnabled = true;                                                        \
  };                                                                                               \
  template <>                                                                                      \
  struct DeviceAdapterTraits<vtkm::cont::DeviceAdapterTag##Name>                                   \
  {                                                                                                \
    static DeviceAdapterNameType GetName() { return DeviceAdapterNameType(#Name); }                \
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
  struct VTKM_ALWAYS_EXPORT DeviceAdapterTag##Name : DeviceAdapterId                               \
  {                                                                                                \
    constexpr DeviceAdapterTag##Name()                                                             \
      : DeviceAdapterId(Id)                                                                        \
    {                                                                                              \
    }                                                                                              \
    static constexpr bool IsEnabled = false;                                                       \
  };                                                                                               \
  template <>                                                                                      \
  struct DeviceAdapterTraits<vtkm::cont::DeviceAdapterTag##Name>                                   \
  {                                                                                                \
    static DeviceAdapterNameType GetName() { return DeviceAdapterNameType(#Name); }                \
  };                                                                                               \
  }                                                                                                \
  }

/// Checks that the argument is a proper device adapter tag. This is a handy
/// concept check for functions and classes to make sure that a template
/// argument is actually a device adapter tag. (You can get weird errors
/// elsewhere in the code when a mistake is made.)
///
#define VTKM_IS_DEVICE_ADAPTER_TAG(tag)                                                            \
  static_assert(std::is_base_of<vtkm::cont::DeviceAdapterId, tag>::value &&                        \
                  !std::is_same<vtkm::cont::DeviceAdapterId, tag>::value,                          \
                "Provided type is not a valid VTK-m device adapter tag.")

#endif //vtk_m_cont_internal_DeviceAdapterTag_h
