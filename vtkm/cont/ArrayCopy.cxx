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
#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/Invoker.h>

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
         (in.GetComponentArray(0).IsOnDevice(device))))
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
      UnknownCopyFunctor2{}(out.ExtractArrayFromComponents<InType>(), in);
    }
    else if (out.IsBaseComponentType<vtkm::FloatDefault>())
    {
      // Can copy anything to default float.
      UnknownCopyFunctor2{}(out.ExtractArrayFromComponents<vtkm::FloatDefault>(), in);
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

void DoUnknownArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
                        const vtkm::cont::UnknownArrayHandle& destination)
{
  if (source.GetNumberOfValues() > 0)
  {
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

void ArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
               vtkm::cont::UnknownArrayHandle& destination)
{
  if (!destination.IsValid())
  {
    destination = source.NewInstanceBasic();
  }

  DoUnknownArrayCopy(source, destination);
}

void ArrayCopy(const vtkm::cont::UnknownArrayHandle& source,
               const vtkm::cont::UnknownArrayHandle& destination)
{
  if (!destination.IsValid())
  {
    throw vtkm::cont::ErrorBadValue(
      "Attempty to copy to a constant UnknownArrayHandle with no valid array.");
  }

  DoUnknownArrayCopy(source, destination);
}

}
} // namespace vtkm::cont
