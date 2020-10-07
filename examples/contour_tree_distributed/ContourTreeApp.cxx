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
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/filter/ContourTreeUniformDistributed.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/ProcessContourTree.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

#ifdef ENABLE_SET_NUM_THREADS
#include "tbb/task_scheduler_init.h"
#endif

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#include <mpi.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

using ValueType = vtkm::Float32;

namespace ctaug_ns = vtkm::worklet::contourtree_augmented;

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
  // Setup the MPI environment.
  MPI_Init(&argc, &argv);
  auto comm = MPI_COMM_WORLD;

  // Tell VTK-m which communicator it should use.
  vtkm::cont::EnvironmentTracker::SetCommunicator(vtkmdiy::mpi::communicator());

  // get the rank and size
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  int numBlocks = size;
  int blocksPerRank = 1;

  // initialize vtkm-m (e.g., logging via -v and device via the -d option)
  vtkm::cont::InitializeOptions vtkm_initialize_options =
    vtkm::cont::InitializeOptions::RequireDevice;
  vtkm::cont::InitializeResult vtkm_config =
    vtkm::cont::Initialize(argc, argv, vtkm_initialize_options);
  auto device = vtkm_config.Device;

  VTKM_LOG_IF_S(vtkm::cont::LogLevel::Info, rank == 0, "Running with MPI. #ranks=" << size);

  // Setup timing
  vtkm::Float64 prevTime = 0;
  vtkm::Float64 currTime = 0;
  vtkm::cont::Timer totalTime;

  totalTime.Start();

  ////////////////////////////////////////////
  // Parse the command line options
  ////////////////////////////////////////////
  ParseCL parser;
  parser.parse(argc, argv);
  std::string filename = parser.getOptions().back();
  bool useMarchingCubes = false;
  if (parser.hasOption("--mc"))
    useMarchingCubes = true;

#ifdef ENABLE_SET_NUM_THREADS
  int numThreads = tbb::task_scheduler_init::default_num_threads();
  if (parser.hasOption("--numThreads"))
  {
    bool deviceIsTBB = (device.GetName() == "TBB");
    // Set the number of threads to be used for TBB
    if (deviceIsTBB)
    {
      numThreads = std::stoi(parser.getOption("--numThreads"));
      tbb::task_scheduler_init schedulerInit(numThreads);
    }
    // Print warning about mismatch between the --numThreads and -d/--device option
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "WARNING: Mismatch between --numThreads and -d/--device option."
                 "numThreads option requires the use of TBB as device. "
                 "Ignoring the numThread option.");
    }
  }
#endif

  if (rank == 0 && (argc < 2 || parser.hasOption("--help") || parser.hasOption("-h")))
  {
    std::cout << "ContourTreeAugmented <options> <fileName>" << std::endl;
    std::cout << std::endl;
    std::cout << "<fileName>       Name of the input data file." << std::endl;
    std::cout << "The file is expected to be ASCII with either: " << std::endl;
    std::cout << "  - xdim ydim integers for 2D or" << std::endl;
    std::cout << "  - xdim ydim zdim integers for 3D" << std::endl;
    std::cout << "followed by vector data last dimension varying fastest" << std::endl;
    std::cout << std::endl;
    std::cout << "----------------------------- VTKM Options -----------------------------"
              << std::endl;
    std::cout << vtkm_config.Usage << std::endl;
    std::cout << std::endl;
    std::cout << "------------------------- Contour Tree Options -------------------------"
              << std::endl;
    std::cout << "Options: (Bool options are give via int, i.e. =0 for False and =1 for True)"
              << std::endl;
    std::cout << "--mc              Use marching cubes interpolation for contour tree calculation. "
                 "(Default=False)"
              << std::endl;
#ifdef ENABLE_SET_NUM_THREADS
    std::cout << "--numThreads      Specifiy the number of threads to use. Available only with TBB."
              << std::endl;
#endif
    std::cout << std::endl;

    MPI_Finalize();
    return EXIT_SUCCESS;
  }

  if (rank == 0)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               std::endl
                 << "    ------------ Settings -----------" << std::endl
                 << "    filename=" << filename << std::endl
                 << "    device=" << device.GetName() << std::endl
                 << "    mc=" << useMarchingCubes << std::endl
#ifdef ENABLE_SET_NUM_THREADS
                 << "    numThreads=" << numThreads << std::endl
#endif
                 << "    nblocks=" << numBlocks << std::endl);
  }
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 startUpTime = currTime - prevTime;
  prevTime = currTime;

