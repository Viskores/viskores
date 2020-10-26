//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/GhostCellClassify.h>
#include <vtkm/filter/ParticleAdvection.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/thirdparty/diy/environment.h>

namespace
{

#if 0
class TestingDataSetBuilder
{
public:
   enum class DataSetGenerationType
   {
       UNIFORM = 0,
       RECTILINEAR = 1,
       CURVILINEAR = 2,
       EXPLICIT_SINGLE = 3,
       EXPLICIT = 4
   };

  TestingDataSetBuilder() {}

  vtkm::cont::DataSet GenerateDataSet(const vtkm::Bounds& bounds, const vtkm::Id& dims, DataSetGenerationType dsType)
  {
    if (dsType == DataSetGenerationType::UNIFORM)
        return this->GenerateUniform(bounds, dims);
    else if (dsType == DataSetGenerationType::RECTILINEAR)
        return this->GenerateUniform(bounds, dims);
  }

  void SetDataSetTypeUniform() { this->DataSetType = DataSetGenerationType::UNIFORM; }
  void SetDataSetTypeRectilinear() { this->DataSetType = DataSetGenerationType::RECTILINEAR; }
  void SetDataSetTypeCurvilinear() { this->DataSetType = DataSetGenerationType::CURVILINEAR; }
  void SetDataSetTypeExplicitSingle() { this->DataSetType = DataSetGenerationType::EXPLICIT_SINGLE; }
  void SetDataSetTypeExplicit() { this->DataSetType = DataSetGenerationType::EXPLICIT; }

private:
  vtkm::cont::DataSet GenerateUniform(const vtkm::Bounds& bounds, const vtkm::Id& dims)
  {
    vtkm::Vec3f origin(static_cast<vtkm::FloatDefault>(bounds.X.Min),
                       static_cast<vtkm::FloatDefault>(bounds.Y.Min),
                       static_cast<vtkm::FloatDefault>(bounds.Z.Min));
    vtkm::Vec3f spacing(static_cast<vtkm::FloatDefault>(bounds.X.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[0] - 1)),
                        static_cast<vtkm::FloatDefault>(bounds.Y.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[1] - 1)),
                        static_cast<vtkm::FloatDefault>(bounds.Z.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[2] - 1)));

    vtkm::cont::DataSetBuilderUniform dataSetBuilder;
    vtkm::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);
    return ds;
  }

  DataSetGenerationType DataSetType;
};
#endif

vtkm::cont::ArrayHandle<vtkm::Vec3f> CreateConstantVectorField(vtkm::Id num, const vtkm::Vec3f& vec)
{
  vtkm::cont::ArrayHandleConstant<vtkm::Vec3f> vecConst;
  vecConst = vtkm::cont::make_ArrayHandleConstant(vec, num);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> vecField;
  vtkm::cont::ArrayCopy(vecConst, vecField);
  return vecField;
}

vtkm::cont::DataSet CreateUniformDataSet(const vtkm::Bounds& bounds,
                                         const vtkm::Id3& dims,
                                         bool addGhost = false)
{
  vtkm::Vec3f origin(static_cast<vtkm::FloatDefault>(bounds.X.Min),
                     static_cast<vtkm::FloatDefault>(bounds.Y.Min),
                     static_cast<vtkm::FloatDefault>(bounds.Z.Min));
  vtkm::Vec3f spacing(static_cast<vtkm::FloatDefault>(bounds.X.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[0] - 1)),
                      static_cast<vtkm::FloatDefault>(bounds.Y.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[1] - 1)),
                      static_cast<vtkm::FloatDefault>(bounds.Z.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[2] - 1)));

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);

  if (addGhost)
  {
    vtkm::filter::GhostCellClassify addGhostFilter;
    return addGhostFilter.Execute(ds);
  }
  return ds;
}

