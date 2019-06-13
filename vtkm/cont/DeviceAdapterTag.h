//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DeviceAdapterTag_h
#define vtk_m_cont_DeviceAdapterTag_h

#include <vtkm/StaticAssert.h>
#include <vtkm/Types.h>
#include <vtkm/internal/Configure.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/cont/vtkm_cont_export.h>

#include <string>

#ifdef VTKM_DEVICE_ADAPTER
// Rather than use defines to specify the default device adapter
// VTK-m now builds for all device adapters and uses runtime controls
// to determine where execution occurs
#error The VTKM_DEVICE_ADAPTER define is no longer obeyed and needs to be removed
#endif
#ifdef VTKM_DEFAULT_DEVICE_ADAPTER_TAG
// Rather than use device adapter tag that had no shared parent
// VTK-m now uses a runtime device adapter implementation that
// allows for runtime execution selection of what device to execute on
#error The VTKM_DEFAULT_DEVICE_ADAPTER_TAG define is no longer required and needs to be removed
#endif

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

using DeviceAdapterNameType = std::string;

struct DeviceAdapterId
{
  constexpr bool operator==(DeviceAdapterId other) const { return this->Value == other.Value; }
  constexpr bool operator!=(DeviceAdapterId other) const { return this->Value != other.Value; }
  constexpr bool operator<(DeviceAdapterId other) const { return this->Value < other.Value; }

  constexpr bool IsValueValid() const
  {
    return this->Value > 0 && this->Value < VTKM_MAX_DEVICE_ADAPTER_ID;
  }

  constexpr vtkm::Int8 GetValue() const { return this->Value; }

  VTKM_CONT_EXPORT
  DeviceAdapterNameType GetName() const;

protected:
  friend DeviceAdapterId make_DeviceAdapterId(vtkm::Int8 id);

  constexpr explicit DeviceAdapterId(vtkm::Int8 id)
    : Value(id)
  {
  }

private:
  vtkm::Int8 Value;
};

/// Construct a device adapter id from a runtime string
/// The string is case-insensitive. So CUDA will be selected with 'cuda', 'Cuda', or 'CUDA'.
VTKM_CONT_EXPORT
DeviceAdapterId make_DeviceAdapterId(const DeviceAdapterNameType& name);

/// Construct a device adapter id a vtkm::Int8.
/// The mapping of integer value to devices are:
///
/// DeviceAdapterTagSerial == 1
/// DeviceAdapterTagCuda == 2
/// DeviceAdapterTagTBB == 3
/// DeviceAdapterTagOpenMP == 4
///
inline DeviceAdapterId make_DeviceAdapterId(vtkm::Int8 id)
{
  return DeviceAdapterId(id);
}

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

// Represents when using TryExecute that the functor
// can be executed on any device instead of a specific
// one
VTKM_VALID_DEVICE_ADAPTER(Any, VTKM_DEVICE_ADAPTER_ANY)

VTKM_INVALID_DEVICE_ADAPTER(Undefined, VTKM_DEVICE_ADAPTER_UNDEFINED)

/// Checks that the argument is a proper device adapter tag. This is a handy
/// concept check for functions and classes to make sure that a template
/// argument is actually a device adapter tag. (You can get weird errors
/// elsewhere in the code when a mistake is made.)
///
#define VTKM_IS_DEVICE_ADAPTER_TAG(tag)                                                            \
  static_assert(std::is_base_of<vtkm::cont::DeviceAdapterId, tag>::value &&                        \
                  !std::is_same<vtkm::cont::DeviceAdapterId, tag>::value,                          \
                "Provided type is not a valid VTK-m device adapter tag.")

#endif //vtk_m_cont_DeviceAdapterTag_h
