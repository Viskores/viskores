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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/filter/ContourTreeUniformAugmented.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/ProcessContourTree.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/Branch.h>

#ifdef ENABLE_SET_NUM_THREADS
#include "tbb/task_scheduler_init.h"
#endif

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#ifdef WITH_MPI
#include <mpi.h>
#endif

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

#define DEBUG_TIMING

using ValueType = vtkm::Float32;
using BranchType = vtkm::worklet::contourtree_augmented::process_contourtree_inc::Branch<ValueType>;

namespace cppp2_ns = vtkm::worklet::contourtree_augmented;

// Simple helper class for parsing the command line options
class ParseCL
{
public:
  ParseCL() {}

  void parse(int& argc, char** argv)
  {
    mCLOptions.resize(static_cast<std::size_t>(argc));
    for (std::size_t i = 1; i < static_cast<std::size_t>(argc); ++i)
    {
      this->mCLOptions[i] = std::string(argv[i]);
    }
  }

  vtkm::Id findOption(const std::string& option) const
  {
    auto it =
      std::find_if(this->mCLOptions.begin(),
                   this->mCLOptions.end(),
                   [option](const std::string& val) -> bool { return val.find(option) == 0; });
    if (it == this->mCLOptions.end())
    {
      return -1;
    }
    else
    {
      return (it - this->mCLOptions.begin());
    }
  }

  bool hasOption(const std::string& option) const { return this->findOption(option) >= 0; }

  std::string getOption(const std::string& option) const
  {
    std::size_t index = static_cast<std::size_t>(this->findOption(option));
    std::string val = this->mCLOptions[index];
    auto valPos = val.find("=");
    if (valPos)
    {
      return val.substr(valPos + 1);
    }
    return std::string("");
  }

  const std::vector<std::string>& getOptions() const { return this->mCLOptions; }

private:
  std::vector<std::string> mCLOptions;
};



