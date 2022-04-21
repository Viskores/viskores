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
//=======================================================================================
//
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// HierarchicalAugmenter.h
//
//=======================================================================================

#ifndef vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_hierarchical_augmenter_in_out_data_h
#define vtk_m_worklet_contourtree_distributed_hierarchical_augmenter_hierarchical_augmenter_in_out_data_h


#include <iostream> // std::cout
#include <sstream>  // std::stringstrea
#include <string>   // std::string
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
namespace hierarchical_augmenter
{

/// Class for storing input or output data for the HierarchicalAugmenter. The  data is factored out in this class to
/// allow for modular code and easue reuse, since the input and output require the same types of array parameters
template <typename FieldType>
class HierarchicalAugmenterInOutData
{ // class HierarchicalAugmenter
public:
  vtkm::worklet::contourtree_augmented::IdArrayType GlobalRegularIds;
  vtkm::cont::ArrayHandle<FieldType> DataValues;
  vtkm::worklet::contourtree_augmented::IdArrayType SupernodeIds;
  vtkm::worklet::contourtree_augmented::IdArrayType Superparents;
  vtkm::worklet::contourtree_augmented::IdArrayType SuperparentRounds;
  vtkm::worklet::contourtree_augmented::IdArrayType WhichRounds;

  /// empty constructor
  HierarchicalAugmenterInOutData() {}

  /// main constructor
  HierarchicalAugmenterInOutData(
    vtkm::worklet::contourtree_augmented::IdArrayType& globalRegularIds,
    vtkm::cont::ArrayHandle<FieldType>& dataValues,
    vtkm::worklet::contourtree_augmented::IdArrayType& supernodeIds,
    vtkm::worklet::contourtree_augmented::IdArrayType& superparents,
    vtkm::worklet::contourtree_augmented::IdArrayType& superparentRounds,
    vtkm::worklet::contourtree_augmented::IdArrayType& whichRounds)
    : GlobalRegularIds(globalRegularIds)
    , DataValues(dataValues)
    , SupernodeIds(supernodeIds)
    , Superparents(superparents)
    , SuperparentRounds(superparentRounds)
    , WhichRounds(whichRounds)
  {
  }

  /// Destructor
  ~HierarchicalAugmenterInOutData();

  /// Clear all arrays
  void ReleaseResources();

  /// Print contents fo this objects
  std::string DebugPrint(std::string message, const char* fileName, long lineNum);

}; // class HierarchicalAugmenterInOutData

template <typename FieldType>
HierarchicalAugmenterInOutData<FieldType>::~HierarchicalAugmenterInOutData()
{
  this->ReleaseResources();
}

// routine to release memory used for out arrays
template <typename FieldType>
void HierarchicalAugmenterInOutData<FieldType>::ReleaseResources()
{ // ReleaseResources()
  this->GlobalRegularIds.ReleaseResources();
  this->DataValues.ReleaseResources();
  this->SupernodeIds.ReleaseResources();
  this->Superparents.ReleaseResources();
  this->SuperparentRounds.ReleaseResources();
  this->WhichRounds.ReleaseResources();
} // ReleaseResources()

template <typename FieldType>
std::string HierarchicalAugmenterInOutData<FieldType>::DebugPrint(std::string message,
                                                                  const char* fileName,
                                                                  long lineNum)
{
  // DebugPrint()
  std::stringstream resultStream;
  resultStream << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << std::endl;
  resultStream << message << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Global Regular Ids", this->GlobalRegularIds, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintValues(
    "Data Values", this->DataValues, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Supernode Ids", this->SupernodeIds, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Superparents", this->Superparents, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Superparent Rounds", this->SuperparentRounds, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Which Rounds", this->WhichRounds, -1, resultStream);
  return resultStream.str();
}

} // namespace hierarchical_augmenter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

namespace vtkmdiy
{

// Struct to serialize ContourTreeMesh objects (i.e., load/save) needed in parralle for DIY
template <typename FieldType>
struct Serialization<vtkm::worklet::contourtree_distributed::hierarchical_augmenter::
                       HierarchicalAugmenterInOutData<FieldType>>
{
  static void save(vtkmdiy::BinaryBuffer& bb,
                   const vtkm::worklet::contourtree_distributed::hierarchical_augmenter::
                     HierarchicalAugmenterInOutData<FieldType>& ha)
  {
    vtkmdiy::save(bb, ha.GlobalRegularIds);
    vtkmdiy::save(bb, ha.DataValues);
    vtkmdiy::save(bb, ha.SupernodeIds);
    vtkmdiy::save(bb, ha.Superparents);
    vtkmdiy::save(bb, ha.SuperparentRounds);
    vtkmdiy::save(bb, ha.WhichRounds);
  }

  static void load(vtkmdiy::BinaryBuffer& bb,
                   vtkm::worklet::contourtree_distributed::hierarchical_augmenter::
                     HierarchicalAugmenterInOutData<FieldType>& ha)
  {
    vtkmdiy::load(bb, ha.GlobalRegularIds);
    vtkmdiy::load(bb, ha.DataValues);
    vtkmdiy::load(bb, ha.SupernodeIds);
    vtkmdiy::load(bb, ha.Superparents);
    vtkmdiy::load(bb, ha.SuperparentRounds);
    vtkmdiy::load(bb, ha.WhichRounds);
  }
};

} // namespace mangled_vtkmdiy_namespace

#endif
