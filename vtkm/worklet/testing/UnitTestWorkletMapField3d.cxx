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
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace mapfield3d
{
static constexpr vtkm::Id3 SCHEDULE_SIZE = { 10, 10, 10 };
static constexpr vtkm::Id ARRAY_SIZE = SCHEDULE_SIZE[0] * SCHEDULE_SIZE[1] * SCHEDULE_SIZE[2];


template <typename PortalType>
struct ExecutionObject
{
  PortalType Portal;
};

template <typename T>
struct ExecutionObjectInterface : public vtkm::cont::ExecutionObjectBase
{
  vtkm::cont::ArrayHandle<T> Data;
  vtkm::Id3 ScheduleRange;

  template <typename Device>
  VTKM_CONT auto PrepareForExecution(Device device, vtkm::cont::Token& token) const
    -> ExecutionObject<decltype(this->Data.PrepareForInput(device, token))>
  {
    return ExecutionObject<decltype(this->Data.PrepareForInput(device, token))>{
      this->Data.PrepareForInput(device, token)
    };
  }

  vtkm::Id3 GetRange3d() const { return this->ScheduleRange; }
};
}


namespace vtkm
{
namespace exec
{
namespace arg
{
// Fetch for ArrayPortalTex3D when being used for Loads
template <typename PType>
struct Fetch<vtkm::exec::arg::FetchTagExecObject,
             vtkm::exec::arg::AspectTagDefault,
             mapfield3d::ExecutionObject<PType>>
{
  using ValueType = typename PType::ValueType;
  using PortalType = mapfield3d::ExecutionObject<PType>;

  template <typename ThreadIndicesType>
  VTKM_EXEC ValueType Load(const ThreadIndicesType& indices, const PortalType& field) const
  {
    return field.Portal.Get(indices.GetInputIndex());
  }

  template <typename ThreadIndicesType>
  VTKM_EXEC void Store(const ThreadIndicesType&, const PortalType&, const ValueType&) const
  {
  }
};
}
}
}

namespace mapfield3d
{

class TestMapFieldWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(ExecObject, FieldOut, FieldInOut);
  using ExecutionSignature = _3(_1, _2, _3, WorkIndex);

  template <typename T>
  VTKM_EXEC T operator()(const T& in, T& out, T& inout, vtkm::Id workIndex) const
  {
    auto expected = TestValue(workIndex, T()) + T(100);
    if (!test_equal(in, expected))
    {
      this->RaiseError("Got wrong input value.");
    }
    out = static_cast<T>(in - T(100));
    if (!test_equal(inout, TestValue(workIndex, T()) + T(100)))
    {
      this->RaiseError("Got wrong in-out value.");
    }

    // We return the new value of inout. Since _3 is both an arg and return,
    // this tests that the return value is set after updating the arg values.
    return static_cast<T>(inout - T(100));
  }

  template <typename T1, typename T2, typename T3>
  VTKM_EXEC T3 operator()(const T1&, const T2&, const T3&, vtkm::Id) const
  {
    this->RaiseError("Cannot call this worklet with different types.");
    return vtkm::TypeTraits<T3>::ZeroInitialization();
  }
};

template <typename T>
inline vtkm::Id3 SchedulingRange(const ExecutionObjectInterface<T>& inputDomain)
{
  return inputDomain.GetRange3d();
}

template <typename T>
inline vtkm::Id3 SchedulingRange(const ExecutionObjectInterface<T>* const inputDomain)
{
  return inputDomain->GetRange3d();
}


template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    vtkm::cont::ArrayHandle<T> inputHandle =
      vtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE, vtkm::CopyFlag::Off);
    vtkm::cont::ArrayHandle<T> outputHandleAsPtr;
    vtkm::cont::ArrayHandle<T> inoutHandleAsPtr;

    ExecutionObjectInterface<T> inputExecObject;
    inputExecObject.Data = inputHandle;
    inputExecObject.ScheduleRange = SCHEDULE_SIZE;

    vtkm::cont::ArrayCopy(inputHandle, inoutHandleAsPtr);

    std::cout << "Create and run dispatchers." << std::endl;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(inputExecObject, &outputHandleAsPtr, &inoutHandleAsPtr);

    std::cout << "Check results." << std::endl;
    CheckPortal(outputHandleAsPtr.ReadPortal());
    CheckPortal(inoutHandleAsPtr.ReadPortal());
  }
};


void TestWorkletMapField3d(vtkm::cont::DeviceAdapterId id)
{

  using HandleTypesToTest3D =
    vtkm::List<vtkm::Id, vtkm::Vec2i_32, vtkm::FloatDefault, vtkm::Vec3f_64>;

  std::cout << "Testing Map Field with 3d types on device adapter: " << id.GetName() << std::endl;

  //need to test with ExecObject that has 3d range
  //need to fetch from ExecObject that has 3d range
  vtkm::testing::Testing::TryTypes(mapfield3d::DoTestWorklet<TestMapFieldWorklet>(),
                                   HandleTypesToTest3D());
}

} // mapfield3d namespace



int UnitTestWorkletMapField3d(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(mapfield3d::TestWorkletMapField3d, argc, argv);
}