// Compute and render an isosurface for a uniform grid example
int main(int argc, char* argv[])
{
#ifdef WITH_MPI
  // Setup the MPI environment.
  MPI_Init(&argc, &argv);
  auto comm = MPI_COMM_WORLD;

  // Tell VTK-m which communicator it should use.
  vtkm::cont::EnvironmentTracker::SetCommunicator(vtkmdiy::mpi::communicator(comm));

  // get the rank and size
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  int numBlocks = size;
  int blocksPerRank = 1;
  if (rank == 0)
  {
    std::cout << "Running with MPI. #ranks=" << size << std::endl;
  }
#else
  std::cout << "Single node run" << std::endl;
  int rank = 0;
#endif

  vtkm::Float64 prevTime = 0;
  vtkm::Float64 currTime = 0;
  vtkm::cont::Timer totalTime;

  totalTime.Start();
  if (rank == 0)
  {
    std::cout << "ContourTreePPP2Mesh <options> <fileName>" << std::endl;
  }

  ////////////////////////////////////////////
  // Parse the command line options
  ////////////////////////////////////////////
  ParseCL parser;
  parser.parse(argc, argv);
  std::string filename = parser.getOptions().back();
  unsigned int computeRegularStructure = 1; // 1=fully augmented
  bool useMarchingCubes = false;
  bool computeBranchDecomposition = true;
  bool printContourTree = false;
  if (parser.hasOption("--augmentTree"))
    computeRegularStructure =
      static_cast<unsigned int>(std::stoi(parser.getOption("--augmentTree")));
  if (parser.hasOption("--mc"))
    useMarchingCubes = true;
  if (parser.hasOption("--printCT"))
    printContourTree = true;
  if (parser.hasOption("--branchDecomp"))
    computeBranchDecomposition = std::stoi(parser.getOption("--branchDecomp"));
  // We need the fully augmented tree to compute the branch decomposition
  if (computeBranchDecomposition && (computeRegularStructure != 1))
  {
    std::cout << "Regular structure is required for branch decomposition."
                 " Disabling branch decomposition"
              << std::endl;
    computeBranchDecomposition = false;
  }

  std::string device("default");
  if (parser.hasOption("--device"))
  {
    device = parser.getOption("--device");
    auto& rtTracker = vtkm::cont::GetRuntimeDeviceTracker();
    if (device == "serial" && rtTracker.CanRunOn(vtkm::cont::DeviceAdapterTagSerial()))
    {
      rtTracker.ForceDevice(vtkm::cont::DeviceAdapterTagSerial());
    }
    else if (device == "openmp" && rtTracker.CanRunOn(vtkm::cont::DeviceAdapterTagOpenMP()))
    {
      rtTracker.ForceDevice(vtkm::cont::DeviceAdapterTagOpenMP());
    }
    else if (device == "tbb" && rtTracker.CanRunOn(vtkm::cont::DeviceAdapterTagTBB()))
    {
      rtTracker.ForceDevice(vtkm::cont::DeviceAdapterTagTBB());
    }
    else if (device == "cuda" && rtTracker.CanRunOn(vtkm::cont::DeviceAdapterTagCuda()))
    {
      rtTracker.ForceDevice(vtkm::cont::DeviceAdapterTagCuda());
    }
    else
    {
      std::cout << "Invalid or unavialable device adapter: " << device << std::endl;
      return EXIT_FAILURE;
    }
  }

#ifdef ENABLE_SET_NUM_THREADS
  int numThreads = tbb::task_scheduler_init::default_num_threads();
  if (parser.hasOption("--numThreads"))
  {
    if (device == "default" &&
        vtkm::cont::GetRuntimeDeviceTracker().CanRunOn(vtkm::cont::DeviceAdapterTagTBB()))
    {
      std::cout << "--numThreads specified without device. Forcing device as tbb.";
      device = "tbb";
      vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(vtkm::cont::DeviceAdapterTagTBB());
    }

    numThreads = std::stoi(parser.getOption("--numThreads"));
    if (device != "tbb")
    {
      std::cout << "numThreads will be ignored for devices other than tbb";
    }
  }
  tbb::task_scheduler_init schedulerInit(numThreads);
#endif

  // Iso value selection parameters
  // Approach to be used to select contours based on the tree
  vtkm::Id contourType = 0;
  // Error away from critical point
  ValueType eps = 0.00001f;
  // Number of iso levels to be selected. By default we disable the isovalue selection.
  vtkm::Id numLevels = 0;
  // Number of components the tree should be simplified to
  vtkm::Id numComp = numLevels + 1;
  // Method to be used to compute the relevant iso values
  vtkm::Id contourSelectMethod = 0;
  bool usePersistenceSorter = true;
  if (parser.hasOption("--levels"))
    numLevels = std::stoi(parser.getOption("--levels"));
  if (parser.hasOption("--type"))
    contourType = std::stoi(parser.getOption("--type"));
  if (parser.hasOption("--eps"))
    eps = std::stof(parser.getOption("--eps"));
  if (parser.hasOption("--method"))
    contourSelectMethod = std::stoi(parser.getOption("--method"));
  if (parser.hasOption("--comp"))
    numComp = std::stoi(parser.getOption("--comp"));
  if (contourSelectMethod == 0)
    numComp = numLevels + 1;
  if (parser.hasOption("--useVolumeSorter"))
    usePersistenceSorter = false;
  if ((numLevels > 0) && (!computeBranchDecomposition))
  {
    std::cout << "Iso level selection only available when branch decomposition is enabled."
                 " Disabling iso value selection"
              << std::endl;
    numLevels = 0;
  }

  if (rank == 0 && (argc < 2 || parser.hasOption("--help") || parser.hasOption("-h")))
  {
    std::cout << "Parameter is <fileName>" << std::endl;
    std::cout << "File is expected to be ASCII with either: " << std::endl;
    std::cout << "  - xdim ydim integers for 2D or" << std::endl;
    std::cout << "  - xdim ydim zdim integers for 3D" << std::endl;
    std::cout << "followed by vector data last dimension varying fastest" << std::endl;

    std::cout << std::endl;
    std::cout << "Options: (Bool options are give via int, i.e. =0 for False and =1 for True)"
              << std::endl;
    std::cout << "--mc              Use marching cubes interpolation for contour tree calculation. "
                 "(Default=False)"
              << std::endl;
    std::cout << "--augmentTree     1 = compute the fully augmented contour tree (Default)"
              << std::endl;
    std::cout << "                  2 = compute the boundary augmented contour tree " << std::endl;
    std::cout << "                  0 = no augmentation. NOTE: When using MPI, local ranks use"
              << std::endl;
    std::cout << "                      boundary augmentation to support parallel merge of blocks"
              << std::endl;
    std::cout << "--branchDecomp    Compute the volume branch decomposition for the contour tree. "
                 "Requires --augmentTree (Default=True)"
              << std::endl;
    std::cout << "--printCT         Print the contour tree. (Default=False)" << std::endl;
    std::cout << "--device          Set the device to use (serial, openmp, tbb, cuda). "
                 "Use the default device if unspecified"
              << std::endl;
#ifdef ENABLE_SET_NUM_THREADS
    std::cout << "--numThreads      Specifiy the number of threads to use. Available only with TBB."
              << std::endl;
#endif
    std::cout << std::endl;
    std::cout << "Isovalue selection options: (require --branchDecomp=1 and augmentTree=1)"
              << std::endl;
    std::cout << "--levels=<int>  Number of iso-contour levels to be used (default=0, i.e., "
                 "disable isovalue computation)"
              << std::endl;
    std::cout << "--comp=<int>    Number of components the contour tree should be simplified to. "
                 "Only used if method==1. (default=0)"
              << std::endl;
    std::cout
      << "--eps=<float>   Floating point offset awary from the critical point. (default=0.00001)"
      << std::endl;
    std::cout << "--type=<int>    Approach to be used for selection of iso-values. 0=saddle+-eps; "
                 "1=mid point between saddle and extremum, 2=extremum+-eps. (default=0)"
              << std::endl;
    std::cout << "--method=<int>  Method used for selecting relevant iso-values. (default=0)"
              << std::endl;
#ifdef WITH_MPI
    MPI_Finalize();
#endif
    return EXIT_SUCCESS;
  }

  if (rank == 0)
  {
    std::cout << "Settings:" << std::endl;
    std::cout << "    filename=" << filename << std::endl;
    std::cout << "    device=" << device << std::endl;
    std::cout << "    mc=" << useMarchingCubes << std::endl;
    std::cout << "    augmentTree=" << computeRegularStructure << std::endl;
    std::cout << "    branchDecomp=" << computeBranchDecomposition << std::endl;
#ifdef WITH_MPI
    std::cout << "    nblocks=" << numBlocks << std::endl;
#endif
#ifdef ENABLE_SET_NUM_THREADS
    std::cout << "    numThreads=" << numThreads << std::endl;
#endif
    std::cout << "    computeIsovalues=" << (numLevels > 0) << std::endl;
    if (numLevels > 0)
    {
      std::cout << "    levels=" << numLevels << std::endl;
      std::cout << "    eps=" << eps << std::endl;
      std::cout << "    comp" << numComp << std::endl;
      std::cout << "    type=" << contourType << std::endl;
      std::cout << "    method=" << contourSelectMethod << std::endl;
      std::cout << "    mc=" << useMarchingCubes << std::endl;
      std::cout << "    use" << (usePersistenceSorter ? "PersistenceSorter" : "VolumeSorter")
                << std::endl;
    }
    std::cout << std::endl;
  }
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 startUpTime = currTime - prevTime;
  prevTime = currTime;

// Redirect stdout to file if we are using MPI with Debugging
#ifdef WITH_MPI
#ifdef DEBUG_PRINT
  // From https://www.unix.com/302983597-post2.html
  char* cstr_filename = new char[15];
  snprintf(cstr_filename, sizeof(filename), "cout_%d.log", rank);
  int out = open(cstr_filename, O_RDWR | O_CREAT | O_APPEND, 0600);
  if (-1 == out)
  {
    perror("opening cout.log");
    return 255;
  }

  snprintf(cstr_filename, sizeof(cstr_filename), "cerr_%d.log", rank);
  int err = open(cstr_filename, O_RDWR | O_CREAT | O_APPEND, 0600);
  if (-1 == err)
  {
    perror("opening cerr.log");
    return 255;
  }

  int save_out = dup(fileno(stdout));
  int save_err = dup(fileno(stderr));

  if (-1 == dup2(out, fileno(stdout)))
  {
    perror("cannot redirect stdout");
    return 255;
  }
  if (-1 == dup2(err, fileno(stderr)))
  {
    perror("cannot redirect stderr");
    return 255;
  }

  delete[] cstr_filename;
#endif
#endif

  ///////////////////////////////////////////////
  // Read the input data
  ///////////////////////////////////////////////
  std::ifstream inFile(filename);
  if (inFile.bad())
    return 0;

  // Read the dimensions of the mesh, i.e,. number of elementes in x, y, and z
  std::vector<std::size_t> dims;
  std::string line;
  getline(inFile, line);
  std::istringstream linestream(line);
  std::size_t dimVertices;
  while (linestream >> dimVertices)
  {
    dims.push_back(dimVertices);
  }

  // Compute the number of vertices, i.e., xdim * ydim * zdim
  unsigned short nDims = static_cast<unsigned short>(dims.size());
  std::size_t nVertices = static_cast<std::size_t>(
    std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<std::size_t>()));

  // Print the mesh metadata
  if (rank == 0)
  {
    std::cout << "Number of dimensions: " << nDims << std::endl;
    std::cout << "Number of mesh vertices: " << nVertices << std::endl;
  }
  // Check the the number of dimensiosn is either 2D or 3D
  bool invalidNumDimensions = (nDims < 2 || nDims > 3);
  bool invalidMCOption = (useMarchingCubes && nDims != 3);
  if (rank == 0)
  {
    if (invalidNumDimensions)
    {
      std::cout << "The input mesh is " << nDims << "D. Input data must be either 2D or 3D."
                << std::endl;
    }
    if (invalidMCOption)
    {
      std::cout << "The input mesh is " << nDims
                << "D. Contour tree using marching cubes only supported for 3D data." << std::endl;
    }
  }
  if (invalidNumDimensions || invalidMCOption)
  {
#ifdef WITH_MPI
    MPI_Finalize();
#endif
    return EXIT_SUCCESS;
  }

  // Read data
  std::vector<ValueType> values(nVertices);
  for (std::size_t vertex = 0; vertex < nVertices; ++vertex)
  {
    inFile >> values[vertex];
  }

  // finish reading the data
  inFile.close();

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 dataReadTime = currTime - prevTime;
  prevTime = currTime;

  vtkm::cont::DataSetBuilderUniform dsb;
