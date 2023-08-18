//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/cont/internal/ArrayCopyUnknown.h>

#include <vtkm/worklet/WorkletMapField.h>

namespace
{

// Use a worklet because device adapter copies often have an issue with casting the values from the
// `ArrayHandleRecomineVec` that comes from `UnknownArrayHandle::CastAndCallWithExtractedArray`.
struct CopyWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename InType, typename OutType>
  VTKM_EXEC void operator()(const InType& in, OutType& out) const
  {
    out = in;
  }
};

struct UnknownCopyOnDevice
{
  bool Called = false;

  template <typename InType, typename OutType>
  void operator()(vtkm::cont::DeviceAdapterId device,
                  const vtkm::cont::ArrayHandleRecombineVec<InType>& in,
                  const vtkm::cont::ArrayHandleRecombineVec<OutType>& out)
  {
    // Note: ArrayHandleRecombineVec returns the wrong value for IsOnDevice (always true).
    // This is one of the consequences of ArrayHandleRecombineVec breaking assumptions of
    // ArrayHandle. It does this by stuffing Buffer objects in another Buffer's meta data
    // rather than listing them explicitly (where they can be queried). We get around this
    // by pulling out one of the component arrays and querying that.
    if (!this->Called &&
        ((device == vtkm::cont::DeviceAdapterTagAny{}) ||
         (in.GetComponentArray(0).IsOnDevice(device) &&
          vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(device))))
    {
      vtkm::cont::Invoker invoke(device);
      invoke(CopyWorklet{}, in, out);
      this->Called = true;
    }
  }
};

struct UnknownCopyFunctor2
{
  template <typename OutType, typename InType>
  void operator()(const vtkm::cont::ArrayHandleRecombineVec<OutType>& out,
                  const vtkm::cont::ArrayHandleRecombineVec<InType>& in) const
  {
    UnknownCopyOnDevice doCopy;

    // Try to copy on a device that the data are already on.
    vtkm::ListForEach(doCopy, VTKM_DEFAULT_DEVICE_ADAPTER_LIST{}, in, out);

    // If it was not on any device, call one more time with any adapter to copy wherever.
    doCopy(vtkm::cont::DeviceAdapterTagAny{}, in, out);
  }
};

struct UnknownCopyFunctor1
{
  template <typename InType>
  void operator()(const vtkm::cont::ArrayHandleRecombineVec<InType>& in,
                  const vtkm::cont::UnknownArrayHandle& out) const
  {
    out.Allocate(in.GetNumberOfValues());

    this->DoIt(in, out, typename std::is_same<vtkm::FloatDefault, InType>::type{});
  }

  template <typename InType>
  void DoIt(const vtkm::cont::ArrayHandleRecombineVec<InType>& in,
            const vtkm::cont::UnknownArrayHandle& out,
            std::false_type) const
  {
    // Source is not float.
    if (out.IsBaseComponentType<InType>())
    {
      // Arrays have the same base component type. Copy directly.
      try
      {
        UnknownCopyFunctor2{}(out.ExtractArrayFromComponents<InType>(vtkm::CopyFlag::Off), in);
      }
      catch (vtkm::cont::Error& error)
      {
        throw vtkm::cont::ErrorBadType(
          "Unable to copy to an array of type " + out.GetArrayTypeName() +
          " using anonymous methods. Try using vtkm::cont::ArrayCopyDevice. "
          "(Original error: `" +
          error.GetMessage() + "')");
      }
    }
    else if (out.IsBaseComponentType<vtkm::FloatDefault>())
    {
      // Can copy anything to default float.
      try
      {
        UnknownCopyFunctor2{}(
          out.ExtractArrayFromComponents<vtkm::FloatDefault>(vtkm::CopyFlag::Off), in);
      }
      catch (vtkm::cont::Error& error)
      {
        throw vtkm::cont::ErrorBadType(
          "Unable to copy to an array of type " + out.GetArrayTypeName() +
          " using anonymous methods. Try using vtkm::cont::ArrayCopyDevice. "
          "(Original error: `" +
          error.GetMessage() + "')");
      }
    }
    else
    {
      // Arrays have different base types. To reduce the number of template paths from nxn to 3n,
      // copy first to a temp array of default float.
      vtkm::cont::UnknownArrayHandle temp = out.NewInstanceFloatBasic();
      (*this)(in, temp);
      vtkm::cont::ArrayCopy(temp, out);
    }
  }

