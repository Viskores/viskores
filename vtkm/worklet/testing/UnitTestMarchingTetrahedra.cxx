//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/Contour.h>
#include <vtkm/filter/Tetrahedralize.h>
#include <vtkm/worklet/WorkletMapField.h>

class Tangle : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coord, FieldOut v);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::Vec<vtkm::FloatDefault, 3> coord, vtkm::Float32& v) const
  {
    v = vtkm::MagnitudeSquared(coord);
  }
};

vtkm::cont::DataSet MakeTangleDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataset;

  // create a point coordinate system
  const vtkm::Id3 vdims{ dims[0] + 1, dims[1] + 1, dims[2] + 1 };
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates{
    vdims,
    vtkm::Vec<vtkm::FloatDefault, 3>(-1.0, -1.0, -1.0),
    vtkm::Vec<vtkm::FloatDefault, 3>(2.0f / vdims[0], 2.0f / vdims[1], 2.0f / vdims[2])
  };
  dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  // generate point field from point coordinates.
  vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
  vtkm::worklet::DispatcherMapField<Tangle> dispatcherMapField(Tangle{});
  dispatcherMapField.Invoke(coordinates, pointFieldArray);
  dataset.AddField(
    vtkm::cont::Field("nodevar", vtkm::cont::Field::Association::POINTS, pointFieldArray));

  // add cell set
  vtkm::cont::CellSetStructured<3> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataset.AddCellSet(cellSet);

  return dataset;
}

void TestMarchingTetrahedra()
{
  vtkm::cont::DataSet input = MakeTangleDataSet({ 2, 2, 2 });

  vtkm::filter::Tetrahedralize tetrahedralize;
  vtkm::cont::DataSet tetra = tetrahedralize.Execute(input);

  std::cout << "number of tetras: " << tetra.GetCellSet().GetNumberOfCells() << std::endl;
  vtkm::cont::ArrayHandle<vtkm::Float32> pointFieldArray;
  tetra.GetField("nodevar").GetData().CopyTo(pointFieldArray);

  vtkm::Float32 isoValue = 0.5;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> verticesArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> normalsArray;
  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsArray;

  vtkm::worklet::Contour isosurfaceFilter;
  isosurfaceFilter.SetMergeDuplicatePoints(false);

  auto result = isosurfaceFilter.Run(&isoValue,
                                     1,
                                     tetra.GetCellSet(0),
                                     tetra.GetCoordinateSystem(),
                                     pointFieldArray,
                                     verticesArray,
                                     normalsArray);

  result.PrintSummary(std::cout);
}

int UnitTestMarchingTetrahedra(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMarchingTetrahedra, argc, argv);
}