#ifndef WITH_MPI                 // construct regular, single-block VTK-M input dataset
  vtkm::cont::DataSet inDataSet; // Single block dataset
  {
    // build the input dataset
    // 2D data
    if (nDims == 2)
    {
      vtkm::Id2 vdims;
      vdims[0] = static_cast<vtkm::Id>(dims[1]);
      vdims[1] = static_cast<vtkm::Id>(dims[0]);
      inDataSet = dsb.Create(vdims);
    }
    // 3D data
    else
    {
      vtkm::Id3 vdims;
      vdims[0] = static_cast<vtkm::Id>(dims[1]);
      vdims[1] = static_cast<vtkm::Id>(dims[0]);
      vdims[2] = static_cast<vtkm::Id>(dims[2]);
      inDataSet = dsb.Create(vdims);
    }
    vtkm::cont::DataSetFieldAdd dsf;
    dsf.AddPointField(inDataSet, "values", values);
  }
#else  // Create a multi-block dataset for multi-block DIY-paralle processing
  vtkm::cont::PartitionedDataSet inDataSet; // Partitioned variant of the input dataset
  vtkm::Id3 blocksPerDim =
    nDims == 3 ? vtkm::Id3(1, 1, numBlocks) : vtkm::Id3(1, numBlocks, 1); // Decompose the data into
  vtkm::Id3 globalSize = nDims == 3 ? vtkm::Id3(static_cast<vtkm::Id>(dims[0]),
                                                static_cast<vtkm::Id>(dims[1]),
                                                static_cast<vtkm::Id>(dims[2]))
                                    : vtkm::Id3(static_cast<vtkm::Id>(dims[0]),
                                                static_cast<vtkm::Id>(dims[1]),
                                                static_cast<vtkm::Id>(0));
  vtkm::cont::ArrayHandle<vtkm::Id3> localBlockIndices;
  vtkm::cont::ArrayHandle<vtkm::Id3> localBlockOrigins;
  vtkm::cont::ArrayHandle<vtkm::Id3> localBlockSizes;
  localBlockIndices.Allocate(blocksPerRank);
  localBlockOrigins.Allocate(blocksPerRank);
  localBlockSizes.Allocate(blocksPerRank);
  auto localBlockIndicesPortal = localBlockIndices.GetPortalControl();
  auto localBlockOriginsPortal = localBlockOrigins.GetPortalControl();
  auto localBlockSizesPortal = localBlockSizes.GetPortalControl();

  {
    vtkm::Id lastDimSize =
      (nDims == 2) ? static_cast<vtkm::Id>(dims[1]) : static_cast<vtkm::Id>(dims[2]);
    if (size > (lastDimSize / 2.))
    {
      if (rank == 0)
      {
        std::cout << "Number of ranks to large for data. Use " << lastDimSize / 2
                  << "or fewer ranks" << std::endl;
      }
      MPI_Finalize();
      return EXIT_FAILURE;
    }
    vtkm::Id standardBlockSize = (vtkm::Id)(lastDimSize / numBlocks);
    vtkm::Id blockSize = standardBlockSize;
    vtkm::Id blockSliceSize =
      nDims == 2 ? static_cast<vtkm::Id>(dims[0]) : static_cast<vtkm::Id>((dims[0] * dims[1]));
    vtkm::Id blockNumValues = blockSize * blockSliceSize;

    vtkm::Id startBlock = blocksPerRank * rank;
    vtkm::Id endBlock = startBlock + blocksPerRank;
    for (vtkm::Id blockIndex = startBlock; blockIndex < endBlock; ++blockIndex)
    {
      vtkm::Id localBlockIndex = blockIndex - startBlock;
      vtkm::Id blockStart = blockIndex * blockNumValues;
      vtkm::Id blockEnd = blockStart + blockNumValues;
      if (blockIndex < (numBlocks - 1)) // add overlap between regions
      {
        blockEnd += blockSliceSize;
      }
      else
      {
        blockEnd = lastDimSize * blockSliceSize;
      }
      vtkm::Id currBlockSize = (vtkm::Id)((blockEnd - blockStart) / blockSliceSize);

      vtkm::cont::DataSet ds;

      // 2D data
      if (nDims == 2)
      {
        vtkm::Id2 vdims;
        vdims[0] = static_cast<vtkm::Id>(currBlockSize);
        vdims[1] = static_cast<vtkm::Id>(dims[0]);
        vtkm::Vec<ValueType, 2> origin(0, blockIndex * blockSize);
        vtkm::Vec<ValueType, 2> spacing(1, 1);
        ds = dsb.Create(vdims, origin, spacing);

        localBlockIndicesPortal.Set(localBlockIndex, vtkm::Id3(blockIndex, 0, 0));
        localBlockOriginsPortal.Set(localBlockIndex,
                                    vtkm::Id3((blockStart / blockSliceSize), 0, 0));
        localBlockSizesPortal.Set(localBlockIndex,
                                  vtkm::Id3(currBlockSize, static_cast<vtkm::Id>(dims[0]), 0));
      }
      // 3D data
      else
      {
        vtkm::Id3 vdims;
        vdims[0] = static_cast<vtkm::Id>(dims[0]);
        vdims[1] = static_cast<vtkm::Id>(dims[1]);
        vdims[2] = static_cast<vtkm::Id>(currBlockSize);
        vtkm::Vec<ValueType, 3> origin(0, 0, (blockIndex * blockSize));
        vtkm::Vec<ValueType, 3> spacing(1, 1, 1);
        ds = dsb.Create(vdims, origin, spacing);

        localBlockIndicesPortal.Set(localBlockIndex, vtkm::Id3(0, 0, blockIndex));
        localBlockOriginsPortal.Set(localBlockIndex,
                                    vtkm::Id3(0, 0, (blockStart / blockSliceSize)));
        localBlockSizesPortal.Set(
          localBlockIndex,
          vtkm::Id3(static_cast<vtkm::Id>(dims[0]), static_cast<vtkm::Id>(dims[1]), currBlockSize));
      }

      std::vector<vtkm::Float32> subValues((values.begin() + blockStart),
                                           (values.begin() + blockEnd));

      vtkm::cont::DataSetFieldAdd dsf;
      dsf.AddPointField(ds, "values", subValues);
      inDataSet.AppendPartition(ds);
    }
  }
