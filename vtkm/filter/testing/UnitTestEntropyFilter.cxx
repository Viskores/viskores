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

#include <vtkm/filter/Entropy.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>


namespace
{

///// dataset and field generator from "vtkm-m/vtkm/rendering/testing/UnitTestMapperVolume.cxx" /////
class TangleField : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn<IdType> vertexId, FieldOut<Scalar> v);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  const vtkm::Id xdim, ydim, zdim;
  const vtkm::Float32 xmin, ymin, zmin, xmax, ymax, zmax;
  const vtkm::Id cellsPerLayer;

  VTKM_CONT
  TangleField(const vtkm::Id3 dims, const vtkm::Float32 mins[3], const vtkm::Float32 maxs[3])
    : xdim(dims[0])
    , ydim(dims[1])
    , zdim(dims[2])
    , xmin(mins[0])
    , ymin(mins[1])
    , zmin(mins[2])
    , xmax(maxs[0])
    , ymax(maxs[1])
    , zmax(maxs[2])
    , cellsPerLayer((xdim) * (ydim)){};

  VTKM_EXEC
  void operator()(const vtkm::Id& vertexId, vtkm::Float32& v) const
  {
    const vtkm::Id x = vertexId % (xdim);
    const vtkm::Id y = (vertexId / (xdim)) % (ydim);
    const vtkm::Id z = vertexId / cellsPerLayer;

    const vtkm::Float32 fx = static_cast<vtkm::Float32>(x) / static_cast<vtkm::Float32>(xdim - 1);
    const vtkm::Float32 fy = static_cast<vtkm::Float32>(y) / static_cast<vtkm::Float32>(xdim - 1);
    const vtkm::Float32 fz = static_cast<vtkm::Float32>(z) / static_cast<vtkm::Float32>(xdim - 1);

    const vtkm::Float32 xx = 3.0f * (xmin + (xmax - xmin) * (fx));
    const vtkm::Float32 yy = 3.0f * (ymin + (ymax - ymin) * (fy));
    const vtkm::Float32 zz = 3.0f * (zmin + (zmax - zmin) * (fz));

    v = (xx * xx * xx * xx - 5.0f * xx * xx + yy * yy * yy * yy - 5.0f * yy * yy +
         zz * zz * zz * zz - 5.0f * zz * zz + 11.8f) *
        0.2f +
      0.5f;
  }
};


// Construct an input data set using the tangle field worklet
vtkm::cont::DataSet MakeIsosurfaceTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  vtkm::Float32 mins[3] = { -1.0f, -1.0f, -1.0f };
  vtkm::Float32 maxs[3] = { 1.0f, 1.0f, 1.0f };

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> vertexCountImplicitArray(
    0, 1, vdims[0] * vdims[1] * vdims[2]);
  vtkm::worklet::DispatcherMapField<TangleField> tangleFieldDispatcher(
    TangleField(vdims, mins, maxs));
  tangleFieldDispatcher.Invoke(vertexCountImplicitArray, fieldArray);

  vtkm::Vec<vtkm::FloatDefault, 3> origin(0.0f, 0.0f, 0.0f);
  vtkm::Vec<vtkm::FloatDefault, 3> spacing(1.0f / static_cast<vtkm::FloatDefault>(dims[0]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[2]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[1]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  dataSet.AddField(
    vtkm::cont::Field("nodevar", vtkm::cont::Field::Association::POINTS, fieldArray));

  static constexpr vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

void TestEntropy()
{
  ///// make a data set /////
  vtkm::Id3 dims(32, 32, 32);
  vtkm::cont::DataSet dataSet = MakeIsosurfaceTestDataSet(dims);

  vtkm::filter::Entropy entropyFilter;

  ///// calculate entropy of "nodevar" field of the data set /////
  entropyFilter.SetNumberOfBins(50); //set number of bins
  entropyFilter.SetActiveField("nodevar");
  vtkm::cont::DataSet resultEntropy = entropyFilter.Execute(dataSet);

  ///// get entropy from resultEntropy /////
  vtkm::cont::ArrayHandle<vtkm::Float64> entropy;
  resultEntropy.GetField("entropy").GetData().CopyTo(entropy);
  vtkm::cont::ArrayHandle<vtkm::Float64>::PortalConstControl portal =
    entropy.GetPortalConstControl();
  vtkm::Float64 entropyFromFilter = portal.Get(0);

  /////// check if calculating entopry is close enough to ground truth value /////
  VTKM_TEST_ASSERT(fabs(entropyFromFilter - 4.59093) < 0.001, "Entropy calculation is incorrect");
} // TestFieldEntropy
}

int UnitTestEntropyFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestEntropy);
}
