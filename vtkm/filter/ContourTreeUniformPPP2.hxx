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

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/ContourTreeUniformPPP2.h>
#include <vtkm/worklet/contourtree_ppp2/ProcessContourTree.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ContourTreePPP2::ContourTreePPP2(bool useMarchingCubes,
                                 bool computeRegularStructure,
                                 vtkm::Id cellSetId)
  : mUseMarchingCubes(useMarchingCubes)
  , mComputeRegularStructure(computeRegularStructure)
  , mCellSetId(cellSetId)
  , mTimings()
{
  this->SetOutputFieldName("arcs");
}

template <typename Base, typename T>
inline bool instanceof (const T* ptr)
{
  return dynamic_cast<const Base*>(ptr) != nullptr;
}


const vtkm::worklet::contourtree_ppp2::ContourTree& ContourTreePPP2::GetContourTree() const
{
  return this->mContourTree;
}

const vtkm::worklet::contourtree_ppp2::IdArrayType& ContourTreePPP2::GetSortOrder() const
{
  return this->mSortOrder;
}

vtkm::Id ContourTreePPP2::GetNumIterations() const
{
  return this->nIterations;
}

const std::vector<std::pair<std::string, vtkm::Float64>>& ContourTreePPP2::GetTimings() const
{
  return this->mTimings;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
vtkm::cont::DataSet ContourTreePPP2::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  // Start the timer
  vtkm::cont::Timer<DeviceAdapter> timer;
  mTimings.clear();

  // Check that the field is Ok
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  //vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;
  // Create the worklet
  vtkm::worklet::ContourTreePPP2 worklet;
  vtkm::Id nRows;
  vtkm::Id nCols;
  vtkm::Id nSlices = 1;

  vtkm::cont::DynamicCellSet temp = input.GetCellSet(mCellSetId);
  // Collect sizing information from the dataset
  if (temp.IsType<vtkm::cont::CellSetStructured<2>>()) // Check if we have a 2D cell set
  {
    // TODO print warning if mUseMarchingCubes is set to True for 2D data to indicate that the flag is ignored
    vtkm::cont::CellSetStructured<2> cellSet;
    input.GetCellSet(mCellSetId)
      .CopyTo(
        cellSet); // TODO should the ID of the cell-set we use be an input parameter to ContourTreePPP2?
    // How should policy be used?
    vtkm::filter::ApplyPolicy(cellSet, policy);
    vtkm::Id2 pointDimensions = cellSet.GetPointDimensions();
    nRows = pointDimensions[0];
    nCols = pointDimensions[1];
    nSlices = 1;
  }
  else if (temp.IsType<vtkm::cont::CellSetStructured<3>>()) // Check if we have a 3D cell set
  {
    vtkm::cont::CellSetStructured<3> cellSet;
    input.GetCellSet(mCellSetId)
      .CopyTo(cellSet); // TODO see above. cell-set ID always 0 or user-defined
    // How should policy be used?
    vtkm::filter::ApplyPolicy(cellSet, policy);
    vtkm::Id3 pointDimensions = cellSet.GetPointDimensions();
    nRows = pointDimensions[0];
    nCols = pointDimensions[1];
    nSlices = pointDimensions[2];
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("Expected 2D or 3D structured cell cet! ");
  }

  //vtkm::Float64 startupTime = timer.GetElapsedTime();
  //std::cout<<"Time to prep for worklet call" <<startupTime<<std::endl;

  // Run the worklet
  worklet.Run(field,
              mTimings,
              mContourTree,
              mSortOrder,
              nIterations,
              device,
              nRows,
              nCols,
              nSlices,
              mUseMarchingCubes,
              mComputeRegularStructure);

  // Compute the saddle peak dataset for return
  // ProcessContourTree::CollectSortedSuperarcs<DeviceAdapter>(mContourTree, mSortOrder, saddlePeak);

  // Create the vtkm result object
  auto result = internal::CreateResult(input,
                                       mContourTree.arcs,
                                       this->GetOutputFieldName(),
                                       fieldMeta.GetAssociation(),
                                       fieldMeta.GetCellSetName());

  // Update the total timings
  vtkm::Float64 totalTimeWorklet = 0;
  for (std::vector<std::pair<std::string, vtkm::Float64>>::size_type i = 0; i < mTimings.size();
       i++)
    totalTimeWorklet += mTimings[i].second;
  //std::cout<<"Total time measured by worklet: "<<totalTimeWorklet<<std::endl;
  mTimings.push_back(std::pair<std::string, vtkm::Float64>(
    "Others (ContourTreePPP2 Filter): ", timer.GetElapsedTime() - totalTimeWorklet));

  // Return the result
  return result;
} // ContourTreePPP2::DoExecute


} // namespace filter
} // namespace vtkm::filter