#endif // WITH_MPI construct input dataset

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 buildDatasetTime = currTime - prevTime;
  prevTime = currTime;

  // Convert the mesh of values into contour tree, pairs of vertex ids
  vtkm::filter::ContourTreePPP2 filter(useMarchingCubes, computeRegularStructure);

#ifdef WITH_MPI
  filter.SetSpatialDecomposition(
    blocksPerDim, globalSize, localBlockIndices, localBlockOrigins, localBlockSizes);
#endif
  filter.SetActiveField("values");

  // Execute the contour tree analysis. NOTE: If MPI is used the result  will be
  // a vtkm::cont::PartitionedDataSet instead of a vtkm::cont::DataSet
  auto result = filter.Execute(inDataSet);

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeContourTreeTime = currTime - prevTime;
  prevTime = currTime;

#ifdef WITH_MPI
#ifdef DEBUG_PRINT
  if (rank == 0)
  {
    std::cout << "----- rank=" << rank << " ----Final Contour Tree Data----------------------------"
              << std::endl;
    filter.GetContourTree().PrintContent();
    vtkm::worklet::contourtree_augmented::printIndices("Mesh Sort Order", filter.GetSortOrder());
  }
#endif
#endif

#ifdef DEBUG_TIMING
  if (rank == 0)
  {
    std::cout << "----------------------- " << rank << " --------------------------------------"
              << std::endl;
    std::cout << "-------------------Contour Tree Timings----------------------" << std::endl;

    // Get the timings from the contour tree computation
    const std::vector<std::pair<std::string, vtkm::Float64>>& contourTreeTimings =
      filter.GetTimings();
    for (std::size_t i = 0; i < contourTreeTimings.size(); ++i)
      std::cout << std::setw(42) << std::left << contourTreeTimings[i].first << ": "
                << contourTreeTimings[i].second << " seconds" << std::endl;
  }
