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
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/io/BOVDataSetReader.h>

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
#include <sstream>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

using ValueType = vtkm::Float32;
using BranchType = vtkm::worklet::contourtree_augmented::process_contourtree_inc::Branch<ValueType>;

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
#ifdef WITH_MPI
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
#endif

  // initialize vtkm-m (e.g., logging via -v and device via the -d option)
  vtkm::cont::InitializeOptions vtkm_initialize_options =
    vtkm::cont::InitializeOptions::RequireDevice;
  vtkm::cont::InitializeResult vtkm_config =
    vtkm::cont::Initialize(argc, argv, vtkm_initialize_options);
  auto device = vtkm_config.Device;

#ifdef WITH_MPI
  VTKM_LOG_IF_S(vtkm::cont::LogLevel::Info, rank == 0, "Running with MPI. #ranks=" << size);
#else
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Single node run");
  int rank = 0;
#endif

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
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "Regular structure is required for branch decomposition."
               " Disabling branch decomposition");
    computeBranchDecomposition = false;
  }


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
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "Iso level selection only available when branch decomposition is enabled. "
               "Disabling iso value selection");
    numLevels = 0;
  }

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
#ifdef ENABLE_SET_NUM_THREADS
    std::cout << "--numThreads      Specifiy the number of threads to use. Available only with TBB."
              << std::endl;
#endif
    std::cout << std::endl;
    std::cout << "---------------------- Isovalue Selection Options ----------------------"
              << std::endl;
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
    std::cout << std::endl;

#ifdef WITH_MPI
    MPI_Finalize();
#endif
    return EXIT_SUCCESS;
  }

  if (rank == 0)
  {
    std::stringstream logmessage;
    logmessage << "    ------------ Settings -----------" << std::endl
               << "    filename=" << filename << std::endl
               << "    device=" << device.GetName() << std::endl
               << "    mc=" << useMarchingCubes << std::endl
               << "    augmentTree=" << computeRegularStructure << std::endl
               << "    branchDecomp=" << computeBranchDecomposition << std::endl
               <<
#ifdef WITH_MPI
      "    nblocks=" << numBlocks << std::endl
               <<
#endif
#ifdef ENABLE_SET_NUM_THREADS
      "    numThreads=" << numThreads << std::endl
               <<
#endif
      "    computeIsovalues=" << (numLevels > 0);
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, std::endl << logmessage.str());
    VTKM_LOG_IF_S(vtkm::cont::LogLevel::Info,
                  numLevels > 0,
                  std::endl
                    << "    ------------ Settings Isolevel Selection -----------" << std::endl
                    << "    levels=" << numLevels << std::endl
                    << "    eps=" << eps << std::endl
                    << "    comp" << numComp << std::endl
                    << "    type=" << contourType << std::endl
                    << "    method=" << contourSelectMethod << std::endl
                    << "    mc=" << useMarchingCubes << std::endl
                    << "    use" << (usePersistenceSorter ? "PersistenceSorter" : "VolumeSorter"));
  }
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 startUpTime = currTime - prevTime;
  prevTime = currTime;

// Redirect stdout to file if we are using MPI with Debugging
#ifdef WITH_MPI
#ifdef DEBUG_PRINT
  // From https://www.unix.com/302983597-post2.html
  char cstr_filename[32];
  snprintf(cstr_filename, sizeof(cstr_filename), "cout_%d.log", rank);
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
#endif
#endif

  ///////////////////////////////////////////////
  // Read the input data
  ///////////////////////////////////////////////
  vtkm::Float64 dataReadTime = 0;
  vtkm::Float64 buildDatasetTime = 0;
  std::vector<vtkm::Float32>::size_type nDims = 0;
  vtkm::cont::DataSet inDataSet;
  std::vector<ValueType> values;
  std::vector<vtkm::Id> dims;
  if (filename.compare(filename.length() - 3, 3, "bov") == 0)
  {
    std::cout << "Reading BOV file" << std::endl;
    vtkm::io::BOVDataSetReader reader(filename);
    inDataSet = reader.ReadDataSet();
    nDims = 3;
    currTime = totalTime.GetElapsedTime();
    dataReadTime = currTime - prevTime;
    prevTime = currTime;
#ifdef WITH_MPI
    // Copy the data into the values array so we can construct a multiblock dataset
    // TODO All we should need to do to implement BOV support is to copy the values
    // in the values vector and copy the dimensions in the dims vector
    vtkm::Id3 meshSize;
    vtkm::worklet::contourtree_augmented::GetPointDimensions temp;
    temp(inDataSet.GetCellSet(), meshSize);
    dims[0] = meshSize[0];
    dims[1] = meshSize[1];
    dims[2] = meshSize[2];
    // TODO/FIXME: The following is commented out since it creates a a warning that
    // AsVirtual() will no longer be supported. Since this implementation is
    // incomplete anyway, it currently makes more sense to comment it out than
    // to fix the warning.
    // auto tempField = inDataSet.GetField("values").GetData();
    // values.resize(static_cast<std::size_t>(tempField.GetNumberOfValues()));
    // auto tempFieldHandle = tempField.AsVirtual<ValueType>().ReadPortal();
    // for (vtkm::Id i = 0; i < tempField.GetNumberOfValues(); i++)
    // {
    //   values[static_cast<std::size_t>(i)] = static_cast<ValueType>(tempFieldHandle.Get(i));
    // }
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "BOV reader not yet support in MPI mode by this example");
    MPI_Finalize();
    return EXIT_FAILURE;
