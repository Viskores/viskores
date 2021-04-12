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
#include <vtkm/cont/internal/OptionParser.h>
#include <vtkm/testing/Testing.h>
#include <vtkm/thirdparty/diy/Configure.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/cont/testing/vtkm_cont_testing_export.h>

#include <sstream>
#include <vtkm/thirdparty/diy/diy.h>

// We could, conceivably, use CUDA or Kokkos specific print statements here.
// But we cannot use std::stringstream on device, so for now, we'll just accept
// that on CUDA and Kokkos we print less actionalble information.
#if defined(VTKM_ENABLE_CUDA) || defined(VTKM_ENABLE_KOKKOS)
#define VTKM_MATH_ASSERT(condition, message) \
  {                                          \
    if (!(condition))                        \
    {                                        \
      this->RaiseError(message);             \
    }                                        \
  }
#else
#define VTKM_MATH_ASSERT(condition, message)                                                       \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      std::stringstream ss;                                                                        \
      ss << "\n\tError at " << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\n\t" << message \
         << "\n";                                                                                  \
      this->RaiseError(ss.str().c_str());                                                          \
    }                                                                                              \
  }
#endif

namespace opt = vtkm::cont::internal::option;

namespace vtkm
{
namespace cont
{
namespace testing
{

enum TestOptionsIndex
{
  TEST_UNKNOWN,
  DATADIR,     // base dir containing test data files
  BASELINEDIR, // base dir for regression test images
  WRITEDIR     // base dir for generated regression test images
};

struct TestVtkmArg : public opt::Arg
{
  static opt::ArgStatus Required(const opt::Option& option, bool msg)
  {
    if (option.arg == nullptr)
    {
      if (msg)
      {
        VTKM_LOG_ALWAYS_S(vtkm::cont::LogLevel::Error,
                          "Missing argument after option '"
                            << std::string(option.name, static_cast<size_t>(option.namelen))
                            << "'.\n");
      }
      return opt::ARG_ILLEGAL;
    }
    else
    {
      return opt::ARG_OK;
    }
  }

  // Method used for guessing whether an option that do not support (perhaps that calling
  // program knows about it) has an option attached to it (which should also be ignored).
  static opt::ArgStatus Unknown(const opt::Option& option, bool msg)
  {
    // If we don't have an arg, obviously we don't have an arg.
    if (option.arg == nullptr)
    {
      return opt::ARG_NONE;
    }

    // The opt::Arg::Optional method will return that the ARG is OK if and only if
    // the argument is attached to the option (e.g. --foo=bar). If that is the case,
    // then we definitely want to report that the argument is OK.
    if (opt::Arg::Optional(option, msg) == opt::ARG_OK)
    {
      return opt::ARG_OK;
    }

    // Now things get tricky. Maybe the next argument is an option or maybe it is an
    // argument for this option. We will guess that if the next argument does not
    // look like an option, we will treat it as such.
    if (option.arg[0] == '-')
    {
      return opt::ARG_NONE;
    }
    else
    {
      return opt::ARG_OK;
    }
  }
};

struct Testing
{
public:
  static VTKM_CONT const std::string GetTestDataBasePath() { return SetAndGetTestDataBasePath(); }

  static VTKM_CONT const std::string DataPath(const std::string& filename)
  {
    return GetTestDataBasePath() + filename;
  }

  static VTKM_CONT const std::string GetRegressionTestImageBasePath()
  {
    return SetAndGetRegressionImageBasePath();
  }

  static VTKM_CONT const std::string RegressionImagePath(const std::string& filename)
  {
    return GetRegressionTestImageBasePath() + filename;
  }

  static VTKM_CONT const std::string GetWriteDirBasePath() { return SetAndGetWriteDirBasePath(); }

  static VTKM_CONT const std::string WriteDirPath(const std::string& filename)
  {
    return GetWriteDirBasePath() + filename;
  }