#endif


  ////////////////////////////////////////////
  // Compute the branch decomposition
  ////////////////////////////////////////////
  if (rank == 0 && computeBranchDecomposition && computeRegularStructure)
  {
    vtkm::cont::Timer branchDecompTimer;
    branchDecompTimer.Start();
    // compute the volume for each hyperarc and superarc
    cppp2_ns::IdArrayType superarcIntrinsicWeight;
    cppp2_ns::IdArrayType superarcDependentWeight;
    cppp2_ns::IdArrayType supernodeTransferWeight;
    cppp2_ns::IdArrayType hyperarcDependentWeight;

    cppp2_ns::ProcessContourTree::ComputeVolumeWeights(filter.GetContourTree(),
                                                       filter.GetNumIterations(),
                                                       superarcIntrinsicWeight,  // (output)
                                                       superarcDependentWeight,  // (output)
                                                       supernodeTransferWeight,  // (output)
                                                       hyperarcDependentWeight); // (output)
    std::cout << std::setw(42) << std::left << "Compute Volume Weights"
              << ": " << branchDecompTimer.GetElapsedTime() << " seconds" << std::endl;
    branchDecompTimer.Start();

    // compute the branch decomposition by volume
    cppp2_ns::IdArrayType whichBranch;
    cppp2_ns::IdArrayType branchMinimum;
    cppp2_ns::IdArrayType branchMaximum;
    cppp2_ns::IdArrayType branchSaddle;
    cppp2_ns::IdArrayType branchParent;

    cppp2_ns::ProcessContourTree::ComputeVolumeBranchDecomposition(filter.GetContourTree(),
                                                                   superarcDependentWeight,
                                                                   superarcIntrinsicWeight,
                                                                   whichBranch,   // (output)
                                                                   branchMinimum, // (output)
                                                                   branchMaximum, // (output)
                                                                   branchSaddle,  // (output)
                                                                   branchParent); // (output)
    std::cout << std::setw(42) << std::left << "Compute Volume Branch Decomposition"
              << ": " << branchDecompTimer.GetElapsedTime() << " seconds" << std::endl;

    //----main branch decompostion end
    //----Isovalue seleciton start
    if (numLevels > 0) // if compute isovalues
    {
// Get the data values for computing the explicit branch decomposition
// TODO Can we cast the handle we get from GetData() instead of doing a CopyTo?
#ifdef WITH_MPI
      vtkm::cont::ArrayHandle<ValueType> dataField;
      result.GetPartitions()[0].GetField(0).GetData().CopyTo(dataField);
      bool dataFieldIsSorted = true;
#else
      vtkm::cont::ArrayHandle<ValueType> dataField;
      inDataSet.GetField(0).GetData().CopyTo(dataField);
      bool dataFieldIsSorted = false;
#endif

      // create explicit representation of the branch decompostion from the array representation
      BranchType* branchDecompostionRoot =
        cppp2_ns::ProcessContourTree::ComputeBranchDecomposition<ValueType>(
          filter.GetContourTree().superparents,
          filter.GetContourTree().supernodes,
          whichBranch,
          branchMinimum,
          branchMaximum,
          branchSaddle,
          branchParent,
          filter.GetSortOrder(),
          dataField,
          dataFieldIsSorted);

#ifdef DEBUG_PRINT
      branchDecompostionRoot->print(std::cout);
#endif

      // Simplify the contour tree of the branch decompostion
      branchDecompostionRoot->simplifyToSize(numComp, usePersistenceSorter);

      // Compute the relevant iso-values
      std::vector<ValueType> isoValues;
      switch (contourSelectMethod)
      {
        default:
        case 0:
        {
          branchDecompostionRoot->getRelevantValues(static_cast<int>(contourType), eps, isoValues);
        }
        break;
        case 1:
        {
          vtkm::worklet::contourtree_augmented::process_contourtree_inc::PiecewiseLinearFunction<
            ValueType>
            plf;
          branchDecompostionRoot->accumulateIntervals(static_cast<int>(contourType), eps, plf);
          isoValues = plf.nLargest(static_cast<unsigned int>(numLevels));
        }
        break;
      }

      // Print the compute iso values
      std::cout << std::endl;
      std::cout << "Isovalue Suggestions" << std::endl;
      std::cout << "====================" << std::endl;
      std::sort(isoValues.begin(), isoValues.end());
      std::cout << "Isovalues: ";
      for (ValueType val : isoValues)
        std::cout << val << " ";
      std::cout << std::endl;

      // Unique isovalues
      std::vector<ValueType>::iterator it = std::unique(isoValues.begin(), isoValues.end());
      isoValues.resize(static_cast<std::size_t>(std::distance(isoValues.begin(), it)));
      std::cout << isoValues.size() << "  Unique Isovalues: ";
      for (ValueType val : isoValues)
        std::cout << val << " ";
      std::cout << std::endl;
      std::cout << std::endl;
    } //end if compute isovalue
  }

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeBranchDecompTime = currTime - prevTime;
  prevTime = currTime;

  //vtkm::cont::Field resultField =  result.GetField();
  //vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;
  //resultField.GetData().CopyTo(saddlePeak);

  // Dump out contour tree for comparison
  if (rank == 0 && printContourTree)
  {
    std::cout << "Contour Tree" << std::endl;
    std::cout << "============" << std::endl;
    cppp2_ns::EdgePairArray saddlePeak;
    cppp2_ns::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);
    cppp2_ns::printEdgePairArray(saddlePeak);
  }

