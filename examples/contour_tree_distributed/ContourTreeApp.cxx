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
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/io/BOVDataSetReader.h>

#include "ContourTreeAppDataIO.h"

#include <vtkm/filter/scalar_topology/ContourTreeUniformDistributed.h>
#include <vtkm/filter/scalar_topology/DistributedBranchDecompositionFilter.h>
#include <vtkm/filter/scalar_topology/SelectTopVolumeContoursFilter.h>
#include <vtkm/filter/scalar_topology/worklet/branch_decomposition/HierarchicalVolumetricBranchDecomposer.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/ProcessContourTree.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/TreeCompiler.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#include <mpi.h>

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using ValueType = vtkm::Float64;

#define SINGLE_FILE_STDOUT_STDERR

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
      return static_cast<vtkm::Id>(it - this->mCLOptions.begin());
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
  // Set the logging levels we want to use. These could be made command line options as well
  // Log level to be used for outputting timing information.
  vtkm::cont::LogLevel timingsLogLevel = vtkm::cont::LogLevel::Info;
  // Log level to be used for outputting metadata about the trees.
  vtkm::cont::LogLevel treeLogLevel = vtkm::cont::LogLevel::Info;
  // Log level for outputs specific to the example
  vtkm::cont::LogLevel exampleLogLevel = vtkm::cont::LogLevel::Info;

  // Setup the MPI environment.
  MPI_Init(&argc, &argv);
  auto comm = MPI_COMM_WORLD;

  // Tell VTK-m which communicator it should use.
  vtkm::cont::EnvironmentTracker::SetCommunicator(vtkmdiy::mpi::communicator());

  // get the rank and size
  int rank, size;
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  // initialize vtkm-m (e.g., logging via -v and device via the -d option)
  vtkm::cont::InitializeOptions vtkm_initialize_options =
    vtkm::cont::InitializeOptions::RequireDevice;
  vtkm::cont::InitializeResult vtkm_config =
    vtkm::cont::Initialize(argc, argv, vtkm_initialize_options);
  auto device = vtkm_config.Device;

  VTKM_LOG_IF_S(exampleLogLevel, rank == 0, "Running with MPI. #ranks=" << size);

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
  bool augmentHierarchicalTree = false;
  if (parser.hasOption("--augmentHierarchicalTree"))
  {
    augmentHierarchicalTree = true;
  }

  bool computeHierarchicalVolumetricBranchDecomposition = false;
  if (parser.hasOption("--computeVolumeBranchDecomposition"))
  {
    computeHierarchicalVolumetricBranchDecomposition = true;
    if (!augmentHierarchicalTree)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "Warning: --computeVolumeBranchDecomposition requires augmentation. "
                 "Enabling --augmentHierarchicalTree option.");
      augmentHierarchicalTree = true;
    }
  }

  bool useBoundaryExtremaOnly = true;
  if (parser.hasOption("--useFullBoundary"))
  {
    useBoundaryExtremaOnly = false;
  }
  bool useMarchingCubes = false;
  if (parser.hasOption("--mc"))
  {
    useMarchingCubes = true;
  }
  bool preSplitFiles = false;
  if (parser.hasOption("--preSplitFiles"))
  {
    preSplitFiles = true;
  }
  bool saveDotFiles = false;
  if (parser.hasOption("--saveDot"))
  {
    saveDotFiles = true;
  }
  bool saveOutputData = false;
  if (parser.hasOption("--saveOutputData"))
  {
    saveOutputData = true;
  }
  bool forwardSummary = false;
  if (parser.hasOption("--forwardSummary"))
  {
    forwardSummary = true;
  }

  int numBlocks = size;
  int blocksPerRank = 1;
  if (parser.hasOption("--numBlocks"))
  {
    numBlocks = std::stoi(parser.getOption("--numBlocks"));
    if (numBlocks % size == 0)
      blocksPerRank = numBlocks / size;
    else
    {
      std::cerr << "Error: Number of blocks must be divisible by number of ranks." << std::endl;
      MPI_Finalize();
      return EXIT_FAILURE;
    }
  }

  vtkm::Id numBranches = 0;
  if (parser.hasOption("--numBranches"))
  {
    numBranches = std::stoi(parser.getOption("--numBranches"));
    if (!computeHierarchicalVolumetricBranchDecomposition)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "Warning: --numBranches requires computing branch decomposition. "
                 "Enabling --computeHierarchicalVolumetricBranchDecomposition option.");
      computeHierarchicalVolumetricBranchDecomposition = true;
    }
    if (!augmentHierarchicalTree)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "Warning: --numBranches requires augmentation. "
                 "Enabling --augmentHierarchicalTree option.");
      augmentHierarchicalTree = true;
    }
  }

  ValueType eps = 0.00001f;

  if (parser.hasOption("--eps"))
    eps = std::stof(parser.getOption("--eps"));

