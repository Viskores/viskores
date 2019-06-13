//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/arg/FetchTagArrayTopologyMapIn.h>

#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

struct TestIndexPortal
{
  using ValueType = vtkm::Id;

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const { return index; }
};

struct TestZeroPortal
{
  using ValueType = vtkm::IdComponent;

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id) const { return 0; }
};

template <vtkm::IdComponent InputDomainIndex, vtkm::IdComponent ParamIndex, typename T>
struct FetchArrayTopologyMapInTests
{

  template <typename Invocation>
  void TryInvocation(const Invocation& invocation) const
  {
    using ConnectivityType = typename Invocation::InputDomainType;
    using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

    using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
                                             vtkm::exec::arg::AspectTagDefault,
                                             ThreadIndicesType,
                                             TestPortal<T>>;

    FetchType fetch;

    const vtkm::Id threadIndex = 0;
    const vtkm::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const vtkm::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const vtkm::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    ThreadIndicesType indices(
      threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain());

    typename FetchType::ValueType value =
      fetch.Load(indices, invocation.Parameters.template GetParameter<ParamIndex>());
    VTKM_TEST_ASSERT(value.GetNumberOfComponents() == 8,
                     "Topology fetch got wrong number of components.");

    VTKM_TEST_ASSERT(test_equal(value[0], TestValue(0, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[1], TestValue(1, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[2], TestValue(3, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[3], TestValue(2, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[4], TestValue(4, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[5], TestValue(5, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[6], TestValue(7, T())), "Got invalid value from Load.");
    VTKM_TEST_ASSERT(test_equal(value[7], TestValue(6, T())), "Got invalid value from Load.");
  }

  void operator()() const
  {
    std::cout << "Trying ArrayTopologyMapIn fetch on parameter " << ParamIndex << " with type "
              << vtkm::testing::TypeName<T>::Name() << std::endl;

    using BaseFunctionInterface = vtkm::internal::FunctionInterface<void(vtkm::internal::NullType,
                                                                         vtkm::internal::NullType,
                                                                         vtkm::internal::NullType,
                                                                         vtkm::internal::NullType,
                                                                         vtkm::internal::NullType)>;

    vtkm::internal::ConnectivityStructuredInternals<3> connectivityInternals;
    connectivityInternals.SetPointDimensions(vtkm::Id3(2, 2, 2));
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                       vtkm::TopologyElementTagCell,
                                       3>
      connectivity(connectivityInternals);

    this->TryInvocation(vtkm::internal::make_Invocation<InputDomainIndex>(
      BaseFunctionInterface()
        .Replace<InputDomainIndex>(connectivity)
        .template Replace<ParamIndex>(TestPortal<T>()),
      BaseFunctionInterface(),
      BaseFunctionInterface(),
      TestIndexPortal(),
      TestZeroPortal(),
      TestIndexPortal()));
  }
};

struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayTopologyMapInTests<3, 1, T>()();
    FetchArrayTopologyMapInTests<1, 2, T>()();
    FetchArrayTopologyMapInTests<2, 3, T>()();
    FetchArrayTopologyMapInTests<1, 4, T>()();
    FetchArrayTopologyMapInTests<1, 5, T>()();
  }
};

template <vtkm::IdComponent NumDimensions, vtkm::IdComponent ParamIndex, typename Invocation>
void TryStructuredPointCoordinatesInvocation(const Invocation& invocation)
{
  using ConnectivityType = typename Invocation::InputDomainType;
  using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType>;

  vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
                         vtkm::exec::arg::AspectTagDefault,
                         ThreadIndicesType,
                         vtkm::internal::ArrayPortalUniformPointCoordinates>
    fetch;

  vtkm::Vec<vtkm::FloatDefault, 3> origin = TestValue(0, vtkm::Vec<vtkm::FloatDefault, 3>());
  vtkm::Vec<vtkm::FloatDefault, 3> spacing = TestValue(1, vtkm::Vec<vtkm::FloatDefault, 3>());

  {
    const vtkm::Id threadIndex = 0;
    const vtkm::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const vtkm::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const vtkm::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    vtkm::VecAxisAlignedPointCoordinates<NumDimensions> value =
      fetch.Load(ThreadIndicesType(
                   threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain()),
                 invocation.Parameters.template GetParameter<ParamIndex>());
    VTKM_TEST_ASSERT(test_equal(value.GetOrigin(), origin), "Bad origin.");
    VTKM_TEST_ASSERT(test_equal(value.GetSpacing(), spacing), "Bad spacing.");
  }

  origin[0] += spacing[0];
  {
    const vtkm::Id threadIndex = 1;
    const vtkm::Id outputIndex = invocation.ThreadToOutputMap.Get(threadIndex);
    const vtkm::Id inputIndex = invocation.OutputToInputMap.Get(outputIndex);
    const vtkm::IdComponent visitIndex = invocation.VisitArray.Get(outputIndex);
    vtkm::VecAxisAlignedPointCoordinates<NumDimensions> value =
      fetch.Load(ThreadIndicesType(
                   threadIndex, inputIndex, visitIndex, outputIndex, invocation.GetInputDomain()),
                 invocation.Parameters.template GetParameter<ParamIndex>());
    VTKM_TEST_ASSERT(test_equal(value.GetOrigin(), origin), "Bad origin.");
    VTKM_TEST_ASSERT(test_equal(value.GetSpacing(), spacing), "Bad spacing.");
  }
}

template <vtkm::IdComponent NumDimensions>
void TryStructuredPointCoordinates(
  const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                           vtkm::TopologyElementTagCell,
                                           NumDimensions>& connectivity,
  const vtkm::internal::ArrayPortalUniformPointCoordinates& coordinates)
{
  using BaseFunctionInterface = vtkm::internal::FunctionInterface<void(vtkm::internal::NullType,
                                                                       vtkm::internal::NullType,
                                                                       vtkm::internal::NullType,
                                                                       vtkm::internal::NullType,
                                                                       vtkm::internal::NullType)>;

  // Try with topology in argument 1 and point coordinates in argument 2
  TryStructuredPointCoordinatesInvocation<NumDimensions, 2>(vtkm::internal::make_Invocation<1>(
    BaseFunctionInterface().Replace<1>(connectivity).template Replace<2>(coordinates),
    BaseFunctionInterface(),
    BaseFunctionInterface(),
    TestIndexPortal(),
    TestZeroPortal(),
    TestIndexPortal()));
  // Try again with topology in argument 3 and point coordinates in argument 1
  TryStructuredPointCoordinatesInvocation<NumDimensions, 1>(vtkm::internal::make_Invocation<3>(
    BaseFunctionInterface().Replace<3>(connectivity).template Replace<1>(coordinates),
    BaseFunctionInterface(),
    BaseFunctionInterface(),
    TestIndexPortal(),
    TestZeroPortal(),
    TestIndexPortal()));
}

void TryStructuredPointCoordinates()
{
  std::cout << "*** Fetching special case of uniform point coordinates. *****" << std::endl;

  vtkm::internal::ArrayPortalUniformPointCoordinates coordinates(
    vtkm::Id3(3, 2, 2),
    TestValue(0, vtkm::Vec<vtkm::FloatDefault, 3>()),
    TestValue(1, vtkm::Vec<vtkm::FloatDefault, 3>()));

  std::cout << "3D" << std::endl;
  vtkm::internal::ConnectivityStructuredInternals<3> connectivityInternals3d;
  connectivityInternals3d.SetPointDimensions(vtkm::Id3(3, 2, 2));
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell, 3>
    connectivity3d(connectivityInternals3d);
  TryStructuredPointCoordinates(connectivity3d, coordinates);

  std::cout << "2D" << std::endl;
  vtkm::internal::ConnectivityStructuredInternals<2> connectivityInternals2d;
  connectivityInternals2d.SetPointDimensions(vtkm::Id2(3, 2));
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell, 2>
    connectivity2d(connectivityInternals2d);
  TryStructuredPointCoordinates(connectivity2d, coordinates);

  std::cout << "1D" << std::endl;
  vtkm::internal::ConnectivityStructuredInternals<1> connectivityInternals1d;
  connectivityInternals1d.SetPointDimensions(3);
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell, 1>
    connectivity1d(connectivityInternals1d);
  TryStructuredPointCoordinates(connectivity1d, coordinates);
}

void TestArrayTopologyMapIn()
{
  vtkm::testing::Testing::TryTypes(TryType(), vtkm::TypeListTagCommon());

  TryStructuredPointCoordinates();
}

} // anonymous namespace

int UnitTestFetchArrayTopologyMapIn(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestArrayTopologyMapIn, argc, argv);
}
