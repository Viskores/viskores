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

#include <vtkm/filter/ContourTreeUniformDistributed.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/ProcessContourTree.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_distributed/TreeCompiler.h>

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
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

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
  bool useBoundaryExtremaOnly = true;
  if (parser.hasOption("--useFullBoundary"))
  {
    useBoundaryExtremaOnly = false;
  }
  bool useMarchingCubes = false;
  if (parser.hasOption("--mc"))
  {
    useMarchingCubes = true;
    if (useBoundaryExtremaOnly)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "Warning: Marching cubes connectivity currently only works when "
                 "using full boundary. Enabling the --useFullBoundary option "
                 "to ensure that the app works.");
      useBoundaryExtremaOnly = false;
    }
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
  bool saveTreeCompilerData = false;
  if (parser.hasOption("--saveTreeCompilerData"))
  {
    saveTreeCompilerData = true;
  }
  bool forwardSummary = false;
  if (parser.hasOption("--forwardSummary"))
  {
    forwardSummary = true;
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

  if (argc < 2 || parser.hasOption("--help") || parser.hasOption("-h"))
  {
    if (rank == 0)
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
      std::cout << "--mc             Use marching cubes connectivity (Default=False)." << std::endl;
      std::cout << "--useFullBoundary Use the full boundary during. Typically only useful"
                << std::endl;
      std::cout << "                 to compare the performance between using the full boundary"
                << std::endl;
      std::cout << "                 and when using only boundary extrema." << std::endl;
      std::cout << "--preSplitFiles  Input data is already pre-split into blocks." << std::endl;
      std::cout << "--saveDot        Save DOT files of the distributed contour tree "
                << "computation (Default=False). " << std::endl;
      std::cout << "--saveTreeCompilerData  Save data files needed for the tree compiler"
                << std::endl;
#ifdef ENABLE_SET_NUM_THREADS
      std::cout << "--numThreads     Specifiy the number of threads to use. "
                << "Available only with TBB." << std::endl;
#endif
      std::cout << "--numBlocks      Number of blocks to use during computation "
                << "(Default=number of MPI ranks.)" << std::endl;
      std::cout << "--forwardSummary Forward the summary timings also to the per-rank "
                << "log files. Default is to round-robin print the summary instead" << std::endl;
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
                 << "    saveTreeCompilerData=" << saveTreeCompilerData << std::endl
                 << "    forwardSummary=" << forwardSummary << std::endl
#ifdef ENABLE_SET_NUM_THREADS
                 << "    numThreads=" << numThreads << std::endl
#endif
                 << "    nblocks=" << numBlocks << std::endl);
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
  vtkm::cont::ArrayHandle<vtkm::Id3> localBlockOrigins;
  vtkm::cont::ArrayHandle<vtkm::Id3> localBlockSizes;
  localBlockIndices.Allocate(blocksPerRank);
  localBlockOrigins.Allocate(blocksPerRank);
  localBlockSizes.Allocate(blocksPerRank);
  auto localBlockIndicesPortal = localBlockIndices.WritePortal();
  auto localBlockOriginsPortal = localBlockOrigins.WritePortal();
  auto localBlockSizesPortal = localBlockSizes.WritePortal();

  // Read the pre-split data files
  if (preSplitFiles)
  {
    for (int blockNo = 0; blockNo < blocksPerRank; ++blockNo)
    {
      // Translate pattern into filename for this block
      char block_filename[256];
      snprintf(block_filename,
               sizeof(block_filename),
               filename.c_str(),
               static_cast<int>(rank * blocksPerRank + blockNo));
      std::cout << "Reading file " << block_filename << std::endl;

      // Open file
      std::ifstream inFile(block_filename);
      if (!inFile.is_open() || inFile.bad())
      {
        std::cerr << "Error: Couldn't open file " << block_filename << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
      }

      // Read header with dimensions
      std::string line;
      std::string tag;
      vtkm::Id dimVertices;

      getline(inFile, line);
      std::istringstream global_extents_stream(line);
      global_extents_stream >> tag;
      if (tag != "#GLOBAL_EXTENTS")
      {
        std::cerr << "Error: Expected #GLOBAL_EXTENTS, got " << tag << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
      }

      std::vector<vtkm::Id> global_extents;
      while (global_extents_stream >> dimVertices)
        global_extents.push_back(dimVertices);

      // Swap dimensions so that they are from fastest to slowest growing
      // dims[0] -> col; dims[1] -> row, dims[2] ->slice
      std::swap(global_extents[0], global_extents[1]);

      if (blockNo == 0)
      { // First block: Set globalSize
        globalSize =
          vtkm::Id3{ static_cast<vtkm::Id>(global_extents[0]),
                     static_cast<vtkm::Id>(global_extents[1]),
                     static_cast<vtkm::Id>(global_extents.size() > 2 ? global_extents[2] : 1) };
      }
      else
      { // All other blocks: Consistency check of globalSize
        if (globalSize !=
            vtkm::Id3{ static_cast<vtkm::Id>(global_extents[0]),
                       static_cast<vtkm::Id>(global_extents[1]),
                       static_cast<vtkm::Id>(global_extents.size() > 2 ? global_extents[2] : 1) })
        {
          std::cerr << "Error: Global extents mismatch between blocks!" << std::endl;
          MPI_Finalize();
          return EXIT_FAILURE;
        }
      }

      getline(inFile, line);
      std::istringstream offset_stream(line);
      offset_stream >> tag;
      if (tag != "#OFFSET")
      {
        std::cerr << "Error: Expected #OFFSET, got " << tag << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
      }
      std::vector<vtkm::Id> offset;
      while (offset_stream >> dimVertices)
        offset.push_back(dimVertices);
      // Swap dimensions so that they are from fastest to slowest growing
      // dims[0] -> col; dims[1] -> row, dims[2] ->slice
      std::swap(offset[0], offset[1]);

      getline(inFile, line);
      std::istringstream bpd_stream(line);
      bpd_stream >> tag;
      if (tag != "#BLOCKS_PER_DIM")
      {
        std::cerr << "Error: Expected #BLOCKS_PER_DIM, got " << tag << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
      }
      std::vector<vtkm::Id> bpd;
      while (bpd_stream >> dimVertices)
        bpd.push_back(dimVertices);
      // Swap dimensions so that they are from fastest to slowest growing
      // dims[0] -> col; dims[1] -> row, dims[2] ->slice
      std::swap(bpd[0], bpd[1]);

      getline(inFile, line);
      std::istringstream blockIndex_stream(line);
      blockIndex_stream >> tag;
      if (tag != "#BLOCK_INDEX")
      {
        std::cerr << "Error: Expected #BLOCK_INDEX, got " << tag << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
      }
      std::vector<vtkm::Id> blockIndex;
      while (blockIndex_stream >> dimVertices)
        blockIndex.push_back(dimVertices);
      // Swap dimensions so that they are from fastest to slowest growing
      // dims[0] -> col; dims[1] -> row, dims[2] ->slice
      std::swap(blockIndex[0], blockIndex[1]);

      getline(inFile, line);
      std::istringstream linestream(line);
      std::vector<vtkm::Id> dims;
      while (linestream >> dimVertices)
      {
        dims.push_back(dimVertices);
      }

      if (dims.size() != global_extents.size() || dims.size() != offset.size())
      {
        std::cerr << "Error: Dimension mismatch" << std::endl;
        MPI_Finalize();
        return EXIT_FAILURE;
      }
      // Swap dimensions so that they are from fastest to slowest growing
      // dims[0] -> col; dims[1] -> row, dims[2] ->slice
      std::swap(dims[0], dims[1]);

      // Compute the number of vertices, i.e., xdim * ydim * zdim
      nDims = static_cast<unsigned short>(dims.size());
      std::size_t numVertices = static_cast<std::size_t>(
        std::accumulate(dims.begin(), dims.end(), std::size_t(1), std::multiplies<std::size_t>()));

      // Check for fatal input errors
      // Check that the number of dimensiosn is either 2D or 3D
      bool invalidNumDimensions = (nDims < 2 || nDims > 3);
      // Log any errors if found on rank 0
      VTKM_LOG_IF_S(vtkm::cont::LogLevel::Error,
                    invalidNumDimensions && (rank == 0),
                    "The input mesh is " << nDims
                                         << "D. "
                                            "The input data must be either 2D or 3D.");

      // If we found any errors in the setttings than finalize MPI and exit the execution
      if (invalidNumDimensions)
      {
        MPI_Finalize();
        return EXIT_FAILURE;
      }

      // Read data
      using ValueType = vtkm::Float64;
      std::vector<ValueType> values(numVertices);
      if (filename.compare(filename.length() - 5, 5, ".bdem") == 0)
      {
        inFile.read(reinterpret_cast<char*>(values.data()),
                    static_cast<std::streamsize>(numVertices * sizeof(ValueType)));
      }
      else
      {
        for (std::size_t vertex = 0; vertex < numVertices; ++vertex)
        {
          inFile >> values[vertex];
        }
      }

      currTime = totalTime.GetElapsedTime();
      dataReadTime = currTime - prevTime;
      prevTime = currTime;

      // Create vtk-m data set
      vtkm::cont::DataSetBuilderUniform dsb;
      vtkm::cont::DataSet ds;
      if (nDims == 2)
      {
        const vtkm::Id2 v_dims{
          static_cast<vtkm::Id>(dims[0]),
          static_cast<vtkm::Id>(dims[1]),
        };
        const vtkm::Vec<ValueType, 2> v_origin{ static_cast<ValueType>(offset[0]),
                                                static_cast<ValueType>(offset[1]) };
        const vtkm::Vec<ValueType, 2> v_spacing{ 1, 1 };
        ds = dsb.Create(v_dims, v_origin, v_spacing);
      }
      else
      {
        VTKM_ASSERT(nDims == 3);
        const vtkm::Id3 v_dims{ static_cast<vtkm::Id>(dims[0]),
                                static_cast<vtkm::Id>(dims[1]),
                                static_cast<vtkm::Id>(dims[2]) };
        const vtkm::Vec<ValueType, 3> v_origin{ static_cast<ValueType>(offset[0]),
                                                static_cast<ValueType>(offset[1]),
                                                static_cast<ValueType>(offset[2]) };
        vtkm::Vec<ValueType, 3> v_spacing(1, 1, 1);
        ds = dsb.Create(v_dims, v_origin, v_spacing);
      }
      ds.AddPointField("values", values);
      // and add to partition
      useDataSet.AppendPartition(ds);

      localBlockIndicesPortal.Set(
        blockNo,
        vtkm::Id3{ static_cast<vtkm::Id>(blockIndex[0]),
                   static_cast<vtkm::Id>(blockIndex[1]),
                   static_cast<vtkm::Id>(nDims == 3 ? blockIndex[2] : 0) });
      localBlockOriginsPortal.Set(blockNo,
                                  vtkm::Id3{ static_cast<vtkm::Id>(offset[0]),
                                             static_cast<vtkm::Id>(offset[1]),
                                             static_cast<vtkm::Id>(nDims == 3 ? offset[2] : 0) });
      localBlockSizesPortal.Set(blockNo,
                                vtkm::Id3{ static_cast<vtkm::Id>(dims[0]),
                                           static_cast<vtkm::Id>(dims[1]),
                                           static_cast<vtkm::Id>(nDims == 3 ? dims[2] : 0) });

      if (blockNo == 0)
      {
        blocksPerDim = vtkm::Id3{ static_cast<vtkm::Id>(bpd[0]),
                                  static_cast<vtkm::Id>(bpd[1]),
                                  static_cast<vtkm::Id>(nDims == 3 ? bpd[2] : 1) };
      }
    }

    // Print the mesh metadata
    if (rank == 0)
    {
      VTKM_LOG_S(exampleLogLevel,
                 std::endl
                   << "    ---------------- Input Mesh Properties --------------" << std::endl
                   << "    Number of dimensions: " << nDims << std::endl);
    }
  }
  // Read single-block data and split it for the ranks
  else
  {
    vtkm::cont::DataSet inDataSet;
    // Currently FloatDefualt would be fine, but it could cause problems if we ever
    // read binary files here.
    using ValueType = vtkm::Float64;
    std::vector<ValueType> values;
    std::vector<vtkm::Id> dims;

    // Read BOV data file
    if (filename.compare(filename.length() - 3, 3, "bov") == 0)
    {
      std::cout << "Reading BOV file" << std::endl;
      vtkm::io::BOVDataSetReader reader(filename);
      inDataSet = reader.ReadDataSet();
      nDims = 3;
      currTime = totalTime.GetElapsedTime();
      dataReadTime = currTime - prevTime;
      prevTime = currTime;
      // Copy the data into the values array so we can construct a multiblock dataset
      // TODO All we should need to do to implement BOV support is to copy the values
      // in the values vector and copy the dimensions in the dims vector
      vtkm::Id3 pointDimensions;
      auto cellSet = inDataSet.GetCellSet();
      cellSet.CastAndCall(vtkm::worklet::contourtree_augmented::GetPointDimensions(),
                          pointDimensions);
      std::cout << "Point dimensions are " << pointDimensions << std::endl;
      dims.resize(3);
      dims[0] = pointDimensions[0];
      dims[1] = pointDimensions[1];
      dims[2] = pointDimensions[2];
      auto tempFieldData = inDataSet.GetField(0).GetData();
      values.resize(static_cast<std::size_t>(tempFieldData.GetNumberOfValues()));
      auto valuesHandle = vtkm::cont::make_ArrayHandle(values, vtkm::CopyFlag::Off);
      vtkm::cont::ArrayCopy(tempFieldData, valuesHandle);
      valuesHandle.SyncControlArray(); //Forces values to get updated if copy happened on GPU
    }
    // Read ASCII data input
    else
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
      // Swap dimensions so that they are from fastest to slowest growing
      // dims[0] -> col; dims[1] -> row, dims[2] ->slice
      std::swap(dims[0], dims[1]);

      // Compute the number of vertices, i.e., xdim * ydim * zdim
      nDims = static_cast<unsigned short>(dims.size());
      std::size_t numVertices = static_cast<std::size_t>(
        std::accumulate(dims.begin(), dims.end(), std::size_t(1), std::multiplies<std::size_t>()));

      // Check the the number of dimensiosn is either 2D or 3D
      bool invalidNumDimensions = (nDims < 2 || nDims > 3);
      // Log any errors if found on rank 0
      VTKM_LOG_IF_S(vtkm::cont::LogLevel::Error,
                    invalidNumDimensions && (rank == 0),
                    "The input mesh is " << nDims << "D. The input data must be either 2D or 3D.");
      // If we found any errors in the setttings than finalize MPI and exit the execution
      if (invalidNumDimensions)
      {
        MPI_Finalize();
        return EXIT_FAILURE;
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

    } // END ASCII Read

    // Print the mesh metadata
    if (rank == 0)
    {
      VTKM_LOG_S(exampleLogLevel,
                 std::endl
                   << "    ---------------- Input Mesh Properties --------------" << std::endl
                   << "    Number of dimensions: " << nDims);
    }

    // Create a multi-block dataset for multi-block DIY-paralle processing
    blocksPerDim = nDims == 3 ? vtkm::Id3(1, 1, numBlocks)
                              : vtkm::Id3(1, numBlocks, 1); // Decompose the data into
    globalSize = nDims == 3 ? vtkm::Id3(static_cast<vtkm::Id>(dims[0]),
                                        static_cast<vtkm::Id>(dims[1]),
                                        static_cast<vtkm::Id>(dims[2]))
                            : vtkm::Id3(static_cast<vtkm::Id>(dims[0]),
                                        static_cast<vtkm::Id>(dims[1]),
                                        static_cast<vtkm::Id>(1));
    std::cout << blocksPerDim << " " << globalSize << std::endl;
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

          localBlockIndicesPortal.Set(localBlockIndex, vtkm::Id3(0, blockIndex, 0));
          localBlockOriginsPortal.Set(localBlockIndex,
                                      vtkm::Id3(0, (blockStart / blockSliceSize), 0));
          localBlockSizesPortal.Set(localBlockIndex,
                                    vtkm::Id3(static_cast<vtkm::Id>(dims[0]), currBlockSize, 0));
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
          localBlockSizesPortal.Set(localBlockIndex,
                                    vtkm::Id3(static_cast<vtkm::Id>(dims[0]),
                                              static_cast<vtkm::Id>(dims[1]),
                                              currBlockSize));
        }

        std::vector<vtkm::Float32> subValues((values.begin() + blockStart),
                                             (values.begin() + blockEnd));

        ds.AddPointField("values", subValues);
        useDataSet.AppendPartition(ds);
      }
    }
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
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  currTime = totalTime.GetElapsedTime();
  buildDatasetTime = currTime - prevTime;
  prevTime = currTime;

  // Make sure that all ranks have started up before we start the data read
  MPI_Barrier(comm);
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 dataReadSyncTime = currTime - prevTime;
  prevTime = currTime;

  // Convert the mesh of values into contour tree, pairs of vertex ids
  vtkm::filter::ContourTreeUniformDistributed filter(blocksPerDim,
                                                     globalSize,
                                                     localBlockIndices,
                                                     localBlockOrigins,
                                                     localBlockSizes,
                                                     useBoundaryExtremaOnly,
                                                     useMarchingCubes,
                                                     saveDotFiles,
                                                     timingsLogLevel,
                                                     treeLogLevel);
  filter.SetActiveField("values");

  // Execute the contour tree analysis
  auto result = filter.Execute(useDataSet);

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeContourTreeTime = currTime - prevTime;
  prevTime = currTime;

  // Make sure that all ranks have started up before we start the data read
  MPI_Barrier(comm);
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 postFilterSyncTime = currTime - prevTime;
  prevTime = currTime;

  /*
  std::cout << "Result dataset has " << result.GetNumberOfPartitions() << " partitions" << std::endl;

  for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
  {
    auto ds = result.GetPartition(ds_no);
    for (vtkm::Id f_no = 0; f_no < ds.GetNumberOfFields(); ++f_no)
    {
      auto field = ds.GetField(f_no);
      std::cout << field.GetName() << ": ";
      PrintArrayContents(field.GetData());
      std::cout << std::endl;
    }
  }
  */

  if (saveTreeCompilerData)
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

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 saveTreeCompilerDataTime = currTime - prevTime;
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
               << std::setw(42) << std::left << "    Save Tree Compiler Data"
               << ": " << saveTreeCompilerDataTime << " seconds" << std::endl
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