vtkm::cont::DataSet CreateRectilinearDataSet(const vtkm::Bounds& bounds,
                                             const vtkm::Id3& dims,
                                             bool addGhost = false)
{
  vtkm::cont::DataSetBuilderRectilinear dataSetBuilder;
  std::vector<vtkm::FloatDefault> xvals, yvals, zvals;

  vtkm::Vec3f spacing(static_cast<vtkm::FloatDefault>(bounds.X.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[0] - 1)),
                      static_cast<vtkm::FloatDefault>(bounds.Y.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[1] - 1)),
                      static_cast<vtkm::FloatDefault>(bounds.Z.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[2] - 1)));
  xvals.resize((size_t)dims[0]);
  xvals[0] = static_cast<vtkm::FloatDefault>(bounds.X.Min);
  for (size_t i = 1; i < (size_t)dims[0]; i++)
    xvals[i] = xvals[i - 1] + spacing[0];

  yvals.resize((size_t)dims[1]);
  yvals[0] = static_cast<vtkm::FloatDefault>(bounds.Y.Min);
  for (size_t i = 1; i < (size_t)dims[1]; i++)
    yvals[i] = yvals[i - 1] + spacing[1];

  zvals.resize((size_t)dims[2]);
  zvals[0] = static_cast<vtkm::FloatDefault>(bounds.Z.Min);
  for (size_t i = 1; i < (size_t)dims[2]; i++)
    zvals[i] = zvals[i - 1] + spacing[2];

  vtkm::cont::DataSet ds = dataSetBuilder.Create(xvals, yvals, zvals);

  if (addGhost)
  {
    vtkm::filter::GhostCellClassify addGhostFilter;
    return addGhostFilter.Execute(ds);
  }
  return ds;
}

enum class DataSetOption
{
  SINGLE = 0,
  CURVILINEAR,
  EXPLICIT
};

template <class CellSetType, vtkm::IdComponent NDIM>
static void MakeExplicitCells(const CellSetType& cellSet,
                              vtkm::Vec<vtkm::Id, NDIM>& cellDims,
                              vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
                              vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                              vtkm::cont::ArrayHandle<vtkm::Id>& conn)
{
  using Connectivity = vtkm::internal::ConnectivityStructuredInternals<NDIM>;

  vtkm::Id nCells = cellSet.GetNumberOfCells();
  vtkm::IdComponent nVerts = (NDIM == 2 ? 4 : 8);
  vtkm::Id connLen = (NDIM == 2 ? nCells * 4 : nCells * 8);

  conn.Allocate(connLen);
  shapes.Allocate(nCells);
  numIndices.Allocate(nCells);

  Connectivity structured;
  structured.SetPointDimensions(cellDims + vtkm::Vec<vtkm::Id, NDIM>(1));

  auto connPortal = conn.WritePortal();
  auto shapesPortal = shapes.WritePortal();
  auto numIndicesPortal = numIndices.WritePortal();
  vtkm::Id connectionIndex = 0;
  for (vtkm::Id cellIndex = 0; cellIndex < nCells; cellIndex++)
  {
    auto ptIds = structured.GetPointsOfCell(cellIndex);
    for (vtkm::IdComponent vertexIndex = 0; vertexIndex < nVerts; vertexIndex++, connectionIndex++)
      connPortal.Set(connectionIndex, ptIds[vertexIndex]);

    shapesPortal.Set(cellIndex, (NDIM == 2 ? vtkm::CELL_SHAPE_QUAD : vtkm::CELL_SHAPE_HEXAHEDRON));
    numIndicesPortal.Set(cellIndex, nVerts);
  }
}

vtkm::cont::DataSet CreateExplicitFromStructuredDataSet(const vtkm::Bounds& bounds,
                                                        const vtkm::Id3& dims,
                                                        DataSetOption option,
                                                        bool addGhost = false)
{
  using CoordType = vtkm::Vec3f;
  auto input = CreateUniformDataSet(bounds, dims, addGhost);

  auto inputCoords = input.GetCoordinateSystem(0).GetData();
  vtkm::cont::ArrayHandle<CoordType> explCoords;
  vtkm::cont::ArrayCopy(inputCoords, explCoords);

  vtkm::cont::DynamicCellSet cellSet = input.GetCellSet();
  vtkm::cont::ArrayHandle<vtkm::Id> conn;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  vtkm::cont::DataSet output;
  vtkm::cont::DataSetBuilderExplicit dsb;

  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;

  switch (option)
  {
    case DataSetOption::SINGLE:
      if (cellSet.IsType<Structured2DType>())
      {
        Structured2DType cells2D = cellSet.Cast<Structured2DType>();
        vtkm::Id2 cellDims = cells2D.GetCellDimensions();
        MakeExplicitCells(cells2D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, vtkm::CellShapeTagQuad(), 4, conn, "coordinates");
      }
      else
      {
        Structured3DType cells3D = cellSet.Cast<Structured3DType>();
        vtkm::Id3 cellDims = cells3D.GetCellDimensions();
        MakeExplicitCells(cells3D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, vtkm::CellShapeTagHexahedron(), 8, conn, "coordinates");
      }
      break;

    case DataSetOption::CURVILINEAR:
      // In this case the cell set/connectivity is the same as the input
      // Only the coords are no longer Uniform / Rectilinear
      output.SetCellSet(cellSet);
      output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", explCoords));
      break;

    case DataSetOption::EXPLICIT:
      if (cellSet.IsType<Structured2DType>())
      {
        Structured2DType cells2D = cellSet.Cast<Structured2DType>();
        vtkm::Id2 cellDims = cells2D.GetCellDimensions();
        MakeExplicitCells(cells2D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, shapes, numIndices, conn, "coordinates");
      }
      else
      {
        Structured3DType cells3D = cellSet.Cast<Structured3DType>();
        vtkm::Id3 cellDims = cells3D.GetCellDimensions();
        MakeExplicitCells(cells3D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, shapes, numIndices, conn, "coordinates");
      }
      break;
  }

  if (addGhost)
    output.AddField(input.GetField("vtkmGhostCells"));
  return output;
}