  template <class Func>
  static VTKM_CONT int Run(Func function, int& argc, char* argv[])
  {
    std::unique_ptr<vtkmdiy::mpi::environment> env_diy = nullptr;
    if (!vtkmdiy::mpi::environment::initialized())
    {
      env_diy.reset(new vtkmdiy::mpi::environment(argc, argv));
    }

    vtkm::cont::Initialize(argc, argv);
    ParseAdditionalTestArgs(argc, argv);

    try
    {
      function();
    }
    catch (vtkm::testing::Testing::TestFailure const& error)
    {
      std::cerr << "Error at " << error.GetFile() << ":" << error.GetLine() << ":"
                << error.GetFunction() << "\n\t" << error.GetMessage() << "\n";
      return 1;
    }
    catch (vtkm::cont::Error const& error)
    {
      std::cerr << "Uncaught VTKm exception thrown.\n" << error.GetMessage() << "\n";
      std::cerr << "Stacktrace:\n" << error.GetStackTrace() << "\n";
      return 1;
    }
    catch (std::exception const& error)
    {
      std::cerr << "STL exception throw.\n" << error.what() << "\n";
      return 1;
    }
    catch (...)
    {
      std::cerr << "Unidentified exception thrown.\n";
      return 1;
    }
    return 0;
  }

  template <class Func>
  static VTKM_CONT int RunOnDevice(Func function, int argc, char* argv[])
  {
    auto opts = vtkm::cont::InitializeOptions::RequireDevice;
    auto config = vtkm::cont::Initialize(argc, argv, opts);
    ParseAdditionalTestArgs(argc, argv);

    try
    {
      function(config.Device);
    }
    catch (vtkm::testing::Testing::TestFailure& error)
    {
      std::cerr << "Error at " << error.GetFile() << ":" << error.GetLine() << ":"
                << error.GetFunction() << "\n\t" << error.GetMessage() << "\n";
      return 1;
    }
    catch (vtkm::cont::Error& error)
    {
      std::cerr << "Uncaught VTKm exception thrown.\n" << error.GetMessage() << "\n";
      std::cerr << "Stacktrace:\n" << error.GetStackTrace() << "\n";
      return 1;
    }
    catch (std::exception& error)
    {
      std::cerr << "STL exception throw.\n\t" << error.what() << "\n";
      return 1;
    }
    catch (...)
    {
      std::cerr << "Unidentified exception thrown.\n";
      return 1;
    }
    return 0;
  }

private:
  static std::string& SetAndGetTestDataBasePath(std::string path = "")
  {
    static std::string TestDataBasePath;

    if (!path.empty())
    {
      TestDataBasePath = path;
      if ((TestDataBasePath.back() != '/') && (TestDataBasePath.back() != '\\'))
      {
        TestDataBasePath = TestDataBasePath + "/";
      }
    }

    if (TestDataBasePath.empty())
    {
      VTKM_LOG_S(
        vtkm::cont::LogLevel::Error,
        "TestDataBasePath was never set, was --data-dir set correctly? (hint: ../data/data)");
    }

    return TestDataBasePath;
  }

  static std::string& SetAndGetRegressionImageBasePath(std::string path = "")
  {
    static std::string RegressionTestImageBasePath;

    if (!path.empty())
    {
      RegressionTestImageBasePath = path;
      if ((RegressionTestImageBasePath.back() != '/') &&
          (RegressionTestImageBasePath.back() != '\\'))
      {
        RegressionTestImageBasePath = RegressionTestImageBasePath + '/';
      }
    }

    if (RegressionTestImageBasePath.empty())
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Error,
                 "RegressionTestImageBasePath was never set, was --baseline-dir set correctly? "
                 "(hint: ../data/baseline)");
    }

    return RegressionTestImageBasePath;
  }

  static std::string& SetAndGetWriteDirBasePath(std::string path = "")
  {
    static std::string WriteDirBasePath;

    if (!path.empty())
    {
      WriteDirBasePath = path;
      if ((WriteDirBasePath.back() != '/') && (WriteDirBasePath.back() != '\\'))
      {
        WriteDirBasePath = WriteDirBasePath + '/';
      }
    }

    return WriteDirBasePath;
  }

