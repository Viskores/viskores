//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/Particle.h>
#include <viskores/cont/AssignerPartitionedDataSet.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/flow/ParticleAdvection.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <mpi.h>
#include <viskores/thirdparty/diy/diy.h>
#include <viskores/thirdparty/diy/mpi-cast.h>

void LoadData(std::string& fname,
              std::vector<viskores::cont::DataSet>& dataSets,
              int rank,
              int nRanks)
{
  std::string buff;
  std::ifstream is;
  is.open(fname);
  std::cout << "Opening: " << fname << std::endl;
  if (!is)
  {
    std::cout << "File not found! : " << fname << std::endl;
    throw "unknown file: " + fname;
  }

  auto p0 = fname.rfind(".visit");
  if (p0 == std::string::npos)
    throw "Only .visit files are supported.";
  auto tmp = fname.substr(0, p0);
  auto p1 = tmp.rfind("/");
  auto dir = tmp.substr(0, p1);

  std::getline(is, buff);
  auto numBlocks = std::stoi(buff.substr(buff.find("!NBLOCKS ") + 9, buff.size()));
  if (rank == 0)
    std::cout << "numBlocks= " << numBlocks << std::endl;

  int nPer = numBlocks / nRanks;
  int b0 = rank * nPer, b1 = (rank + 1) * nPer;
  if (rank == (nRanks - 1))
    b1 = numBlocks;

  for (int i = 0; i < numBlocks; i++)
  {
    std::getline(is, buff);
    if (i >= b0 && i < b1)
    {
      viskores::cont::DataSet ds;
      std::string vtkFile = dir + "/" + buff;
      viskores::io::VTKDataSetReader reader(vtkFile);
      ds = reader.ReadDataSet();
      auto f = ds.GetField("grad").GetData();
      viskores::cont::ArrayHandle<viskores::Vec<double, 3>> fieldArray;
      f.AsArrayHandle(fieldArray);
      int n = fieldArray.GetNumberOfValues();
      auto portal = fieldArray.WritePortal();
      for (int ii = 0; ii < n; ii++)
        portal.Set(ii, viskores::Vec<double, 3>(1, 0, 0));

      dataSets.push_back(ds);
    }
  }
}

// Example computing streamlines.
// An example vector field is available in the viskores data directory: rotate-vectors.vtk
// Example usage:
//   this will advect 200 particles 50 steps using a step size of 0.05
//
// Particle_Advection <path-to-data-dir>/rotate-vectors.vtk vec 200 50 0.05 output.vtk
//

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();

  std::string dataFile = argv[1];
  std::vector<viskores::cont::DataSet> dataSets;
  LoadData(dataFile, dataSets, rank, size);

  viskores::filter::flow::ParticleAdvection pa;

  viskores::cont::ArrayHandle<viskores::Particle> seedArray;
  seedArray =
    viskores::cont::make_ArrayHandle({ viskores::Particle(viskores::Vec3f(.1f, .1f, .9f), 0),
                                       viskores::Particle(viskores::Vec3f(.1f, .6f, .6f), 1),
                                       viskores::Particle(viskores::Vec3f(.1f, .9f, .1f), 2) });
  pa.SetStepSize(0.001f);
  pa.SetNumberOfSteps(10000);
  pa.SetSeeds(seedArray);
  pa.SetActiveField("grad");

  viskores::cont::PartitionedDataSet pds(dataSets);
  auto output = pa.Execute(pds);
  output.PrintSummary(std::cout);

  return 0;
}