#ifdef DEBUG_TIMING
#ifdef WITH_MPI
  // Force a simple round-robin on the ranks for the summary prints. Its not perfect for MPI but
  // it works well enough to sort the summaries from the ranks for small-scale debugging.
  if (rank > 0)
  {
    int temp;
    MPI_Status status;
    MPI_Recv(&temp, 1, MPI_INT, (rank - 1), 0, comm, &status);
  }
#endif

  std::cout << "---------------------------" << rank << "----------------------------------"
            << std::endl;
  std::cout << "--------------------------Totals-----------------------------" << std::endl;
  std::cout << std::setw(42) << std::left << "Start-up"
            << ": " << startUpTime << " seconds" << std::endl;
  std::cout << std::setw(42) << std::left << "Data Read"
            << ": " << dataReadTime << " seconds" << std::endl;
  std::cout << std::setw(42) << std::left << "Build VTKM Dataset"
            << ": " << buildDatasetTime << " seconds" << std::endl;
  std::cout << std::setw(42) << std::left << "Compute Contour Tree"
            << ": " << computeContourTreeTime << " seconds" << std::endl;
  if (computeBranchDecomposition)
  {
    std::cout << std::setw(42) << std::left << "Compute Branch Decomposition"
              << ": " << computeBranchDecompTime << " seconds" << std::endl;
  }
  currTime = totalTime.GetElapsedTime();
  //vtkm::Float64 miscTime = currTime - startUpTime - dataReadTime - buildDatasetTime - computeContourTreeTime;
  //if(computeBranchDecomposition) miscTime -= computeBranchDecompTime;
  //std::cout<<std::setw(42)<<std::left<<"Misc. Times"<<": "<<miscTime<<" seconds"<<std::endl;
  std::cout << std::setw(42) << std::left << "Total Time"
            << ": " << currTime << " seconds" << std::endl;

  std::cout << "-------------------------------------------------------------" << std::endl;
  std::cout << "----------------Contour Tree Array Sizes---------------------" << std::endl;
  const cppp2_ns::ContourTree& ct = filter.GetContourTree();
  std::cout << std::setw(42) << std::left << "#Nodes"
            << ": " << ct.nodes.GetNumberOfValues() << std::endl;
  std::cout << std::setw(42) << std::left << "#Arcs"
            << ": " << ct.arcs.GetNumberOfValues() << std::endl;
  std::cout << std::setw(42) << std::left << "#Superparents"
            << ": " << ct.superparents.GetNumberOfValues() << std::endl;
  std::cout << std::setw(42) << std::left << "#Superarcs"
            << ": " << ct.superarcs.GetNumberOfValues() << std::endl;
  std::cout << std::setw(42) << std::left << "#Supernodes"
            << ": " << ct.supernodes.GetNumberOfValues() << std::endl;
  std::cout << std::setw(42) << std::left << "#Hyperparents"
            << ": " << ct.hyperparents.GetNumberOfValues() << std::endl;
  std::cout << std::setw(42) << std::left << "#WhenTransferred"
            << ": " << ct.whenTransferred.GetNumberOfValues() << std::endl;
  std::cout << std::setw(42) << std::left << "#Hypernodes"
            << ": " << ct.hypernodes.GetNumberOfValues() << std::endl;
  std::cout << std::setw(42) << std::left << "#Hyperarcs"
            << ": " << ct.hyperarcs.GetNumberOfValues() << std::endl;
  std::cout << std::flush;

#ifdef WITH_MPI
  // Let the next rank know that it is time to print their summary.
  if (rank < (size - 1))
  {
    int message = 1;
    MPI_Send(&message, 1, MPI_INT, (rank + 1), 0, comm);
  }
#endif
#endif // DEBUG_TIMING

#ifdef WITH_MPI
  MPI_Finalize();
#endif
  return EXIT_SUCCESS;
}