#endif
  }
  else // Read ASCII data input
  {
    std::cout << "Reading ASCII file" << std::endl;
    std::ifstream inFile(filename);
    if (inFile.bad())
      return 0;

    // Read the dimensions of the mesh, i.e,. number of elementes in x, y, and z
    std::string line;
    getline(inFile, line);
    std::istringstream linestream(line);
    vtkm::Id dimVertices;
    while (linestream >> dimVertices)
    {
      dims.push_back(dimVertices);
    }

    // Compute the number of vertices, i.e., xdim * ydim * zdim
    nDims = static_cast<unsigned short>(dims.size());
    std::size_t numVertices = static_cast<std::size_t>(
      std::accumulate(dims.begin(), dims.end(), std::size_t(1), std::multiplies<std::size_t>()));

    // Check for fatal input errors
    // Check the the number of dimensiosn is either 2D or 3D
    bool invalidNumDimensions = (nDims < 2 || nDims > 3);
    // Log any errors if found on rank 0
    VTKM_LOG_IF_S(vtkm::cont::LogLevel::Error,
                  invalidNumDimensions && (rank == 0),
                  "The input mesh is " << nDims << "D. The input data must be either 2D or 3D.");
    // If we found any errors in the setttings than finalize MPI and exit the execution
    if (invalidNumDimensions)
    {
#ifdef WITH_MPI
      MPI_Finalize();
#endif
      return EXIT_SUCCESS;
    }

    // Read data
    values.resize(numVertices);
    for (std::size_t vertex = 0; vertex < numVertices; ++vertex)
    {
      inFile >> values[vertex];
    }

    // finish reading the data
    inFile.close();

    currTime = totalTime.GetElapsedTime();
    dataReadTime = currTime - prevTime;
    prevTime = currTime;

    // swap dims order
    std::swap(dims[0], dims[1]);

#ifndef WITH_MPI // We only need the inDataSet if are not using MPI otherwise we'll constructe a multi-block dataset
    // build the input dataset
    vtkm::cont::DataSetBuilderUniform dsb;
    // 2D data
    if (nDims == 2)
    {
      vtkm::Id2 vdims;
      vdims[0] = static_cast<vtkm::Id>(dims[0]);
      vdims[1] = static_cast<vtkm::Id>(dims[1]);
      inDataSet = dsb.Create(vdims);
    }
    // 3D data
    else
    {
      vtkm::Id3 vdims;
      vdims[0] = static_cast<vtkm::Id>(dims[0]);
      vdims[1] = static_cast<vtkm::Id>(dims[1]);
      vdims[2] = static_cast<vtkm::Id>(dims[2]);
      inDataSet = dsb.Create(vdims);
    }
    inDataSet.AddPointField("values", values);
#endif
  } // END ASCII Read

  // Print the mesh metadata
  if (rank == 0)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
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

  // If we found any errors in the setttings than finalize MPI and exit the execution
  if (invalidMCOption)
  {
#ifdef WITH_MPI
    MPI_Finalize();
#endif
    return EXIT_SUCCESS;
  }

#ifndef WITH_MPI                              // construct regular, single-block VTK-M input dataset
  vtkm::cont::DataSet useDataSet = inDataSet; // Single block dataset