#ifdef ENABLE_HDFIO
  std::string dataset_name = "data";
  if (parser.hasOption("--dataset"))
  {
    dataset_name = parser.getOption("--dataset");
  }

  vtkm::Id3 blocksPerDimIn(1, 1, size);
  if (parser.hasOption("--blocksPerDim"))
  {
    std::string temp = parser.getOption("--blocksPerDim");
    if (std::count(temp.begin(), temp.end(), ',') != 2)
    {
      std::cerr << "Invalid --blocksPerDim option. Expected string of the form 'x,y,z' got" << temp
                << std::endl;
      MPI_Finalize();
      return EXIT_FAILURE;
    }
    char* tempC = (char*)temp.c_str();
    blocksPerDimIn[0] = std::stoi(std::strtok(tempC, ","));
    blocksPerDimIn[1] = std::stoi(std::strtok(nullptr, ","));
    blocksPerDimIn[2] = std::stoi(std::strtok(nullptr, ","));
  }

  vtkm::Id3 selectSize(-1, -1, -1);
  if (parser.hasOption("--selectSize"))
  {
    std::string temp = parser.getOption("--selectSize");
    if (std::count(temp.begin(), temp.end(), ',') != 2)
    {
      std::cerr << "Invalid --selectSize option. Expected string of the form 'x,y,z' got" << temp
                << std::endl;
      MPI_Finalize();
      return EXIT_FAILURE;
    }
    char* tempC = (char*)temp.c_str();
    selectSize[0] = std::stoi(std::strtok(tempC, ","));
    selectSize[1] = std::stoi(std::strtok(nullptr, ","));
    selectSize[2] = std::stoi(std::strtok(nullptr, ","));
  }
