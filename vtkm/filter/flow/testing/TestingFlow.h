//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_testing_TestingFlow_h
#define vtk_m_filter_flow_testing_TestingFlow_h

#include <vtkm/Particle.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <vector>

enum FilterType
{
  PARTICLE_ADVECTION,
  STREAMLINE,
  PATHLINE
};

vtkm::cont::ArrayHandle<vtkm::Vec3f> CreateConstantVectorField(vtkm::Id num,
                                                               const vtkm::Vec3f& vec);

void AddVectorFields(vtkm::cont::PartitionedDataSet& pds,
                     const std::string& fieldName,
                     const vtkm::Vec3f& vec);

std::vector<vtkm::cont::PartitionedDataSet> CreateAllDataSetBounds(vtkm::Id nPerRank,
                                                                   bool useGhost);

std::vector<vtkm::Range> ExtractMaxXRanges(const vtkm::cont::PartitionedDataSet& pds,
                                           bool useGhost);

template <typename FilterType>
void SetFilter(FilterType& filter,
               vtkm::FloatDefault stepSize,
               vtkm::Id numSteps,
               const std::string& fieldName,
               vtkm::cont::ArrayHandle<vtkm::Particle> seedArray,
               bool useThreaded,
               bool useAsyncComm,
               bool useBlockIds,
               const std::vector<vtkm::Id>& blockIds)
{
  filter.SetStepSize(stepSize);
  filter.SetNumberOfSteps(numSteps);
  filter.SetSeeds(seedArray);
  filter.SetActiveField(fieldName);
  filter.SetUseThreadedAlgorithm(useThreaded);
  if (useAsyncComm)
    filter.SetUseAsynchronousCommunication();
  else
    filter.SetUseSynchronousCommunication();

  if (useBlockIds)
    filter.SetBlockIDs(blockIds);
}

void ValidateOutput(const vtkm::cont::DataSet& out,
                    vtkm::Id numSeeds,
                    const vtkm::Range& xMaxRange,
                    FilterType fType,
                    bool checkEndPoint,
                    bool blockDuplication);

void TestPartitionedDataSet(vtkm::Id nPerRank,
                            bool useGhost,
                            FilterType fType,
                            bool useThreaded,
                            bool useAsyncComm,
                            bool useBlockIds,
                            bool duplicateBlocks);

#endif // vtk_m_filter_flow_testing_TestingFlow_h