#else  // Create a multi-block dataset for multi-block DIY-paralle processing
  vtkm::cont::PartitionedDataSet useDataSet; // Partitioned variant of the input dataset
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
                    "Number of ranks too large for data. Use " << lastDimSize / 2
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

      vtkm::cont::DataSetBuilderUniform dsb;
      vtkm::cont::DataSet ds;

      // 2D data
      if (nDims == 2)
      {
        vtkm::Id2 vdims;
        vdims[0] = static_cast<vtkm::Id>(dims[0]);
        vdims[1] = static_cast<vtkm::Id>(currBlockSize);
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
        vdims[0] = static_cast<vtkm::Id>(dims[1]);
        vdims[1] = static_cast<vtkm::Id>(dims[0]);
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

      ds.AddPointField("values", subValues);
      useDataSet.AppendPartition(ds);
    }
  }
#endif // WITH_MPI construct input dataset

  currTime = totalTime.GetElapsedTime();
  buildDatasetTime = currTime - prevTime;
  prevTime = currTime;

  // Convert the mesh of values into contour tree, pairs of vertex ids
  vtkm::filter::ContourTreeAugmented filter(useMarchingCubes, computeRegularStructure);

#ifdef WITH_MPI
  filter.SetSpatialDecomposition(
    blocksPerDim, globalSize, localBlockIndices, localBlockOrigins, localBlockSizes);
#endif
  filter.SetActiveField("values");

  // Execute the contour tree analysis. NOTE: If MPI is used the result  will be
  // a vtkm::cont::PartitionedDataSet instead of a vtkm::cont::DataSet
  auto result = filter.Execute(useDataSet);

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeContourTreeTime = currTime - prevTime;
  prevTime = currTime;

#ifdef WITH_MPI
#ifdef DEBUG_PRINT
  std::cout << std::flush;
  close(out);
  std::cerr << std::flush;
  close(err);

  dup2(save_out, fileno(stdout));
  dup2(save_err, fileno(stderr));

  close(save_out);
  close(save_err);
