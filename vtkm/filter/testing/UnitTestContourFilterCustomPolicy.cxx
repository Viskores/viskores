//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/Contour.h>
#include <vtkm/filter/Contour.hxx>

namespace vtkm_ut_mc_policy
{
class EuclideanNorm
{
public:
  VTKM_EXEC_CONT
  EuclideanNorm()
    : Reference(0., 0., 0.)
  {
  }
  VTKM_EXEC_CONT
  EuclideanNorm(vtkm::Vec3f_32 reference)
    : Reference(reference)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Float32 operator()(vtkm::Vec3f_32 v) const
  {
    vtkm::Vec3f_32 d(
      v[0] - this->Reference[0], v[1] - this->Reference[1], v[2] - this->Reference[2]);
    return vtkm::Magnitude(d);
  }

private:
  vtkm::Vec3f_32 Reference;
};

class CubeGridConnectivity
{
public:
  VTKM_EXEC_CONT
  CubeGridConnectivity()
    : Dimension(1)
    , DimSquared(1)
    , DimPlus1Squared(4)
  {
  }
  VTKM_EXEC_CONT
  CubeGridConnectivity(vtkm::Id dim)
    : Dimension(dim)
    , DimSquared(dim * dim)
    , DimPlus1Squared((dim + 1) * (dim + 1))
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id vertex) const
  {
    using HexTag = vtkm::CellShapeTagHexahedron;
    using HexTraits = vtkm::CellTraits<HexTag>;

    vtkm::Id cellId = vertex / HexTraits::NUM_POINTS;
    vtkm::Id localId = vertex % HexTraits::NUM_POINTS;
    vtkm::Id globalId =
      (cellId + cellId / this->Dimension + (this->Dimension + 1) * (cellId / (this->DimSquared)));

    switch (localId)
    {
      case 0:
        break;
      case 1:
        globalId += 1;
        break;
      case 2:
        globalId += this->Dimension + 2;
        break;
      case 3:
        globalId += this->Dimension + 1;
        break;
      case 4:
        globalId += this->DimPlus1Squared;
        break;
      case 5:
        globalId += this->DimPlus1Squared + 1;
        break;
      case 6:
        globalId += this->Dimension + this->DimPlus1Squared + 2;
        break;
      case 7:
        globalId += this->Dimension + this->DimPlus1Squared + 1;
        break;
    }

    return globalId;
  }

private:
  vtkm::Id Dimension;
  vtkm::Id DimSquared;
  vtkm::Id DimPlus1Squared;
};

class MakeRadiantDataSet
{
public:
  using CoordinateArrayHandle = vtkm::cont::ArrayHandleUniformPointCoordinates;
  using DataArrayHandle =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleUniformPointCoordinates, EuclideanNorm>;
  using ConnectivityArrayHandle =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleCounting<vtkm::Id>,
                                     CubeGridConnectivity>;

  using CellSet = vtkm::cont::CellSetSingleType<
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleCounting<vtkm::Id>,
                                     CubeGridConnectivity>::StorageTag>;

  vtkm::cont::DataSet Make3DRadiantDataSet(vtkm::IdComponent dim = 5);
};

class PolicyRadiantDataSet : public vtkm::filter::PolicyBase<PolicyRadiantDataSet>
{
public:
  using TypeListRadiantCellSetTypes = vtkm::List<MakeRadiantDataSet::CellSet>;

  using AllCellSetList = TypeListRadiantCellSetTypes;
};

inline vtkm::cont::DataSet MakeRadiantDataSet::Make3DRadiantDataSet(vtkm::IdComponent dim)
{
  // create a cube from -.5 to .5 in x,y,z, consisting of <dim> cells on each
  // axis, with point values equal to the Euclidean distance from the origin.

  vtkm::cont::DataSet dataSet;

  using HexTag = vtkm::CellShapeTagHexahedron;
  using HexTraits = vtkm::CellTraits<HexTag>;

  using CoordType = vtkm::Vec3f_32;

  const vtkm::IdComponent nCells = dim * dim * dim;

  vtkm::Float32 spacing = vtkm::Float32(1. / dim);
  CoordinateArrayHandle coordinates(vtkm::Id3(dim + 1, dim + 1, dim + 1),
                                    CoordType(-.5, -.5, -.5),
                                    CoordType(spacing, spacing, spacing));

  DataArrayHandle distanceToOrigin(coordinates);
  DataArrayHandle distanceToOther(coordinates, EuclideanNorm(CoordType(1., 1., 1.)));

  ConnectivityArrayHandle connectivity(
    vtkm::cont::ArrayHandleCounting<vtkm::Id>(0, 1, nCells * HexTraits::NUM_POINTS),
    CubeGridConnectivity(dim));

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(vtkm::cont::Field(
    "distanceToOrigin", vtkm::cont::Field::Association::POINTS, distanceToOrigin));
  dataSet.AddField(
    vtkm::cont::Field("distanceToOther", vtkm::cont::Field::Association::POINTS, distanceToOther));

  CellSet cellSet;
  cellSet.Fill(coordinates.GetNumberOfValues(), HexTag::Id, HexTraits::NUM_POINTS, connectivity);

  dataSet.SetCellSet(cellSet);

  return dataSet;
}

void TestContourCustomPolicy()
{
  std::cout << "Testing Contour filter with custom field and cellset" << std::endl;

  using DataSetGenerator = MakeRadiantDataSet;
  DataSetGenerator dataSetGenerator;

  const vtkm::IdComponent Dimension = 10;
  vtkm::cont::DataSet dataSet = dataSetGenerator.Make3DRadiantDataSet(Dimension);

  vtkm::filter::Contour mc;

  mc.SetGenerateNormals(false);
  mc.SetIsoValue(0, 0.45);
  mc.SetIsoValue(1, 0.45);
  mc.SetIsoValue(2, 0.45);
  mc.SetIsoValue(3, 0.45);

  //We specify a custom execution policy here, since the "distanceToOrigin" is a
  //custom field type
  mc.SetActiveField("distanceToOrigin");
  mc.SetFieldsToPass({ "distanceToOrigin", "distanceToOther" });
  vtkm::cont::DataSet outputData = mc.Execute(dataSet, PolicyRadiantDataSet{});

  VTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 2,
                   "Wrong number of fields in the output dataset");

  vtkm::cont::CoordinateSystem coords = outputData.GetCoordinateSystem();
  VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == (414 * 4), "Should have some coordinates");
}

} //  namespace

int UnitTestContourFilterCustomPolicy(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(vtkm_ut_mc_policy::TestContourCustomPolicy, argc, argv);
}