#endif

  if (argc < 2 || parser.hasOption("--help") || parser.hasOption("-h"))
  {
    if (rank == 0)
    {
      std::cout << "ContourTreeDistributed <options> <fileName>" << std::endl;
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
      std::cout << "--mc             Use marching cubes connectivity (Default=False)." << std::endl;
      std::cout << "--useFullBoundary Use the full boundary during. Typically only useful"
                << std::endl
                << "                  to compare the performance between using the full boundary"
                << std::endl;
      std::cout << "                 and when using only boundary extrema." << std::endl;
      std::cout << "--augmentHierarchicalTree Augment the hierarchical tree." << std::endl;
      std::cout << "--computeVolumeBranchDecomposition Compute the volume branch decomposition. "
                << std::endl;
      std::cout << "                 Requires --augmentHierarchicalTree to be set." << std::endl;
      std::cout << "--numBranches    Number of top volume branches to select." << std::endl;
      std::cout << "                 Requires --computeVolumeBranchDecomposition." << std::endl;
      std::cout
        << "--eps=<float>   Floating point offset awary from the critical point. (default=0.00001)"
        << std::endl;
      std::cout << "--preSplitFiles  Input data is already pre-split into blocks." << std::endl;
      std::cout << "--saveDot        Save DOT files of the distributed contour tree " << std::endl
                << "                 computation (Default=False). " << std::endl;
      std::cout << "--saveOutputData  Save data files with hierarchical tree or volume data"
                << std::endl;
      std::cout << "--numBlocks      Number of blocks to use during computation. (Sngle block "
                   "ASCII/BOV file reader only)"
                << "(Default=number of MPI ranks.)" << std::endl;
      std::cout << "--forwardSummary Forward the summary timings also to the per-rank " << std::endl
                << "                 log files. Default is to round-robin print the " << std::endl
                << "                 summary instead" << std::endl;
#ifdef ENABLE_HDFIO
      std::cout << "--dataset        Name of the dataset to load (HDF5 reader only)(Default=data)"
                << std::endl;
      std::cout << "--blocksPerDim   Number of blocks to split the data into. This is a string of "
                   "the form 'x,y,z'."
                   "(HDF5 reader only)(Default='1,1,#ranks')"
                << std::endl;
      std::cout
        << "--selectSize   Size of the subblock to read. This is a string of the form 'x,y,z'."
           "(HDF5 reader only)(Default='-1,-1,-1')"
        << std::endl;
#endif
      std::cout << std::endl;
    }
    MPI_Finalize();
    return EXIT_SUCCESS;
  }

  if (rank == 0)
  {
    VTKM_LOG_S(exampleLogLevel,
               std::endl
                 << "    ------------ Settings -----------" << std::endl
                 << "    filename=" << filename << std::endl
                 << "    preSplitFiles=" << preSplitFiles << std::endl
                 << "    device=" << device.GetName() << std::endl
                 << "    mc=" << useMarchingCubes << std::endl
                 << "    useFullBoundary=" << !useBoundaryExtremaOnly << std::endl
                 << "    saveDot=" << saveDotFiles << std::endl
                 << "    augmentHierarchicalTree=" << augmentHierarchicalTree << std::endl
                 << "    computeVolumetricBranchDecomposition="
                 << computeHierarchicalVolumetricBranchDecomposition << std::endl
                 << "    saveOutputData=" << saveOutputData << std::endl
                 << "    forwardSummary=" << forwardSummary << std::endl
                 << "    nblocks=" << numBlocks << std::endl
                 << "    nbranches=" << numBranches << std::endl
                 << "    eps=" << eps << std::endl
#ifdef ENABLE_HDFIO
                 << "    dataset=" << dataset_name << " (HDF5 only)" << std::endl
                 << "    blocksPerDim=" << blocksPerDimIn[0] << "," << blocksPerDimIn[1] << ","
                 << blocksPerDimIn[2] << " (HDF5 only)" << std::endl
                 << "    selectSize=" << selectSize[0] << "," << selectSize[1] << ","
                 << selectSize[2] << " (HDF5 only)" << std::endl
#endif
    );
  }

  // Redirect stdout to file if we are using MPI with Debugging
  // From https://www.unix.com/302983597-post2.html
  char cstr_filename[255];
  std::snprintf(cstr_filename, sizeof(cstr_filename), "cout_%d.log", rank);
  int out = open(cstr_filename, O_RDWR | O_CREAT | O_TRUNC, 0600);
  if (-1 == out)
  {
    perror("opening cout.log");
    return 255;
  }

#ifndef SINGLE_FILE_STDOUT_STDERR
  std::snprintf(cstr_filename, sizeof(cstr_filename), "cerr_%d.log", rank);
  int err = open(cstr_filename, O_RDWR | O_CREAT | O_TRUNC, 0600);
  if (-1 == err)
  {
    perror("opening cerr.log");
    return 255;
  }
#endif

  int save_out = dup(fileno(stdout));
  int save_err = dup(fileno(stderr));

  if (-1 == dup2(out, fileno(stdout)))
  {
    perror("cannot redirect stdout");
    return 255;
  }
#ifdef SINGLE_FILE_STDOUT_STDERR
  if (-1 == dup2(out, fileno(stderr)))
#else
  if (-1 == dup2(err, fileno(stderr)))
