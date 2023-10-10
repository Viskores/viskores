//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/contour/SliceMultiple.h>
#include <vtkm/io/VTKDataSetWriter.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
namespace
{
class SetPointValuesWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4);
  template <typename CoordinatesType, typename ScalarType, typename V3Type, typename V4Type>
  VTKM_EXEC void operator()(const CoordinatesType& coordinates,
                            ScalarType& scalar,
                            V3Type& vec3,
                            V4Type& vec4) const
  {
    scalar =
      static_cast<ScalarType>((coordinates[2] * 3 * 3 + coordinates[1] * 3 + coordinates[0]) * 0.1);
    vec3 = { coordinates[0] * 0.1, coordinates[1] * 0.1, coordinates[2] * 0.1 };
    vec4 = {
      coordinates[0] * 0.1, coordinates[1] * 0.1, coordinates[2] * 0.1, coordinates[0] * 0.1
    };
    return;
  }
};

class SetCellValuesWorklet : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn, FieldInPoint, FieldOutCell, FieldOutCell, FieldOutCell);
  using ExecutionSignature = void(_2, _3, _4, _5);
  using InputDomain = _1;
  template <typename PointFieldVecType, typename ScalarType, typename V3Type, typename V4Type>
  VTKM_EXEC void operator()(const PointFieldVecType& pointFieldVec,
                            ScalarType& scalar,
                            V3Type& vec3,
                            V4Type& vec4) const
  {
    //pointFieldVec has 8 values
    scalar = static_cast<ScalarType>(pointFieldVec[0]);
    vec3 = { pointFieldVec[0] * 0.1, pointFieldVec[1] * 0.1, pointFieldVec[2] * 0.1 };
    vec4 = {
      pointFieldVec[0] * 0.1, pointFieldVec[1] * 0.1, pointFieldVec[2] * 0.1, pointFieldVec[3] * 0.1
    };
    return;
  }
};

vtkm::cont::DataSet MakeTestDatasetStructured3D()
{
  static constexpr vtkm::Id xdim = 3, ydim = 3, zdim = 3;
  static const vtkm::Id3 dim(xdim, ydim, zdim);
  vtkm::cont::DataSet ds;
  ds = vtkm::cont::DataSetBuilderUniform::Create(
    dim, vtkm::Vec3f(-1.0f, -1.0f, -1.0f), vtkm::Vec3f(1, 1, 1));
  vtkm::cont::ArrayHandle<vtkm::Float64> pointScalars;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> pointV3;
  //a customized vector which is not in vtkm::TypeListCommon{}
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 4>> pointV4;
  pointScalars.Allocate(xdim * ydim * zdim);
  pointV3.Allocate(xdim * ydim * zdim);
  pointV4.Allocate(xdim * ydim * zdim);
  vtkm::cont::Invoker invoker;
  invoker(
    SetPointValuesWorklet{}, ds.GetCoordinateSystem().GetData(), pointScalars, pointV3, pointV4);
  ds.AddPointField("pointScalars", pointScalars);
  ds.AddPointField("pointV3", pointV3);
  ds.AddPointField("pointV4", pointV4);
  //adding cell data
  vtkm::cont::ArrayHandle<vtkm::Float64> cellScalars;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> cellV3;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 4>> cellV4;
  vtkm::Id NumCells = ds.GetNumberOfCells();
  cellScalars.Allocate(NumCells);
  cellV3.Allocate(NumCells);
  cellV4.Allocate(NumCells);
  invoker(SetCellValuesWorklet{}, ds.GetCellSet(), pointScalars, cellScalars, cellV3, cellV4);
  ds.AddCellField("cellScalars", cellScalars);
  ds.AddCellField("cellV3", cellV3);
  ds.AddCellField("cellV4", cellV4);
  return ds;
}

void TestSliceMultipleFilter()
{
  auto ds = MakeTestDatasetStructured3D();
  vtkm::Plane plane1({ 0, 0, 0 }, { 0, 0, 1 });
  vtkm::Plane plane2({ 0, 0, 0 }, { 0, 1, 0 });
  vtkm::Plane plane3({ 0, 0, 0 }, { 1, 0, 0 });
  vtkm::filter::contour::SliceMultiple sliceMultiple;
  sliceMultiple.AddImplicitFunction(plane1);
  sliceMultiple.AddImplicitFunction(plane2);
  sliceMultiple.AddImplicitFunction(plane3);
  auto result = sliceMultiple.Execute(ds);
  VTKM_TEST_ASSERT(result.GetNumberOfPoints() == 27, "wrong number of points in merged data set");
  VTKM_TEST_ASSERT(result.GetCoordinateSystem().GetData().GetNumberOfValues() == 27,
                   "wrong number of scalars in merged data set");
  vtkm::cont::ArrayHandle<vtkm::Float64> CheckingScalars;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> CheckingV3;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 4>> CheckingV4;
  vtkm::cont::Invoker invoker;
  invoker(SetPointValuesWorklet{},
          result.GetCoordinateSystem().GetData(),
          CheckingScalars,
          CheckingV3,
          CheckingV4);
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(CheckingScalars, result.GetField("pointScalars").GetData()),
    "wrong scalar values");
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(CheckingV3, result.GetField("pointV3").GetData()),
                   "wrong pointV3 values");
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(CheckingV4, result.GetField("pointV4").GetData()),
                   "wrong pointV4 values");
  VTKM_TEST_ASSERT(result.GetNumberOfCells() == 24, "wrong number of cells in merged data set");
}
} // anonymous namespace
int UnitTestSliceMultipleFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSliceMultipleFilter, argc, argv);
}