void TestBasic()
{
  std::cout << "Basic Tests" << std::endl;

  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Bounds bounds(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
  const vtkm::Vec3f vecX(1, 0, 0);

  //Test datasets with and without ghost cells.
  for (int ghostType = 0; ghostType < 2; ghostType++)
  {
    std::vector<vtkm::cont::DataSet> dataSets;
    bool addGhost = (ghostType == 1);
    vtkm::Id3 useDims;
    vtkm::Bounds useBounds;
    if (addGhost)
    {
      useDims = dims + vtkm::Id3(2, 2, 2);
      useBounds.X.Min = bounds.X.Min - 1;
      useBounds.X.Max = bounds.X.Max + 1;
      useBounds.Y.Min = bounds.Y.Min - 1;
      useBounds.Y.Max = bounds.Y.Max + 1;
      useBounds.Z.Min = bounds.Z.Min - 1;
      useBounds.Z.Max = bounds.Z.Max + 1;
    }
    else
    {
      useDims = dims;
      useBounds = bounds;
    }

    dataSets.push_back(CreateUniformDataSet(useBounds, useDims, addGhost));
    dataSets.push_back(CreateRectilinearDataSet(useBounds, useDims, addGhost));
    dataSets.push_back(
      CreateExplicitFromStructuredDataSet(useBounds, useDims, DataSetOption::SINGLE, addGhost));
    dataSets.push_back(CreateExplicitFromStructuredDataSet(
      useBounds, useDims, DataSetOption::CURVILINEAR, addGhost));
    dataSets.push_back(
      CreateExplicitFromStructuredDataSet(useBounds, useDims, DataSetOption::EXPLICIT, addGhost));

    for (auto& ds : dataSets)
    {
      auto vecField = CreateConstantVectorField(ds.GetNumberOfPoints(), vecX);
      ds.AddPointField("vector", vecField);

      const vtkm::FloatDefault x0(0.2);
      std::vector<vtkm::Particle> seeds = { vtkm::Particle(vtkm::Vec3f(x0, 1, 1), 0),
                                            vtkm::Particle(vtkm::Vec3f(x0, 2, 1), 1),
                                            vtkm::Particle(vtkm::Vec3f(x0, 3, 1), 2),
                                            vtkm::Particle(vtkm::Vec3f(x0, 3, 2), 3) };

      auto seedArray = vtkm::cont::make_ArrayHandle(seeds);

      vtkm::filter::ParticleAdvection particleAdvection;
      particleAdvection.SetStepSize(0.1f);
      particleAdvection.SetNumberOfSteps(20);
      particleAdvection.SetSeeds(seedArray);

      particleAdvection.SetActiveField("vector");
      auto output = particleAdvection.Execute(ds);

      //Validate the result is correct.
      VTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");

      vtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
      VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 4, "Wrong number of coordinates");

      vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
      VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 4, "Wrong number of cells");
    }
  }
}