#endif
#endif

  ////////////////////////////////////////////
  // Compute the branch decomposition
  ////////////////////////////////////////////
  if (rank == 0 && computeBranchDecomposition && computeRegularStructure)
  {
    // Time branch decompostion
    vtkm::cont::Timer branchDecompTimer;
    branchDecompTimer.Start();
    // compute the volume for each hyperarc and superarc
    ctaug_ns::IdArrayType superarcIntrinsicWeight;
    ctaug_ns::IdArrayType superarcDependentWeight;
    ctaug_ns::IdArrayType supernodeTransferWeight;
    ctaug_ns::IdArrayType hyperarcDependentWeight;
    ctaug_ns::ProcessContourTree::ComputeVolumeWeightsSerial(filter.GetContourTree(),
                                                             filter.GetNumIterations(),
                                                             superarcIntrinsicWeight,  // (output)
                                                             superarcDependentWeight,  // (output)
                                                             supernodeTransferWeight,  // (output)
                                                             hyperarcDependentWeight); // (output)
    // Record the timings for the branch decomposition
    std::stringstream timingsStream; // Use a string stream to log in one message
    timingsStream << std::endl;
    timingsStream << "    --------------- Branch Decomposition Timings " << rank
                  << " --------------" << std::endl;
    timingsStream << "    " << std::setw(38) << std::left << "Compute Volume Weights"
                  << ": " << branchDecompTimer.GetElapsedTime() << " seconds" << std::endl;
    branchDecompTimer.Start();

    // compute the branch decomposition by volume
    ctaug_ns::IdArrayType whichBranch;
    ctaug_ns::IdArrayType branchMinimum;
    ctaug_ns::IdArrayType branchMaximum;
    ctaug_ns::IdArrayType branchSaddle;
    ctaug_ns::IdArrayType branchParent;
    ctaug_ns::ProcessContourTree::ComputeVolumeBranchDecompositionSerial(filter.GetContourTree(),
                                                                         superarcDependentWeight,
                                                                         superarcIntrinsicWeight,
                                                                         whichBranch,   // (output)
                                                                         branchMinimum, // (output)
                                                                         branchMaximum, // (output)
                                                                         branchSaddle,  // (output)
                                                                         branchParent); // (output)
    // Record and log the branch decompostion timings
    timingsStream << "    " << std::setw(38) << std::left << "Compute Volume Branch Decomposition"
                  << ": " << branchDecompTimer.GetElapsedTime() << " seconds" << std::endl;
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, timingsStream.str());

    //----main branch decompostion end
    //----Isovalue seleciton start
    if (numLevels > 0) // if compute isovalues
    {
// Get the data values for computing the explicit branch decomposition
#ifdef WITH_MPI
      vtkm::cont::ArrayHandle<ValueType> dataField;
      result.GetPartitions()[0].GetField(0).GetData().AsArrayHandle(dataField);
      bool dataFieldIsSorted = true;
#else
      vtkm::cont::ArrayHandle<ValueType> dataField;
      useDataSet.GetField(0).GetData().AsArrayHandle(dataField);
      bool dataFieldIsSorted = false;
#endif

      // create explicit representation of the branch decompostion from the array representation
      BranchType* branchDecompostionRoot =
        ctaug_ns::ProcessContourTree::ComputeBranchDecomposition<ValueType>(
          filter.GetContourTree().Superparents,
          filter.GetContourTree().Supernodes,
          whichBranch,
          branchMinimum,
          branchMaximum,
          branchSaddle,
          branchParent,
          filter.GetSortOrder(),
          dataField,
          dataFieldIsSorted);

#ifdef DEBUG_PRINT
      branchDecompostionRoot->PrintBranchDecomposition(std::cout);
#endif

      // Simplify the contour tree of the branch decompostion
      branchDecompostionRoot->SimplifyToSize(numComp, usePersistenceSorter);

      // Compute the relevant iso-values
      std::vector<ValueType> isoValues;
      switch (contourSelectMethod)
      {
        default:
        case 0:
        {
          branchDecompostionRoot->GetRelevantValues(static_cast<int>(contourType), eps, isoValues);
        }
        break;
        case 1:
        {
          vtkm::worklet::contourtree_augmented::process_contourtree_inc::PiecewiseLinearFunction<
            ValueType>
            plf;
          branchDecompostionRoot->AccumulateIntervals(static_cast<int>(contourType), eps, plf);
          isoValues = plf.nLargest(static_cast<unsigned int>(numLevels));
        }
        break;
      }

      // Print the compute iso values
      std::stringstream isoStream; // Use a string stream to log in one message
      isoStream << std::endl;
      isoStream << "    ------------------- Isovalue Suggestions --------------------" << std::endl;
      std::sort(isoValues.begin(), isoValues.end());
      isoStream << "    Isovalues: ";
      for (ValueType val : isoValues)
      {
        isoStream << val << " ";
      }
      isoStream << std::endl;
      // Unique isovalues
      std::vector<ValueType>::iterator it = std::unique(isoValues.begin(), isoValues.end());
      isoValues.resize(static_cast<std::size_t>(std::distance(isoValues.begin(), it)));
      isoStream << "    Unique Isovalues (" << isoValues.size() << "):";
      for (ValueType val : isoValues)
      {
        isoStream << val << " ";
      }
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, isoStream.str());
    } //end if compute isovalue
  }

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeBranchDecompTime = currTime - prevTime;
  prevTime = currTime;

  //vtkm::cont::Field resultField =  result.GetField();
  //vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;
  //resultField.GetData().AsArrayHandle(saddlePeak);

  // Dump out contour tree for comparison
  if (rank == 0 && printContourTree)
  {
    std::cout << "Contour Tree" << std::endl;
    std::cout << "============" << std::endl;
    ctaug_ns::EdgePairArray saddlePeak;
    ctaug_ns::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);
    ctaug_ns::PrintEdgePairArrayColumnLayout(saddlePeak, std::cout);
  }

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
               << std::setw(42) << std::left << "    Compute Branch Decomposition"
               << ": " << computeBranchDecompTime << " seconds" << std::endl
               << std::setw(42) << std::left << "    Total Time"
               << ": " << currTime << " seconds");

  const ctaug_ns::ContourTree& ct = filter.GetContourTree();
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             std::endl
               << "    ---------------- Contour Tree Array Sizes ---------------------" << std::endl
               << ct.PrintArraySizes());
  // Print hyperstructure statistics
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             std::endl
               << ct.PrintHyperStructureStatistics(false) << std::endl);

  // Flush ouput streams just to make sure everything has been logged (in particular when using MPI)
  std::cout << std::flush;
  std::cerr << std::flush;

#ifdef WITH_MPI
  // Let the next rank know that it is time to print their summary.
  if (rank < (size - 1))
  {
    int message = 1;
    MPI_Send(&message, 1, MPI_INT, (rank + 1), 0, comm);
  }
#endif

#ifdef WITH_MPI
  MPI_Finalize();
#endif
  return EXIT_SUCCESS;
}