#endif
  {
    perror("cannot redirect stderr");
    return 255;
  }


  VTKM_LOG_S(exampleLogLevel,
             std::endl
               << "    ------------ Settings -----------" << std::endl
               << "    filename=" << filename << std::endl
               << "    preSplitFiles=" << preSplitFiles << std::endl
               << "    device=" << device.GetName() << std::endl
               << "    mc=" << useMarchingCubes << std::endl
               << "    useFullBoundary=" << !useBoundaryExtremaOnly << std::endl
               << "    saveDot=" << saveDotFiles << std::endl
               << "    saveOutputData=" << saveOutputData << std::endl
               << "    forwardSummary=" << forwardSummary << std::endl
               << "    numBlocks=" << numBlocks << std::endl
               << "    augmentHierarchicalTree=" << augmentHierarchicalTree << std::endl
               << "    numRanks=" << size << std::endl
               << "    rank=" << rank << std::endl
#ifdef ENABLE_HDFIO
               << "    dataset=" << dataset_name << " (HDF5 only)" << std::endl
               << "    blocksPerDim=" << blocksPerDimIn[0] << "," << blocksPerDimIn[1] << ","
               << blocksPerDimIn[2] << " (HDF5 only)" << std::endl
               << "    selectSize=" << selectSize[0] << "," << selectSize[1] << "," << selectSize[2]
               << " (HDF5 only)" << std::endl
#endif
  );

  // Measure our time for startup
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 startUpTime = currTime - prevTime;
  prevTime = currTime;

  // Make sure that all ranks have started up before we start the data read
  MPI_Barrier(comm);
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 startUpSyncTime = currTime - prevTime;
  prevTime = currTime;

  ///////////////////////////////////////////////
  // Read the input data
  ///////////////////////////////////////////////
  vtkm::Float64 dataReadTime = 0;
  vtkm::Float64 buildDatasetTime = 0;
  std::vector<vtkm::Float32>::size_type nDims = 0;

  // Multi-block dataset for multi-block DIY-paralle processing
  vtkm::cont::PartitionedDataSet useDataSet;

  // Domain decomposition information for DIY/contour tree
  vtkm::Id3 globalSize;
  vtkm::Id3 blocksPerDim;
  vtkm::cont::ArrayHandle<vtkm::Id3> localBlockIndices;
  localBlockIndices.Allocate(blocksPerRank);
  auto localBlockIndicesPortal = localBlockIndices.WritePortal();

  // Read the pre-split data files
  bool readOk = true;
  if (preSplitFiles)
  {
    readOk = readPreSplitFiles<ValueType>(
      // inputs
      rank,
      filename,
      blocksPerRank,
      // outputs
      nDims,
      useDataSet,
      globalSize,
      blocksPerDim,
      localBlockIndices,
      // output timers
      dataReadTime,
      buildDatasetTime);
  }
  // Read single-block data and split it for the ranks
  else
  {
    bool isHDF5 = (0 == filename.compare(filename.length() - 3, 3, ".h5"));
    if (isHDF5)
    {
#ifdef ENABLE_HDFIO
      blocksPerDim = blocksPerDimIn;
      readOk = read3DHDF5File<ValueType>(
        // inputs (blocksPerDim is being modified to swap dimension to fit we re-ordering of dimension)
        rank,
        filename,
        dataset_name,
        blocksPerRank,
        blocksPerDim,
        selectSize,
        // outputs
        nDims,
        useDataSet,
        globalSize,
        localBlockIndices,
        // output timers
        dataReadTime,
        buildDatasetTime);
#else
      VTKM_LOG_S(vtkm::cont::LogLevel::Error,
                 "Can't read HDF5 file. HDF5 reader disabled for this build.");
      readOk = false;
#endif
    }
    else
    {
      readOk = readSingleBlockFile<ValueType>(
        // inputs
        rank,
        size,
        filename,
        numBlocks,
        blocksPerRank,
        // outputs
        nDims,
        useDataSet,
        globalSize,
        blocksPerDim,
        localBlockIndices,
        // output timers
        dataReadTime,
        buildDatasetTime);
    }
  }
  if (!readOk)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error, "Data read failed.");
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  // Print the mesh metadata
  if (rank == 0)
  {
    VTKM_LOG_S(exampleLogLevel,
               std::endl
                 << "    ---------------- Input Mesh Properties --------------" << std::endl
                 << "    Number of dimensions: " << nDims);
  }

  // Check if marching cubes is enabled for non 3D data
  bool invalidMCOption = (useMarchingCubes && nDims != 3);
  VTKM_LOG_IF_S(vtkm::cont::LogLevel::Error,
                invalidMCOption && (rank == 0),
                "The input mesh is "
                  << nDims << "D. "
                  << "Contour tree using marching cubes is only supported for 3D data.");

  // If we found any errors in the settings than finalize MPI and exit the execution
  if (invalidMCOption)
  {
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  // reset timer after read. the dataReadTime and buildDatasetTime are measured by the read functions
  prevTime = totalTime.GetElapsedTime();

  // Make sure that all ranks have started up before we start the data read
  MPI_Barrier(comm);
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 dataReadSyncTime = currTime - prevTime;
  prevTime = currTime;

  // Log information of the (first) local data block
  // TODO: Get localBlockSize and localBlockOrigins from the cell set to log results
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "" //<< std::setw(42) << std::left << "blockSize"
               //<< ":" << localBlockSizesPortal.Get(0) << std::endl
               //<< std::setw(42) << std::left << "blockOrigin=" << localBlockOriginsPortal.Get(0)
               //<< std::endl
               << std::setw(42) << std::left << "blockIndices=" << localBlockIndicesPortal.Get(0)
               << std::endl
               << std::setw(42) << std::left << "blocksPerDim=" << blocksPerDim << std::endl
               << std::setw(42) << std::left << "globalSize=" << globalSize << std::endl

  );

  // Convert the mesh of values into contour tree, pairs of vertex ids
  vtkm::filter::scalar_topology::ContourTreeUniformDistributed filter(timingsLogLevel,
                                                                      treeLogLevel);
  filter.SetBlockIndices(blocksPerDim, localBlockIndices);
  filter.SetUseBoundaryExtremaOnly(useBoundaryExtremaOnly);
  filter.SetUseMarchingCubes(useMarchingCubes);
  filter.SetAugmentHierarchicalTree(augmentHierarchicalTree);
  filter.SetSaveDotFiles(saveDotFiles);
  filter.SetActiveField("values");

  // Execute the contour tree analysis
  auto result = filter.Execute(useDataSet);

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeContourTreeTime = currTime - prevTime;
  prevTime = currTime;

  // Record the time to synchronize after the filter has finished
  MPI_Barrier(comm);
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 postFilterSyncTime = currTime - prevTime;
  prevTime = currTime;

  // Compute branch decomposition if requested
  vtkm::cont::PartitionedDataSet bd_result;
  if (computeHierarchicalVolumetricBranchDecomposition)
  {
    vtkm::filter::scalar_topology::DistributedBranchDecompositionFilter bd_filter;
    bd_result = bd_filter.Execute(result);
  }
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 branchDecompTime = currTime - prevTime;
  prevTime = currTime;

  // Compute SelectTopVolumeContours if needed
  vtkm::cont::PartitionedDataSet tp_result;
  if (numBranches > 0)
  {
    vtkm::filter::scalar_topology::SelectTopVolumeContoursFilter tp_filter;
    tp_filter.SetSavedBranches(numBranches);
    tp_result = tp_filter.Execute(bd_result);
  }

  // Save output
  if (saveOutputData)
  {
    if (augmentHierarchicalTree)
    {
      if (computeHierarchicalVolumetricBranchDecomposition)
      {
        for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
        {
          auto ds = bd_result.GetPartition(ds_no);
          std::string branchDecompositionIntermediateFileName =
            std::string("BranchDecompositionIntermediate_Rank_") +
            std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
            std::to_string(static_cast<int>(ds_no)) + std::string(".txt");

          std::ofstream treeStreamIntermediate(branchDecompositionIntermediateFileName.c_str());

          treeStreamIntermediate
            << vtkm::filter::scalar_topology::HierarchicalVolumetricBranchDecomposer::PrintBranches(
                 ds);

          std::string branchDecompositionFileName = std::string("BranchDecomposition_Rank_") +
            std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
            std::to_string(static_cast<int>(ds_no)) + std::string(".txt");
          std::ofstream treeStream(branchDecompositionFileName.c_str());

          auto upperEndGRId = ds.GetField("UpperEndGlobalRegularIds")
                                .GetData()
                                .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>()
                                .ReadPortal();
          auto lowerEndGRId = ds.GetField("LowerEndGlobalRegularIds")
                                .GetData()
                                .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>()
                                .ReadPortal();
          vtkm::Id nBranches = upperEndGRId.GetNumberOfValues();

          for (vtkm::Id branch = 0; branch < nBranches; ++branch)
          {
            treeStream << std::setw(12) << upperEndGRId.Get(branch) << std::setw(14)
                       << lowerEndGRId.Get(branch) << std::endl;
          }
        }

        if (numBranches > 0)
        {
#ifndef DEBUG_PRINT
          bool print_to_files = (rank == 0);
          vtkm::Id max_blocks_to_print = 1;
#else
          bool print_to_files = true;
          vtkm::Id max_blocks_to_print = result.GetNumberOfPartitions();
#endif

          for (vtkm::Id ds_no = 0; print_to_files && ds_no < max_blocks_to_print; ++ds_no)
          {
            auto ds = tp_result.GetPartition(ds_no);
            std::string topVolumeBranchFileName = std::string("TopVolumeBranch_Rank_") +
              std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
              std::to_string(static_cast<int>(ds_no)) + std::string(".txt");
            std::ofstream topVolumeBranchStream(topVolumeBranchFileName.c_str());
            auto topVolBranchGRId = ds.GetField("TopVolumeBranchGlobalRegularIds")
                                      .GetData()
                                      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>()
                                      .ReadPortal();
            auto topVolBranchVolume = ds.GetField("TopVolumeBranchVolume")
                                        .GetData()
                                        .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>()
                                        .ReadPortal();
            auto topVolBranchSaddleEpsilon = ds.GetField("TopVolumeBranchSaddleEpsilon")
                                               .GetData()
                                               .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>()
                                               .ReadPortal();
            auto topVolBranchSaddleIsoValue = ds.GetField("TopVolumeBranchSaddleIsoValue")
                                                .GetData()
                                                .AsArrayHandle<vtkm::cont::ArrayHandle<ValueType>>()
                                                .ReadPortal();

            vtkm::Id nSelectedBranches = topVolBranchGRId.GetNumberOfValues();
            for (vtkm::Id branch = 0; branch < nSelectedBranches; ++branch)
            {
              topVolumeBranchStream << std::setw(12) << topVolBranchGRId.Get(branch)
                                    << std::setw(14) << topVolBranchVolume.Get(branch)
                                    << std::setw(5) << topVolBranchSaddleEpsilon.Get(branch)
                                    << std::setw(14) << topVolBranchSaddleIsoValue.Get(branch)
                                    << std::endl;
            }

            std::string isoValuesFileName = std::string("IsoValues_Rank_") +
              std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
              std::to_string(static_cast<int>(ds_no)) + std::string(".txt");
            std::ofstream isoValuesStream(isoValuesFileName.c_str());

            for (vtkm::Id branch = 0; branch < nSelectedBranches; ++branch)
            {
              isoValuesStream << (topVolBranchSaddleIsoValue.Get(branch) +
                                  (eps * topVolBranchSaddleEpsilon.Get(branch)))
                              << " ";
            }

            isoValuesStream << std::endl;
          }
        }
      }
      else
      {
        for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
        {
          auto ds = result.GetPartition(ds_no);
          vtkm::worklet::contourtree_augmented::IdArrayType supernodes;
          ds.GetField("Supernodes").GetData().AsArrayHandle(supernodes);
          vtkm::worklet::contourtree_augmented::IdArrayType superarcs;
          ds.GetField("Superarcs").GetData().AsArrayHandle(superarcs);
          vtkm::worklet::contourtree_augmented::IdArrayType regularNodeGlobalIds;
          ds.GetField("RegularNodeGlobalIds").GetData().AsArrayHandle(regularNodeGlobalIds);
          vtkm::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
          vtkm::worklet::contourtree_augmented::IdArrayType intrinsicVolume;
          ds.GetField("IntrinsicVolume").GetData().AsArrayHandle(intrinsicVolume);
          vtkm::worklet::contourtree_augmented::IdArrayType dependentVolume;
          ds.GetField("DependentVolume").GetData().AsArrayHandle(dependentVolume);

          std::string dumpVolumesString =
            vtkm::worklet::contourtree_distributed::HierarchicalContourTree<ValueType>::DumpVolumes(
              supernodes,
              superarcs,
              regularNodeGlobalIds,
              totalVolume,
              intrinsicVolume,
              dependentVolume);

          std::string volumesFileName = std::string("TreeWithVolumes_Rank_") +
            std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
            std::to_string(static_cast<int>(ds_no)) + std::string(".txt");
          std::ofstream treeStream(volumesFileName.c_str());
          treeStream << dumpVolumesString;
        }
      }
    }
    else
    {
      for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
      {
        vtkm::worklet::contourtree_distributed::TreeCompiler treeCompiler;
        treeCompiler.AddHierarchicalTree(result.GetPartition(ds_no));
        char fname[256];
        std::snprintf(fname,
                      sizeof(fname),
                      "TreeCompilerOutput_Rank%d_Block%d.dat",
                      rank,
                      static_cast<int>(ds_no));
        FILE* out_file = std::fopen(fname, "wb");
        treeCompiler.WriteBinary(out_file);
        std::fclose(out_file);
      }
    }
  }


  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 saveOutputDataTime = currTime - prevTime;
  prevTime = currTime;

  std::cout << std::flush;
  std::cerr << std::flush;
  close(out);
