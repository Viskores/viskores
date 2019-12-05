//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_Testing_h
#define vtk_m_cont_testing_Testing_h

#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/Error.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/testing/Testing.h>
#include <vtkm/thirdparty/diy/Configure.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/VariantArrayHandle.h>

#include <vtkm/thirdparty/diy/serialization.h>

namespace vtkm
{
namespace cont
{
namespace testing
{

struct Testing
{
public:
  template <class Func>
  static VTKM_CONT int Run(Func function, int& argc, char* argv[])
  {
    vtkm::cont::Initialize(argc, argv, vtkm::cont::InitializeOptions::Strict);

    try
    {
      function();
    }
    catch (vtkm::testing::Testing::TestFailure& error)
    {
      std::cout << "***** Test failed @ " << error.GetFile() << ":" << error.GetLine() << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (vtkm::cont::Error& error)
    {
      std::cout << "***** Uncaught VTKm exception thrown." << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (std::exception& error)
    {
      std::cout << "***** STL exception throw." << std::endl << error.what() << std::endl;
    }
    catch (...)
    {
      std::cout << "***** Unidentified exception thrown." << std::endl;
      return 1;
    }
    return 0;
  }

  template <class Func>
  static VTKM_CONT int RunOnDevice(Func function, int argc, char* argv[])
  {
    auto opts =
      vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::Strict;
    auto config = vtkm::cont::Initialize(argc, argv, opts);

    try
    {
      function(config.Device);
    }
    catch (vtkm::testing::Testing::TestFailure& error)
    {
      std::cout << "***** Test failed @ " << error.GetFile() << ":" << error.GetLine() << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (vtkm::cont::Error& error)
    {
      std::cout << "***** Uncaught VTKm exception thrown." << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (std::exception& error)
    {
      std::cout << "***** STL exception throw." << std::endl << error.what() << std::endl;
    }
    catch (...)
    {
      std::cout << "***** Unidentified exception thrown." << std::endl;
      return 1;
    }
    return 0;
  }
};

struct Environment
{
  VTKM_CONT Environment(int* argc, char*** argv)
  {
#if defined(VTKM_ENABLE_MPI)
    int provided_threading;
    MPI_Init_thread(argc, argv, MPI_THREAD_FUNNELED, &provided_threading);

    // set the global communicator to use in VTKm.
    vtkmdiy::mpi::communicator comm(MPI_COMM_WORLD);
    vtkm::cont::EnvironmentTracker::SetCommunicator(comm);
#else
    (void)argc;
    (void)argv;
#endif
  }

  VTKM_CONT ~Environment()
  {
#if defined(VTKM_ENABLE_MPI)
    MPI_Finalize();
#endif
  }
};

//============================================================================
class TestEqualResult
{
public:
  void PushMessage(const std::string& msg) { this->Messages.push_back(msg); }

  const std::vector<std::string>& GetMessages() const { return this->Messages; }

  std::string GetMergedMessage() const
  {
    std::string msg;
    std::for_each(this->Messages.rbegin(), this->Messages.rend(), [&](const std::string& next) {
      msg += (msg.empty() ? "" : ": ");
      msg += next;
    });

    return msg;
  }

  operator bool() const { return this->Messages.empty(); }

private:
  std::vector<std::string> Messages;
};

namespace detail
{

struct TestEqualArrayHandle
{
  template <typename T1, typename T2, typename StorageTag1, typename StorageTag2>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T1, StorageTag1>&,
                            const vtkm::cont::ArrayHandle<T2, StorageTag2>&,
                            TestEqualResult& result) const
  {
    result.PushMessage("types don't match");
    return;
  }

  template <typename T, typename StorageTag1, typename StorageTag2>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, StorageTag1>& array1,
                            const vtkm::cont::ArrayHandle<T, StorageTag2>& array2,
                            TestEqualResult& result) const
  {
    if (array1.GetNumberOfValues() != array2.GetNumberOfValues())
    {
      result.PushMessage("sizes don't match");
      return;
    }
    auto portal1 = array1.GetPortalConstControl();
    auto portal2 = array2.GetPortalConstControl();
    for (vtkm::Id i = 0; i < portal1.GetNumberOfValues(); ++i)
    {
      if (!test_equal(portal1.Get(i), portal2.Get(i)))
      {
        result.PushMessage(std::string("values don't match at index ") + std::to_string(i));
        return;
      }
    }
  }

  template <typename T, typename StorageTag, typename TypeList>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, StorageTag>& array1,
                            const vtkm::cont::VariantArrayHandleBase<TypeList>& array2,
                            TestEqualResult& result) const
  {
    array2.CastAndCall(*this, array1, result);
  }

  template <typename T, typename StorageTag, typename TypeList>
  VTKM_CONT void operator()(const vtkm::cont::VariantArrayHandleBase<TypeList>& array1,
                            const vtkm::cont::ArrayHandle<T, StorageTag>& array2,
                            TestEqualResult& result) const
  {
    array1.CastAndCall(*this, array2, result);
  }

  template <typename TypeList1, typename TypeList2>
  VTKM_CONT void operator()(const vtkm::cont::VariantArrayHandleBase<TypeList1>& array1,
                            const vtkm::cont::VariantArrayHandleBase<TypeList2>& array2,
                            TestEqualResult& result) const
  {
    array2.CastAndCall(*this, array1, result);
  }
};
} // detail

template <typename ArrayHandle1, typename ArrayHandle2>
inline VTKM_CONT TestEqualResult test_equal_ArrayHandles(const ArrayHandle1& array1,
                                                         const ArrayHandle2& array2)
{
  TestEqualResult result;
  detail::TestEqualArrayHandle{}(array1, array2, result);
  return result;
}

namespace detail
{

struct TestEqualCellSet
{
  template <typename ShapeST, typename ConnectivityST, typename OffsetST>
  void operator()(const vtkm::cont::CellSetExplicit<ShapeST, ConnectivityST, OffsetST>& cs1,
                  const vtkm::cont::CellSetExplicit<ShapeST, ConnectivityST, OffsetST>& cs2,
                  TestEqualResult& result) const
  {
    vtkm::TopologyElementTagCell visitTopo{};
    vtkm::TopologyElementTagPoint incidentTopo{};

    if (cs1.GetNumberOfPoints() != cs2.GetNumberOfPoints())
    {
      result.PushMessage("number of points don't match");
      return;
    }

    result = test_equal_ArrayHandles(cs1.GetShapesArray(visitTopo, incidentTopo),
                                     cs2.GetShapesArray(visitTopo, incidentTopo));
    if (!result)
    {
      result.PushMessage("shapes arrays don't match");
      return;
    }

    result = test_equal_ArrayHandles(cs1.GetNumIndicesArray(visitTopo, incidentTopo),
                                     cs2.GetNumIndicesArray(visitTopo, incidentTopo));
    if (!result)
    {
      result.PushMessage("counts arrays don't match");
      return;
    }
    result = test_equal_ArrayHandles(cs1.GetConnectivityArray(visitTopo, incidentTopo),
                                     cs2.GetConnectivityArray(visitTopo, incidentTopo));
    if (!result)
    {
      result.PushMessage("connectivity arrays don't match");
      return;
    }
    result = test_equal_ArrayHandles(cs1.GetOffsetsArray(visitTopo, incidentTopo),
                                     cs2.GetOffsetsArray(visitTopo, incidentTopo));
    if (!result)
    {
      result.PushMessage("offsets arrays don't match");
      return;
    }
  }

