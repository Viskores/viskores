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

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/GhostZone.h>

using vtkm::cont::testing::MakeTestDataSet;

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
      for (vtkm::Id i = numLayers; i < nx - numLayers; i++)
        for (vtkm::Id j = numLayers; j < ny - numLayers; j++)
          portal.Set(i * nx + j, normalCell);
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

  vtkm::cont::ArrayHandle<vtkm::UInt8> ghosts;
  ghosts = StructuredGhostZoneArray(numI, numJ, numK, numLayers);

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddCellField(ds, "vtkmGhostCells", ghosts);

  return ds;
}

void TestUniform()
{
  std::cout << "Testing ghost cells for uniform datasets." << std::endl;

  // specify some 2d tests: {numI, numJ, numGhostLayers}.
  std::vector<std::vector<vtkm::Id>> tests2D = { { 4, 4, 0, 2 }, { 5, 5, 0, 3 }, { 10, 10, 0, 3 } };
  for (auto& t : tests2D)
  {
    vtkm::Id nx = t[0], ny = t[1], nz = t[2];
    int nghost = static_cast<int>(t[3]);
    for (int layer = 0; layer < nghost; layer++)
    {
      vtkm::cont::DataSet ds = MakeUniform(nx, ny, nz, layer);

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
            numCellsReq += (nz - 2 * layer);

          VTKM_TEST_ASSERT(numCellsReq == numCells, "Wrong number of cells in output");
          if (ot == "explicit")
            VTKM_TEST_ASSERT(output.GetCellSet(0).IsType<vtkm::cont::CellSetExplicit<>>(),
                             "Wrong cell type for explicit conversion");
        }
      }
    }
  }
}

void TestGhostZone()
{
  TestUniform();
}
}

int UnitTestGhostZone(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestGhostZone);
}