#ifndef SINGLE_FILE_STDOUT_STDERR
  close(err);
#endif

  if (!forwardSummary)
  {
    // write our log data now and close the files so that the summary is written round-robin to
    // the normal log output stream
    dup2(save_out, fileno(stdout));
    dup2(save_err, fileno(stderr));
    // close our MPI forward
    close(save_out);
    close(save_err);

    // Force a simple round-robin on the ranks for the summary prints. Its not perfect for MPI but
    // it works well enough to sort the summaries from the ranks for small-scale debugging.
    if (rank > 0)
    {
      int temp;
      MPI_Status status;
      MPI_Recv(&temp, 1, MPI_INT, (rank - 1), 0, comm, &status);
    }
  }
  currTime = totalTime.GetElapsedTime();
  VTKM_LOG_S(timingsLogLevel,
             std::endl
               << "    -------------------------- Totals " << rank
               << " -----------------------------" << std::endl
               << std::setw(42) << std::left << "    Start-up"
               << ": " << startUpTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Start-up Sync"
               << ": " << startUpSyncTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Data Read"
               << ": " << dataReadTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Build VTKM Dataset"
               << ": " << buildDatasetTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Data Read/Build Sync"
               << ": " << dataReadSyncTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Compute Contour Tree"
               << ": " << computeContourTreeTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Post filter Sync"
               << ": " << postFilterSyncTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Branch Decomposition"
               << ": " << branchDecompTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Save Tree Compiler Data"
               << ": " << saveOutputDataTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Total Time"
               << ": " << currTime << " seconds");

  // Flush ouput streams just to make sure everything has been logged (in particular when using MPI)
  std::cout << std::flush;
  std::cerr << std::flush;

  // Round robin output
  if (!forwardSummary)
  {
    // Let the next rank know that it is time to print their summary.
    if (rank < (size - 1))
    {
      int message = 1;
      MPI_Send(&message, 1, MPI_INT, (rank + 1), 0, comm);
    }
  }
  // Finish logging to our per-rank log-files
  else
  {
    // write our log data now
    // the normal log output stream
    dup2(save_out, fileno(stdout));
    dup2(save_err, fileno(stderr));
    // close our MPI forward
    close(save_out);
    close(save_err);
  }

  // Finalize and finish the execution
  MPI_Finalize();
  return EXIT_SUCCESS;
}