  template <vtkm::IdComponent DIMENSION>
  void operator()(const vtkm::cont::CellSetStructured<DIMENSION>& cs1,
                  const vtkm::cont::CellSetStructured<DIMENSION>& cs2,
                  TestEqualResult& result) const
  {
    if (cs1.GetPointDimensions() != cs2.GetPointDimensions())
    {
      result.PushMessage("point dimensions don't match");
      return;
    }
  }

  template <typename CellSetTypes1, typename CellSetTypes2>
  void operator()(const vtkm::cont::DynamicCellSetBase<CellSetTypes1>& cs1,
                  const vtkm::cont::DynamicCellSetBase<CellSetTypes2>& cs2,
                  TestEqualResult& result) const
  {
    cs1.CastAndCall(*this, cs2, result);
  }

  template <typename CellSet, typename CellSetTypes>
  void operator()(const CellSet& cs,
                  const vtkm::cont::DynamicCellSetBase<CellSetTypes>& dcs,
                  TestEqualResult& result) const
  {
    if (!dcs.IsSameType(cs))
    {
      result.PushMessage("types don't match");
      return;
    }
    this->operator()(cs, dcs.template Cast<CellSet>(), result);
  }
};
} // detail

template <typename CellSet1, typename CellSet2>
inline VTKM_CONT TestEqualResult test_equal_CellSets(const CellSet1& cellset1,
                                                     const CellSet2& cellset2)
{
  TestEqualResult result;
  detail::TestEqualCellSet{}(cellset1, cellset2, result);
  return result;
}

template <typename FieldTypeList = VTKM_DEFAULT_TYPE_LIST>
inline VTKM_CONT TestEqualResult test_equal_Fields(const vtkm::cont::Field& f1,
                                                   const vtkm::cont::Field& f2,
                                                   FieldTypeList fTtypes = FieldTypeList())
{
  TestEqualResult result;

  if (f1.GetName() != f2.GetName())
  {
    result.PushMessage("names don't match");
    return result;
  }

  if (f1.GetAssociation() != f2.GetAssociation())
  {
    result.PushMessage("associations don't match");
    return result;
  }

  result =
    test_equal_ArrayHandles(f1.GetData().ResetTypes(fTtypes), f2.GetData().ResetTypes(fTtypes));
  if (!result)
  {
    result.PushMessage("data doesn't match");
  }

  return result;
}

template <typename CellSetTypes = VTKM_DEFAULT_CELL_SET_LIST,
          typename FieldTypeList = VTKM_DEFAULT_TYPE_LIST>
inline VTKM_CONT TestEqualResult test_equal_DataSets(const vtkm::cont::DataSet& ds1,
                                                     const vtkm::cont::DataSet& ds2,
                                                     CellSetTypes ctypes = CellSetTypes(),
                                                     FieldTypeList fTtypes = FieldTypeList())
{
  TestEqualResult result;
  if (ds1.GetNumberOfCoordinateSystems() != ds2.GetNumberOfCoordinateSystems())
  {
    result.PushMessage("number of coordinate systems don't match");
    return result;
  }
  for (vtkm::IdComponent i = 0; i < ds1.GetNumberOfCoordinateSystems(); ++i)
  {
    result = test_equal_ArrayHandles(ds1.GetCoordinateSystem(i).GetData(),
                                     ds2.GetCoordinateSystem(i).GetData());
    if (!result)
    {
      result.PushMessage(std::string("coordinate systems don't match at index ") +
                         std::to_string(i));
      return result;
    }
  }

  result = test_equal_CellSets(ds1.GetCellSet().ResetCellSetList(ctypes),
                               ds2.GetCellSet().ResetCellSetList(ctypes));
  if (!result)
  {
    result.PushMessage(std::string("cellsets don't match"));
    return result;
  }

  if (ds1.GetNumberOfFields() != ds2.GetNumberOfFields())
  {
    result.PushMessage("number of fields don't match");
    return result;
  }
  for (vtkm::IdComponent i = 0; i < ds1.GetNumberOfFields(); ++i)
  {
    result = test_equal_Fields(ds1.GetField(i), ds2.GetField(i), fTtypes);
    if (!result)
    {
      result.PushMessage(std::string("fields don't match at index ") + std::to_string(i));
      return result;
    }
  }

  return result;
}
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_internal_Testing_h