void TestPartitionedDataSet()
{
  std::cout << "Partitioned data set" << std::endl;

  const vtkm::Id3 dimensions(5, 5, 5);
  const vtkm::Vec3f vecX(1, 0, 0);
  std::vector<vtkm::Bounds> bounds = { vtkm::Bounds(0, 4, 0, 4, 0, 4),
                                       vtkm::Bounds(4, 8, 0, 4, 0, 4),
                                       vtkm::Bounds(8, 12, 0, 4, 0, 4) };

  vtkm::Bounds globalBounds;
  for (auto& b : bounds)
    globalBounds.Include(b);

  const std::string fieldName = "vec";

  for (int gType = 1; gType < 2; gType++)
  {
    bool addGhost = (gType == 1);
    vtkm::Id3 useDims;
    std::vector<vtkm::Bounds> useBounds;
    vtkm::Bounds boundsWithGhosts;

    if (addGhost)
    {
      useDims = dimensions + vtkm::Id3(2, 2, 2);
      for (auto& b : bounds)
      {
        vtkm::Bounds b2;
        b2.X.Min = b.X.Min - 1;
        b2.X.Max = b.X.Max + 1;
        b2.Y.Min = b.Y.Min - 1;
        b2.Y.Max = b.Y.Max + 1;
        b2.Z.Min = b.Z.Min - 1;
        b2.Z.Max = b.Z.Max + 1;
        useBounds.push_back(b2);

        boundsWithGhosts.Include(b2);
      }
    }
    else
    {
      useDims = dimensions;
      useBounds = bounds;
    }

    for (int dsType = 0; dsType < 5; dsType++)
    {
      vtkm::cont::PartitionedDataSet pds;
      for (auto& b : useBounds)
      {
        vtkm::cont::DataSet ds;
        if (dsType == 0)
          ds = CreateUniformDataSet(b, useDims, addGhost);
        else if (dsType == 1)
          ds = CreateRectilinearDataSet(b, useDims, addGhost);
        else if (dsType == 2)
          ds = CreateExplicitFromStructuredDataSet(b, useDims, DataSetOption::SINGLE, addGhost);
        else if (dsType == 3)
          ds =
            CreateExplicitFromStructuredDataSet(b, useDims, DataSetOption::CURVILINEAR, addGhost);
        else if (dsType == 4)
          ds = CreateExplicitFromStructuredDataSet(b, useDims, DataSetOption::EXPLICIT, addGhost);

        ds.AddPointField(fieldName, CreateConstantVectorField(ds.GetNumberOfPoints(), vecX));
        pds.AppendPartition(ds);
      }

      vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
      seedArray = vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                                 vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1),
                                                 vtkm::Particle(vtkm::Vec3f(4.2f, 1.0f, .2f), 2),
                                                 vtkm::Particle(vtkm::Vec3f(8.2f, 1.0f, .2f), 3) });

      vtkm::Id numSeeds = seedArray.GetNumberOfValues();

      vtkm::filter::ParticleAdvection particleAdvection;

      particleAdvection.SetStepSize(0.1f);
      particleAdvection.SetNumberOfSteps(1000);
      particleAdvection.SetSeeds(seedArray);

      particleAdvection.SetActiveField(fieldName);
      auto out = particleAdvection.Execute(pds);

      VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
      auto ds = out.GetPartition(0);

      //Validate the result is correct.
      VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");

      auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();

      VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");
      auto ptPortal = coords.ReadPortal();
      vtkm::Id nPts = ptPortal.GetNumberOfValues();
      for (vtkm::Id j = 0; j < nPts; j++)
      {
        VTKM_TEST_ASSERT(!globalBounds.Contains(ptPortal.Get(j)), "End point not oustide bounds");
        if (addGhost)
          VTKM_TEST_ASSERT(boundsWithGhosts.Contains(ptPortal.Get(j)),
                           "End point not inside bounds with ghosts");
      }

      vtkm::cont::DynamicCellSet dcells = ds.GetCellSet();
      VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds, "Wrong number of cells");
    }
  }
}

