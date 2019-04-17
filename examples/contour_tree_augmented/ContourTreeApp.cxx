//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

#define DEBUG_TIMING

#ifdef ENABLE_SET_NUM_THREADS
#include "tbb/task_scheduler_init.h"
#endif

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/filter/ContourTreeUniformAugmented.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/ProcessContourTree.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace cppp2_ns = vtkm::worklet::contourtree_augmented;

// Simple helper class for parsing the command line options
class ParseCL
{
public:
  ParseCL() {}

  void parse(std::vector<std::string>::size_type argc, char** argv)
  {
    mCLOptions.resize(std::vector<std::string>::size_type(argc));
    for (std::vector<std::string>::size_type i = 1; i < argc; ++i)
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
    vtkm::Id index = this->findOption(option);
    if (index >= 0)
    {
      std::string val = this->mCLOptions[std::vector<std::string>::size_type(index)];
      auto valPos = val.find("=");
      if (valPos)
      {
        return val.substr(valPos + 1);
      }
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
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  vtkm::cont::Timer totalTime;
  totalTime.Start();
  vtkm::Float64 prevTime = 0;
  vtkm::Float64 currTime = 0;

  ////////////////////////////////////////////
  // Parse the command line options
  ////////////////////////////////////////////
  ParseCL parser;
  parser.parse(std::vector<std::string>::size_type(argc), argv);
  std::string filename = parser.getOptions().back();
  bool computeRegularStructure = true;
  bool useMarchingCubes = false;
  bool computeBranchDecomposition = true;
  bool printContourTree = false;
  if (parser.hasOption("--augmentTree"))
    computeRegularStructure = std::stoi(parser.getOption("--augmentTree"));
  if (parser.hasOption("--mc"))
    useMarchingCubes = true;
  if (parser.hasOption("--printCT"))
    printContourTree = true;
  if (parser.hasOption("--branchDecomp"))
    computeBranchDecomposition = std::stoi(parser.getOption("--branchDecomp"));
#ifdef ENABLE_SET_NUM_THREADS
  int numThreads = tbb::task_scheduler_init::default_num_threads();
  if (parser.hasOption("--numThreads"))
    numThreads = std::stoi(parser.getOption("--numThreads"));
  tbb::task_scheduler_init schedulerInit(numThreads);
#endif

  if (argc < 2 || parser.hasOption("--help") || parser.hasOption("-h"))
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
    std::cout << "--augmentTree     Compute the augmented contour tree. (Default=True)"
              << std::endl;
    std::cout << "--branchDecomp    Compute the volume branch decomposition for the contour tree. "
                 "Requires --augmentTree (Default=True)"
              << std::endl;
    std::cout << "--printCT         Print the contour tree. (Default=False)" << std::endl;
#ifdef ENABLE_SET_NUM_THREADS
    std::cout << "--numThreads      Specify the number of threads to use. Available only with TBB."
              << std::endl;
#endif
    std::cout << config.Usage << std::endl;
    return 0;
  }

  std::cout << "ContourTree <options> <fileName>" << std::endl;
  std::cout << "Settings:" << std::endl;
  std::cout << "    filename=" << filename << std::endl;
  std::cout << "    mc=" << useMarchingCubes << std::endl;
  std::cout << "    augmentTree=" << computeRegularStructure << std::endl;
  std::cout << "    branchDecomp=" << computeBranchDecomposition << std::endl;
#ifdef ENABLE_SET_NUM_THREADS
  std::cout << "    numThreads=" << numThreads << std::endl;
#endif
  std::cout << config.Usage << std::endl;
  std::cout << std::endl;


  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 startUpTime = currTime - prevTime;
  prevTime = currTime;

  ///////////////////////////////////////////////
  // Read the input data
  ///////////////////////////////////////////////
  std::ifstream inFile(filename);
  if (inFile.bad())
    return 0;

  // Read the dimensions of the mesh, i.e,. number of elementes in x, y, and z
  std::vector<vtkm::Id> dims;
  std::string line;
  getline(inFile, line);
  std::istringstream linestream(line);
  vtkm::Id dimVertices;
  while (linestream >> dimVertices)
  {
    dims.push_back(dimVertices);
  }

  // Compute the number of vertices, i.e., xdim * ydim * zdim
  auto nDims = dims.size();
  std::vector<vtkm::Float32>::size_type nVertices = std::vector<vtkm::Float32>::size_type(
    std::accumulate(dims.begin(), dims.end(), vtkm::Id(1), std::multiplies<vtkm::Id>()));

  // Print the mesh metadata
  std::cout << "Number of dimensions: " << nDims << std::endl;
  std::cout << "Number of mesh vertices: " << nVertices << std::endl;

  // Check the the number of dimensiosn is either 2D or 3D
  if (nDims < 2 || nDims > 3)
  {
    std::cout << "The input mesh is " << nDims << "D. Input data must be either 2D or 3D."
              << std::endl;
    return 0;
  }
  if (useMarchingCubes && nDims != 3)
  {
    std::cout << "The input mesh is " << nDims
              << "D. Contour tree using marching cubes only supported for 3D data." << std::endl;
    return 0;
  }

  // read data
  std::vector<vtkm::Float32> values(nVertices);
  for (std::vector<vtkm::Float32>::size_type vertex = 0; vertex < nVertices; vertex++)
  {
    inFile >> values[vertex];
  }

  // finish reading the data
  inFile.close();

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 dataReadTime = currTime - prevTime;
  prevTime = currTime;
  // build the input dataset
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::cont::DataSet inDataSet;
  // 2D data
  if (nDims == 2)
  {
    vtkm::Id2 vdims;
    vdims[0] = dims[0];
    vdims[1] = dims[1];
    inDataSet = dsb.Create(vdims);
  }
  // 3D data
  else
  {
    vtkm::Id3 vdims;
    vdims[0] = dims[0];
    vdims[1] = dims[1];
    vdims[2] = dims[2];
    inDataSet = dsb.Create(vdims);
  }
  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(inDataSet, "values", values);

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 buildDatasetTime = currTime - prevTime;
  prevTime = currTime;

  // Output data set is pairs of saddle and peak vertex IDs
  vtkm::cont::DataSet result;

  // Convert the mesh of values into contour tree, pairs of vertex ids
  vtkm::filter::ContourTreePPP2 filter(useMarchingCubes, computeRegularStructure);
  filter.SetActiveField("values");
  result = filter.Execute(inDataSet); //, std::string("values"));

  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeContourTreeTime = currTime - prevTime;
  prevTime = currTime;

#ifdef DEBUG_TIMING
  std::cout << "-------------------------------------------------------------" << std::endl;
  std::cout << "-------------------Contour Tree Timings----------------------" << std::endl;
  // Get the timings from the contour tree computation
  const std::vector<std::pair<std::string, vtkm::Float64>>& contourTreeTimings =
    filter.GetTimings();
  for (std::vector<std::pair<std::string, vtkm::Float64>>::size_type i = 0;
       i < contourTreeTimings.size();
       i++)
    std::cout << std::setw(42) << std::left << contourTreeTimings[i].first << ": "
              << contourTreeTimings[i].second << " seconds" << std::endl;
#endif

  ////////////////////////////////////////////
  // Compute the branch decomposition
  ////////////////////////////////////////////
  if (computeBranchDecomposition)
  {
    // TODO: Change timing to use logging in vtkm/cont/Logging.h
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
    branchDecompTimer.Reset();
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
  }
  currTime = totalTime.GetElapsedTime();
  vtkm::Float64 computeBranchDecompTime = currTime - prevTime;
  prevTime = currTime;

  //vtkm::cont::Field resultField =  result.GetField();
  //vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;
  //resultField.GetData().CopyTo(saddlePeak);

  // dump out contour tree for comparison
  if (printContourTree)
  {
    std::cout << "Contour Tree" << std::endl;
    std::cout << "============" << std::endl;
    cppp2_ns::EdgePairArray saddlePeak;
    cppp2_ns::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);
    cppp2_ns::printEdgePairArray(saddlePeak);
  }

#ifdef DEBUG_TIMING
  std::cout << "-------------------------------------------------------------" << std::endl;
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
  //if (computeBranchDecomposition) miscTime -= computeBranchDecompTime;
  //std::cout<<std::setw(42)<<std::left<<"Misc. Times"<<": "<<miscTime<<" seconds"<<std::endl;
  std::cout << std::setw(42) << std::left << "Total Time"
            << ": " << currTime << " seconds" << std::endl;

  std::cout << "-------------------------------------------------------------" << std::endl;
  std::cout << "----------------Contour Tree Array Sizes---------------------" << std::endl;
  const vtkm::worklet::contourtree_augmented::ContourTree& ct = filter.GetContourTree();
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

#endif

  return 0;
}