// Redirect stdout to file if we are using MPI with Debugging
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
  std::size_t numVertices = static_cast<std::size_t>(
    std::accumulate(dims.begin(), dims.end(), std::size_t(1), std::multiplies<std::size_t>()));

  // Print the mesh metadata
  if (rank == 0)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               std::endl
                 << "    ---------------- Input Mesh Properties --------------" << std::endl
                 << "    Number of dimensions: " << nDims << std::endl
                 << "    Number of mesh vertices: " << numVertices << std::endl);
  }

  // Check for fatal input errors
  // Check the the number of dimensiosn is either 2D or 3D
  bool invalidNumDimensions = (nDims < 2 || nDims > 3);
  // Check if marching cubes is enabled for non 3D data
  bool invalidMCOption = (useMarchingCubes && nDims != 3);
  // Log any errors if found on rank 0
  VTKM_LOG_IF_S(vtkm::cont::LogLevel::Error,
                invalidNumDimensions && (rank == 0),
                "The input mesh is " << nDims
                                     << "D. "
                                        "The input data must be either 2D or 3D.");
  VTKM_LOG_IF_S(vtkm::cont::LogLevel::Error,
                invalidMCOption && (rank == 0),
                "The input mesh is "
                  << nDims << "D. "
                  << "Contour tree using marching cubes is only supported for 3D data.");
  // If we found any errors in the setttings than finalize MPI and exit the execution
  if (invalidNumDimensions || invalidMCOption)
  {
    MPI_Finalize();
    return EXIT_SUCCESS;
  }

  // Read data
  std::vector<ValueType> values(numVertices);
  for (std::size_t vertex = 0; vertex < numVertices; ++vertex)
  {
    inFile >> values[vertex];
  }

  // finish reading the data
  inFile.close();

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 dataReadTime = currTime - prevTime;
  prevTime = currTime;

  vtkm::cont::DataSetBuilderUniform dsb;
  // Create a multi-block dataset for multi-block DIY-paralle processing
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
  auto localBlockIndicesPortal = localBlockIndices.WritePortal();
  auto localBlockOriginsPortal = localBlockOrigins.WritePortal();
  auto localBlockSizesPortal = localBlockSizes.WritePortal();

  {
    vtkm::Id lastDimSize =
      (nDims == 2) ? static_cast<vtkm::Id>(dims[1]) : static_cast<vtkm::Id>(dims[2]);
    if (size > (lastDimSize / 2.))
    {
      VTKM_LOG_IF_S(vtkm::cont::LogLevel::Error,
                    rank == 0,
                    "Number of ranks to large for data. Use " << lastDimSize / 2
                                                              << "or fewer ranks");
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

      //vtkm::cont::DataSetFieldAdd dsf;
      ds.AddPointField("values", subValues);
      inDataSet.AppendPartition(ds);
    }
  }

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 buildDatasetTime = currTime - prevTime;
  prevTime = currTime;

  // Convert the mesh of values into contour tree, pairs of vertex ids
  vtkm::filter::ContourTreeUniformDistributed filter(useMarchingCubes);

  filter.SetSpatialDecomposition(
    blocksPerDim, globalSize, localBlockIndices, localBlockOrigins, localBlockSizes);
  filter.SetActiveField("values");

  // Execute the contour tree analysis. NOTE: If MPI is used the result  will be
  // a vtkm::cont::PartitionedDataSet instead of a vtkm::cont::DataSet
  auto result = filter.Execute(inDataSet);

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeContourTreeTime = currTime - prevTime;
  prevTime = currTime;

  // Force a simple round-robin on the ranks for the summary prints. Its not perfect for MPI but
  // it works well enough to sort the summaries from the ranks for small-scale debugging.
  if (rank > 0)
  {
    int temp;
    MPI_Status status;
    MPI_Recv(&temp, 1, MPI_INT, (rank - 1), 0, comm, &status);
  }
  currTime = totalTime.GetElapsedTime();
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             std::endl
               << "    -------------------------- Totals " << rank
               << " -----------------------------" << std::endl
               << std::setw(42) << std::left << "    Start-up"
               << ": " << startUpTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Data Read"
               << ": " << dataReadTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Build VTKM Dataset"
               << ": " << buildDatasetTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Compute Contour Tree"
               << ": " << computeContourTreeTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Total Time"
               << ": " << currTime << " seconds");

  // Flush ouput streams just to make sure everything has been logged (in particular when using MPI)
  std::cout << std::flush;
  std::cerr << std::flush;

  // Let the next rank know that it is time to print their summary.
  if (rank < (size - 1))
  {
    int message = 1;
    MPI_Send(&message, 1, MPI_INT, (rank + 1), 0, comm);
  }
  MPI_Finalize();
  return EXIT_SUCCESS;
}