void TestDataSet(const vtkm::cont::DataSet& dataset,
                 const std::vector<vtkm::Vec3f>& pts,
                 vtkm::FloatDefault stepSize,
                 vtkm::Id maxSteps,
                 const std::vector<vtkm::Vec3f>& endPts)
{
  vtkm::Id numPoints = static_cast<vtkm::Id>(pts.size());

  std::vector<vtkm::Particle> seeds;
  for (vtkm::Id i = 0; i < numPoints; i++)
    seeds.push_back(vtkm::Particle(pts[static_cast<std::size_t>(i)], i));
  auto seedArray = vtkm::cont::make_ArrayHandle(seeds, vtkm::CopyFlag::On);

  vtkm::filter::ParticleAdvection particleAdvection;
  particleAdvection.SetStepSize(stepSize);
  particleAdvection.SetNumberOfSteps(maxSteps);
  particleAdvection.SetSeeds(seedArray);

  particleAdvection.SetActiveField("vec");
  auto output = particleAdvection.Execute(dataset);

  auto coords = output.GetCoordinateSystem().GetDataAsMultiplexer();
  vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numPoints, "Wrong number of cells");
  VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetSingleType<>>(), "Wrong cell type");
  vtkm::cont::ArrayHandle<vtkm::Vec3f> coordPts;
  auto cPortal = coords.ReadPortal();

  const vtkm::FloatDefault eps = static_cast<vtkm::FloatDefault>(1e-3);
  for (vtkm::Id i = 0; i < numPoints; i++)
  {
    vtkm::Vec3f e = endPts[static_cast<std::size_t>(i)];
    vtkm::Vec3f pt = cPortal.Get(i);
    VTKM_TEST_ASSERT(vtkm::Magnitude(pt - e) <= eps, "Particle advection point is wrong");
  }
}

void TestFile(const std::string& fname,
              const std::vector<vtkm::Vec3f>& pts,
              vtkm::FloatDefault stepSize,
              vtkm::Id maxSteps,
              const std::vector<vtkm::Vec3f>& endPts)
{
  std::cout << fname << std::endl;

  vtkm::io::VTKDataSetReader reader(fname);
  vtkm::cont::DataSet dataset;
  try
  {
    dataset = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  TestDataSet(dataset, pts, stepSize, maxSteps, endPts);

  std::cout << "  as explicit grid" << std::endl;
  vtkm::filter::CleanGrid clean;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(false);
  clean.SetRemoveDegenerateCells(false);
  vtkm::cont::DataSet explicitData = clean.Execute(dataset);

  TestDataSet(explicitData, pts, stepSize, maxSteps, endPts);
}

void TestParticleAdvectionFilter()
{
  TestBasic();
  TestPartitionedDataSet();

  //Fusion test.
  std::vector<vtkm::Vec3f> fusionPts, fusionEndPts;
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.6f, 0.6f));
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.8f, 0.6f));
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.8f, 0.3f));
  //End point values were generated in VisIt.
  fusionEndPts.push_back(vtkm::Vec3f(0.5335789918f, 0.87112802267f, 0.6723330020f));
  fusionEndPts.push_back(vtkm::Vec3f(0.5601879954f, 0.91389900446f, 0.43989110522f));
  fusionEndPts.push_back(vtkm::Vec3f(0.7004770041f, 0.63193398714f, 0.64524400234f));
  vtkm::FloatDefault fusionStep = 0.005f;
  std::string fusionFile = vtkm::cont::testing::Testing::DataPath("rectilinear/fusion.vtk");

  TestFile(fusionFile, fusionPts, fusionStep, 1000, fusionEndPts);

  //Fishtank test.
  std::vector<vtkm::Vec3f> fishPts, fishEndPts;
  fishPts.push_back(vtkm::Vec3f(0.75f, 0.5f, 0.01f));
  fishPts.push_back(vtkm::Vec3f(0.4f, 0.2f, 0.7f));
  fishPts.push_back(vtkm::Vec3f(0.5f, 0.3f, 0.8f));
  //End point values were generated in VisIt.
  fishEndPts.push_back(vtkm::Vec3f(0.7734669447f, 0.4870159328f, 0.8979591727f));
  fishEndPts.push_back(vtkm::Vec3f(0.7257543206f, 0.1277695596f, 0.7468645573f));
  fishEndPts.push_back(vtkm::Vec3f(0.8347796798f, 0.1276152730f, 0.4985143244f));
  vtkm::FloatDefault fishStep = 0.001f;
  std::string fishFile = vtkm::cont::testing::Testing::DataPath("rectilinear/fishtank.vtk");

  TestFile(fishFile, fishPts, fishStep, 100, fishEndPts);
}
}

int UnitTestParticleAdvectionFilter(int argc, char* argv[])
{
  // Setup MPI environment: This test is not intendent to be run in parallel
  // but filter does make MPI calls
  vtkmdiy::mpi::environment env(argc, argv);
  return vtkm::cont::testing::Testing::Run(TestParticleAdvectionFilter, argc, argv);
}