  // Method to parse the extra arguments given to unit tests
  static VTKM_CONT void ParseAdditionalTestArgs(int& argc, char* argv[])
  {
    { // Parse test arguments
      std::vector<opt::Descriptor> usage;

      usage.push_back({ DATADIR,
                        0,
                        "D",
                        "data-dir",
                        TestVtkmArg::Required,
                        "  --data-dir, -D "
                        "<data-dir-path> \tPath to the "
                        "base data directory in the VTK-m "
                        "src dir." });
      usage.push_back({ BASELINEDIR,
                        0,
                        "B",
                        "baseline-dir",
                        TestVtkmArg::Required,
                        "  --baseline-dir, -B "
                        "<baseline-dir-path> "
                        "\tPath to the base dir "
                        "for regression test "
                        "images" });
      usage.push_back({ WRITEDIR,
                        0,
                        "",
                        "write-dir",
                        TestVtkmArg::Required,
                        "  --write-dir "
                        "<write-dir-path> "
                        "\tPath to the write dir "
                        "to store generated "
                        "regression test images" });
      // Required to collect unknown arguments when help is off.
      usage.push_back({ TEST_UNKNOWN, 0, "", "", TestVtkmArg::Unknown, "" });
      usage.push_back({ 0, 0, 0, 0, 0, 0 });


      // Remove argv[0] (executable name) if present:
      int vtkmArgc = argc > 0 ? argc - 1 : 0;
      char** vtkmArgv = argc > 0 ? argv + 1 : argv;

      opt::Stats stats(usage.data(), vtkmArgc, vtkmArgv);
      std::unique_ptr<opt::Option[]> options{ new opt::Option[stats.options_max] };
      std::unique_ptr<opt::Option[]> buffer{ new opt::Option[stats.buffer_max] };
      opt::Parser parse(usage.data(), vtkmArgc, vtkmArgv, options.get(), buffer.get());

      if (parse.error())
      {
        std::cerr << "Internal Initialize parser error" << std::endl;
        exit(1);
      }

      if (options[DATADIR])
      {
        SetAndGetTestDataBasePath(options[DATADIR].arg);
      }

      if (options[BASELINEDIR])
      {
        SetAndGetRegressionImageBasePath(options[BASELINEDIR].arg);
      }

      if (options[WRITEDIR])
      {
        SetAndGetWriteDirBasePath(options[WRITEDIR].arg);
      }

      for (const opt::Option* opt = options[TEST_UNKNOWN]; opt != nullptr; opt = opt->next())
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                   "Unknown option to internal Initialize: " << opt->name << "\n");
      }

      for (int nonOpt = 0; nonOpt < parse.nonOptionsCount(); ++nonOpt)
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                   "Unknown argument to internal Initialize: " << parse.nonOption(nonOpt) << "\n");
      }
    }
  }
};

}
}
} // namespace vtkm::cont::testing

//============================================================================
template <typename T1, typename T2, typename StorageTag1, typename StorageTag2>
VTKM_CONT TestEqualResult
test_equal_ArrayHandles(const vtkm::cont::ArrayHandle<T1, StorageTag1>& array1,
                        const vtkm::cont::ArrayHandle<T2, StorageTag2>& array2)
{
  TestEqualResult result;

  if (array1.GetNumberOfValues() != array2.GetNumberOfValues())
  {
    result.PushMessage("Arrays have different sizes.");
    return result;
  }

  auto portal1 = array1.ReadPortal();
  auto portal2 = array2.ReadPortal();
  for (vtkm::Id i = 0; i < portal1.GetNumberOfValues(); ++i)
  {
    if (!test_equal(portal1.Get(i), portal2.Get(i)))
    {
      result.PushMessage("Values don't match at index " + std::to_string(i));
      break;
    }
  }

  return result;
}

VTKM_CONT_TESTING_EXPORT TestEqualResult
test_equal_ArrayHandles(const vtkm::cont::UnknownArrayHandle& array1,
                        const vtkm::cont::UnknownArrayHandle& array2);

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

inline VTKM_CONT TestEqualResult test_equal_Fields(const vtkm::cont::Field& f1,
                                                   const vtkm::cont::Field& f2)
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

  result = test_equal_ArrayHandles(f1.GetData(), f2.GetData());
  if (!result)
  {
    result.PushMessage("data doesn't match");
  }

  return result;
}

template <typename CellSetTypes = VTKM_DEFAULT_CELL_SET_LIST>
inline VTKM_CONT TestEqualResult test_equal_DataSets(const vtkm::cont::DataSet& ds1,
                                                     const vtkm::cont::DataSet& ds2,
                                                     CellSetTypes ctypes = CellSetTypes())
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
    result = test_equal_Fields(ds1.GetField(i), ds2.GetField(i));
    if (!result)
    {
      result.PushMessage(std::string("fields don't match at index ") + std::to_string(i));
      return result;
    }
  }

  return result;
}

#endif //vtk_m_cont_internal_Testing_h
