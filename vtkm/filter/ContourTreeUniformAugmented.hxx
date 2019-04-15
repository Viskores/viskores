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

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/filter/ContourTreeUniformAugmented.h>
#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/ContourTreeUniformAugmented.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ContourTreePPP2::ContourTreePPP2(bool useMarchingCubes, bool computeRegularStructure)
  : UseMarchingCubes(useMarchingCubes)
  , ComputeRegularStructure(computeRegularStructure)
  , Timings()
{
  this->SetOutputFieldName("arcs");
}

const vtkm::worklet::contourtree_augmented::ContourTree& ContourTreePPP2::GetContourTree() const
{
  return this->ContourTreeData;
}

const vtkm::worklet::contourtree_augmented::IdArrayType& ContourTreePPP2::GetSortOrder() const
{
  return this->MeshSortOrder;
}

vtkm::Id ContourTreePPP2::GetNumIterations() const
{
  return this->NumIterations;
}

const std::vector<std::pair<std::string, vtkm::Float64>>& ContourTreePPP2::GetTimings() const
{
  return this->Timings;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
vtkm::cont::DataSet ContourTreePPP2::DoExecute(const vtkm::cont::DataSet& input,
                                               const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                               const vtkm::filter::FieldMetadata& fieldMeta,
                                               vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // TODO: This should be switched to use the logging macros defined in vtkm/cont/logging.h
  // Start the timer
  vtkm::cont::Timer timer;
  timer.Start();
  Timings.clear();

  // Check that the field is Ok
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  // Use the GetRowsColsSlices struct defined in the header to collect the nRows, nCols, and nSlices information
  vtkm::worklet::ContourTreePPP2 worklet;
  vtkm::Id nRows;
  vtkm::Id nCols;
  vtkm::Id nSlices = 1;
  const auto& cells = input.GetCellSet(this->GetActiveCoordinateSystemIndex());
  vtkm::filter::ApplyPolicy(cells, policy).CastAndCall(GetRowsColsSlices(), nRows, nCols, nSlices);

  // Run the worklet
  worklet.Run(field,
              this->Timings,
              this->ContourTreeData,
              this->MeshSortOrder,
              this->NumIterations,
              nRows,
              nCols,
              nSlices,
              this->UseMarchingCubes,
              this->ComputeRegularStructure);

  // Compute the saddle peak dataset for return
  // vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;
  // ProcessContourTree::CollectSortedSuperarcs<DeviceAdapter>(ContourTreeData, MeshSortOrder, saddlePeak);

  // Create the vtkm result object
  auto result = internal::CreateResult(input,
                                       ContourTreeData.arcs,
                                       this->GetOutputFieldName(),
                                       fieldMeta.GetAssociation(),
                                       fieldMeta.GetCellSetName());

  // Update the total timings
  vtkm::Float64 totalTimeWorklet = 0;
  for (std::vector<std::pair<std::string, vtkm::Float64>>::size_type i = 0; i < Timings.size(); i++)
    totalTimeWorklet += Timings[i].second;
  std::cout << "Total time measured by worklet: " << totalTimeWorklet << std::endl;
  Timings.push_back(std::pair<std::string, vtkm::Float64>(
    "Others (ContourTreePPP2 Filter): ", timer.GetElapsedTime() - totalTimeWorklet));

  // Return the result
  return result;
} // ContourTreePPP2::DoExecute


} // namespace filter
} // namespace vtkm::filter