  template <typename InType>
  void DoIt(const vtkm::cont::ArrayHandleRecombineVec<InType>& in,
            const vtkm::cont::UnknownArrayHandle& out,
            std::true_type) const
  {
    // Source array is FloatDefault. That should be copiable to anything.
    out.CastAndCallWithExtractedArray(UnknownCopyFunctor2{}, in);
  }
};

void ArrayCopySpecialCase(const vtkm::cont::ArrayHandleIndex& source,
                          const vtkm::cont::UnknownArrayHandle& destination)
{
  if (destination.CanConvert<vtkm::cont::ArrayHandleIndex>())
  {
    // Unlikely, but we'll check.
    destination.AsArrayHandle<vtkm::cont::ArrayHandleIndex>().DeepCopyFrom(source);
  }
  else if (destination.IsBaseComponentType<vtkm::Id>())
  {
    destination.Allocate(source.GetNumberOfValues());
    auto dest = destination.ExtractComponent<vtkm::Id>(0, vtkm::CopyFlag::Off);
    vtkm::cont::ArrayCopyDevice(source, dest);
  }
  else if (destination.IsBaseComponentType<vtkm::IdComponent>())
  {
    destination.Allocate(source.GetNumberOfValues());
    auto dest = destination.ExtractComponent<vtkm::IdComponent>(0, vtkm::CopyFlag::Off);
    vtkm::cont::ArrayCopyDevice(source, dest);
  }
  else if (destination.CanConvert<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>())
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> dest;
    destination.AsArrayHandle(dest);
    vtkm::cont::ArrayCopyDevice(source, dest);
  }
  else
  {
    // Initializing something that is probably not really an index. Rather than trace down every
    // unlikely possibility, just copy to float and then to the final array.
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> dest;
    vtkm::cont::ArrayCopyDevice(source, dest);
    vtkm::cont::ArrayCopy(dest, destination);
  }
}

template <typename ArrayHandleType>
bool TryArrayCopySpecialCase(const vtkm::cont::UnknownArrayHandle& source,
                             const vtkm::cont::UnknownArrayHandle& destination)
{
  if (source.CanConvert<ArrayHandleType>())
  {
    ArrayCopySpecialCase(source.AsArrayHandle<ArrayHandleType>(), destination);
    return true;
  }
  else
  {
    return false;
  }
}

void DoUnknownArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
                        const vtkm::cont::UnknownArrayHandle& destination)
{
  if (source.GetNumberOfValues() > 0)
  {
    // Try known special cases.
    if (TryArrayCopySpecialCase<vtkm::cont::ArrayHandleIndex>(source, destination))
    {
      return;
    }

    source.CastAndCallWithExtractedArray(UnknownCopyFunctor1{}, destination);
  }
  else
  {
    destination.ReleaseResources();
  }
}

} // anonymous namespace

namespace vtkm
{
namespace cont
{
namespace internal
{

void ArrayCopyUnknown(const vtkm::cont::UnknownArrayHandle& source,
                      vtkm::cont::UnknownArrayHandle& destination)
{
  if (!destination.IsValid())
  {
    destination = source.NewInstanceBasic();
  }

  DoUnknownArrayCopy(source, destination);
}

void ArrayCopyUnknown(const vtkm::cont::UnknownArrayHandle& source,
                      const vtkm::cont::UnknownArrayHandle& destination)
{
  if (!destination.IsValid())
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempty to copy to a constant UnknownArrayHandle with no valid array.");
  }

  DoUnknownArrayCopy(source, destination);
}

} // namespace vtkm::cont::internal
} // namespace vtkm::cont
} // namespace vtkm
