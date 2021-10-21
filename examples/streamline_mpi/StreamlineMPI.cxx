//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/filter/Streamline.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>
#include <vtkm/io/reader/VTKDataSetReader.h>

#include <mpi.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/thirdparty/diy/mpi-cast.h>


#include <vtkm/filter/ParticleAdvection.h>
#include <vtkm/filter/particleadvection/BoundsMap.h>
#include <vtkm/filter/particleadvection/ParticleMessenger.h>


void LoadData(std::string& fname, std::vector<vtkm::cont::DataSet>& dataSets, int rank, int nRanks)
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
      vtkm::cont::DataSet ds;
      std::string vtkFile = dir + "/" + buff;
      vtkm::io::VTKDataSetReader reader(vtkFile);
      ds = reader.ReadDataSet();
      auto f = ds.GetField("grad").GetData();
      vtkm::cont::ArrayHandle<vtkm::Vec<double, 3>> fieldArray;
      f.AsArrayHandle(fieldArray);
      int n = fieldArray.GetNumberOfValues();
      auto portal = fieldArray.WritePortal();
      for (int ii = 0; ii < n; ii++)
        portal.Set(ii, vtkm::Vec<double, 3>(1, 0, 0));

      dataSets.push_back(ds);
    }
  }
}

// Example computing streamlines.
// An example vector field is available in the vtk-m data directory: magField.vtk
// Example usage:
//   this will advect 200 particles 50 steps using a step size of 0.01
//
// Particle_Advection <path-to-data-dir>/magField.vtk vec 200 50 0.01 output.vtk
//

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  int rank = comm.rank();
  int size = comm.size();

  std::string dataFile = argv[1];
  std::vector<vtkm::cont::DataSet> dataSets;
  LoadData(dataFile, dataSets, rank, size);

  vtkm::filter::ParticleAdvection pa;

  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  seedArray = vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.1f, .1f, .9f), 0),
                                             vtkm::Particle(vtkm::Vec3f(.1f, .6f, .6f), 1),
                                             vtkm::Particle(vtkm::Vec3f(.1f, .9f, .1f), 2) });
  pa.SetStepSize(0.001f);
  pa.SetNumberOfSteps(10000);
  pa.SetSeeds(seedArray);
  pa.SetActiveField("grad");

  vtkm::cont::PartitionedDataSet pds(dataSets);
  auto output = pa.Execute(pds);
  output.PrintSummary(std::cout);

  return 0;
}
