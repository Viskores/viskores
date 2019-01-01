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

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/GhostZone.h>

namespace
{

static vtkm::cont::ArrayHandle<vtkm::UInt8> StructuredGhostZoneArray(vtkm::Id nx,
                                                                     vtkm::Id ny,
                                                                     vtkm::Id nz,
                                                                     int numLayers)
{
  vtkm::Id numCells = nx * ny;
  if (nz > 0)
    numCells *= nz;

  vtkm::UInt8 normalCell = static_cast<vtkm::UInt8>(vtkm::CellClassification::NORMAL);
  vtkm::UInt8 duplicateCell = static_cast<vtkm::UInt8>(vtkm::CellClassification::DUPLICATE);

  vtkm::cont::ArrayHandle<vtkm::UInt8> ghosts;
  ghosts.Allocate(numCells);
  auto portal = ghosts.GetPortalControl();
  for (vtkm::Id i = 0; i < numCells; i++)
  {
    if (numLayers == 0)
      portal.Set(i, normalCell);
    else
      portal.Set(i, duplicateCell);
  }
  if (numLayers > 0)
  {
    //2D case
    if (nz == 0)
    {
      //std::cout<<"dims: "<<nx<<" "<<ny<<": "<<nx-numLayers<<" "<<ny-numLayers<<std::endl;
      for (vtkm::Id i = numLayers; i < nx - numLayers; i++)
        for (vtkm::Id j = numLayers; j < ny - numLayers; j++)
          portal.Set(j * nx + i, normalCell);
    }
    else
    {
      for (vtkm::Id i = numLayers; i < nx - numLayers; i++)
        for (vtkm::Id j = numLayers; j < ny - numLayers; j++)
          for (vtkm::Id k = numLayers; k < nz - numLayers; k++)
            portal.Set(k * nx * ny + j * nx + i, normalCell);
    }
  }

  return ghosts;
}

static vtkm::cont::DataSet MakeUniform(vtkm::Id numI, vtkm::Id numJ, vtkm::Id numK, int numLayers)
{
  vtkm::cont::DataSetBuilderUniform dsb;

  vtkm::cont::DataSet ds;

  if (numK == 0)
    ds = dsb.Create(vtkm::Id2(numI + 1, numJ + 1));
  else
    ds = dsb.Create(vtkm::Id3(numI + 1, numJ + 1, numK + 1));

  auto ghosts = StructuredGhostZoneArray(numI, numJ, numK, numLayers);

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddCellField(ds, "vtkmGhostCells", ghosts);

  return ds;
}

static vtkm::cont::DataSet MakeRectilinear(vtkm::Id numI,
                                           vtkm::Id numJ,
                                           vtkm::Id numK,
                                           int numLayers)
{
  vtkm::cont::DataSetBuilderRectilinear dsb;
  vtkm::cont::DataSet ds;
  std::size_t nx(static_cast<std::size_t>(numI + 1));
  std::size_t ny(static_cast<std::size_t>(numJ + 1));

  std::vector<float> x(nx), y(ny);
  for (std::size_t i = 0; i < nx; i++)
    x[i] = static_cast<float>(i);
  for (std::size_t i = 0; i < ny; i++)
    y[i] = static_cast<float>(i);

  if (numK == 0)
    ds = dsb.Create(x, y);
  else
  {
    std::size_t nz(static_cast<std::size_t>(numK + 1));
    std::vector<float> z(nz);
    for (std::size_t i = 0; i < nz; i++)
      z[i] = static_cast<float>(i);
    ds = dsb.Create(x, y, z);
  }

  auto ghosts = StructuredGhostZoneArray(numI, numJ, numK, numLayers);

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddCellField(ds, "vtkmGhostCells", ghosts);

  return ds;
}

template <class CellSetType, vtkm::IdComponent NDIM>
static void MakeExplicitCells(const CellSetType& cellSet,
                              vtkm::Vec<vtkm::Id, NDIM>& dims,
                              vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
                              vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                              vtkm::cont::ArrayHandle<vtkm::Id>& conn)
{
  using Connectivity = vtkm::internal::ConnectivityStructuredInternals<NDIM>;

  vtkm::Id nCells = cellSet.GetNumberOfCells();
  vtkm::Id connLen = (NDIM == 2 ? nCells * 4 : nCells * 8);

  conn.Allocate(connLen);
  shapes.Allocate(nCells);
  numIndices.Allocate(nCells);

  Connectivity structured;
  structured.SetPointDimensions(dims);

  vtkm::Id idx = 0;
  for (vtkm::Id i = 0; i < nCells; i++)
  {
    auto ptIds = structured.GetPointsOfCell(i);
    for (vtkm::IdComponent j = 0; j < NDIM; j++, idx++)
      conn.GetPortalControl().Set(idx, ptIds[j]);

    shapes.GetPortalControl().Set(
      i, (NDIM == 4 ? vtkm::CELL_SHAPE_QUAD : vtkm::CELL_SHAPE_HEXAHEDRON));
    numIndices.GetPortalControl().Set(i, NDIM);
  }
}

static vtkm::cont::DataSet MakeExplicit(vtkm::Id numI, vtkm::Id numJ, vtkm::Id numK, int numLayers)
{
  using CoordType = vtkm::Vec<vtkm::Float32, 3>;

  vtkm::cont::DataSet dsUniform = MakeUniform(numI, numJ, numK, numLayers);

  auto coordData = dsUniform.GetCoordinateSystem(0).GetData();
  vtkm::Id numPts = coordData.GetNumberOfValues();
  vtkm::cont::ArrayHandle<CoordType> explCoords;

  explCoords.Allocate(numPts);
  auto explPortal = explCoords.GetPortalControl();
  auto cp = coordData.GetPortalConstControl();
  for (vtkm::Id i = 0; i < numPts; i++)
    explPortal.Set(i, cp.Get(i));

  vtkm::cont::DynamicCellSet cellSet = dsUniform.GetCellSet(0);
  vtkm::cont::ArrayHandle<vtkm::Id> conn;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  vtkm::cont::DataSet ds;
  vtkm::cont::DataSetBuilderExplicit dsb;

  if (cellSet.IsType<vtkm::cont::CellSetStructured<2>>())
  {
    vtkm::Vec<vtkm::Id, 2> dims(numI, numJ);
    MakeExplicitCells(
      cellSet.Cast<vtkm::cont::CellSetStructured<2>>(), dims, numIndices, shapes, conn);
    ds = dsb.Create(explCoords, vtkm::CellShapeTagQuad(), 4, conn, "coordinates", "cells");
  }
  else if (cellSet.IsType<vtkm::cont::CellSetStructured<3>>())
  {
    vtkm::Vec<vtkm::Id, 3> dims(numI, numJ, numK);
    MakeExplicitCells(
      cellSet.Cast<vtkm::cont::CellSetStructured<3>>(), dims, numIndices, shapes, conn);
    ds = dsb.Create(explCoords, vtkm::CellShapeTagHexahedron(), 8, conn, "coordinates", "cells");
  }

  auto ghosts = StructuredGhostZoneArray(numI, numJ, numK, numLayers);

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddCellField(ds, "vtkmGhostCells", ghosts);

  return ds;
}

void TestStructured()
{
  std::cout << "Testing ghost cells for uniform datasets." << std::endl;

  // specify some 2d tests: {numI, numJ, numK, numGhostLayers}.
  std::vector<std::vector<vtkm::Id>> tests2D = { { 4, 4, 0, 2 },  { 5, 5, 0, 3 },  { 10, 10, 0, 3 },
                                                 { 10, 5, 0, 2 }, { 5, 10, 0, 2 }, { 20, 10, 0, 3 },
                                                 { 10, 20, 0, 3 } };
  std::vector<std::vector<vtkm::Id>> tests3D = { { 4, 4, 4, 2 },    { 5, 5, 5, 3 },
                                                 { 10, 10, 10, 3 }, { 10, 5, 10, 2 },
                                                 { 5, 10, 10, 2 },  { 20, 10, 10, 3 },
                                                 { 10, 20, 10, 3 } };

  std::vector<std::vector<vtkm::Id>> tests;

  tests.insert(tests.end(), tests2D.begin(), tests2D.end());
  tests.insert(tests.end(), tests3D.begin(), tests3D.end());

  for (auto& t : tests)
  {
    vtkm::Id nx = t[0], ny = t[1], nz = t[2];
    int nghost = static_cast<int>(t[3]);
    for (int layer = 0; layer < nghost; layer++)
    {
      vtkm::cont::DataSet ds;
      std::vector<std::string> dsTypes = { "uniform", "rectilinear", "explicit" };
      for (auto& dsType : dsTypes)
      {
        if (dsType == "uniform")
          ds = MakeUniform(nx, ny, nz, layer);
        else if (dsType == "rectilinear")
          ds = MakeRectilinear(nx, ny, nz, layer);
        else if (dsType == "explicit")
          ds = MakeExplicit(nx, ny, nz, layer);

        std::vector<std::string> removeType = { "all", "byType" };
        for (auto& rt : removeType)
        {
          vtkm::filter::GhostZone ghostZoneRemoval;
          if (rt == "all")
            ghostZoneRemoval.RemoveAllGhost();
          else if (rt == "byType")
            ghostZoneRemoval.RemoveByType(
              static_cast<vtkm::UInt8>(vtkm::CellClassification::DUPLICATE));

          std::vector<std::string> outputType = { "permutation", "explicit" };
          for (auto& ot : outputType)
          {
            if (ot == "explicit")
              ghostZoneRemoval.ConvertOutputToUnstructured();

            auto output = ghostZoneRemoval.Execute(ds, vtkm::filter::GhostZonePolicy());
            vtkm::Id numCells = output.GetCellSet(0).GetNumberOfCells();

            //Validate the output.
            VTKM_TEST_ASSERT(output.GetNumberOfCellSets() == 1,
                             "Wrong number of cell sets in output");
            vtkm::Id numCellsReq = (nx - 2 * layer) * (ny - 2 * layer);
            if (nz != 0)
              numCellsReq *= (nz - 2 * layer);

            VTKM_TEST_ASSERT(numCellsReq == numCells, "Wrong number of cells in output");
            if (ot == "explicit")
              VTKM_TEST_ASSERT(output.GetCellSet(0).IsType<vtkm::cont::CellSetExplicit<>>(),
                               "Wrong cell type for explicit conversion");
          }
        }
      }
    }
  }
}

void TestExplicit()
{
}

void TestGhostZone()
{
  TestStructured();
  TestExplicit();
}
}

int UnitTestGhostZone(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestGhostZone, argc, argv);
}
