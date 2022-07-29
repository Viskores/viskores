//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/filter/AmrArrays.h>
#include <vtkm/filter/field_conversion/CellAverage.h>
#include <vtkm/source/Amr.h>
#include <vtkm/source/Wavelet.h>


namespace vtkm
{
namespace source
{

Amr::Amr(vtkm::IdComponent dimension,
         vtkm::IdComponent cellsPerDimension,
         vtkm::IdComponent numberOfLevels)
  : Dimension(dimension)
  , CellsPerDimension(cellsPerDimension)
  , NumberOfLevels(numberOfLevels)
{
}

Amr::~Amr() = default;

template <vtkm::IdComponent Dim>
vtkm::cont::DataSet Amr::GenerateDataSet(unsigned int level, unsigned int amrIndex) const
{
  vtkm::Id3 extent = { vtkm::Id(this->CellsPerDimension / 2) };
  vtkm::Id3 dimensions = { this->CellsPerDimension + 1 };
  vtkm::Vec3f origin = { float(1. / pow(2, level) * amrIndex) };
  vtkm::Vec3f spacing = { float(1. / this->CellsPerDimension / pow(2, level)) };
  vtkm::Vec3f center = 0.5f - (origin + spacing * extent);
  vtkm::Vec3f frequency = { 60.f, 30.f, 40.f };
  frequency = frequency * this->CellsPerDimension;
  vtkm::FloatDefault deviation = 0.5f / this->CellsPerDimension;

  if (Dim == 2)
  {
    extent[2] = 0;
    dimensions[2] = 1;
    origin[2] = 0;
    spacing[2] = 1;
    center[2] = 0;
  }

  vtkm::source::Wavelet waveletSource(-extent, extent);
  waveletSource.SetOrigin(origin);
  waveletSource.SetSpacing(spacing);
  waveletSource.SetCenter(center);
  waveletSource.SetFrequency(frequency);
  waveletSource.SetStandardDeviation(deviation);
  vtkm::cont::DataSet wavelet = waveletSource.Execute();

  vtkm::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("RTData", vtkm::cont::Field::Association::Points);
  cellAverage.SetOutputFieldName("RTDataCells");
  return cellAverage.Execute(wavelet);
}

vtkm::cont::PartitionedDataSet Amr::Execute() const
{
  assert(this->CellsPerDimension > 1);
  assert(this->CellsPerDimension % 2 == 0);

  // Generate AMR
  std::vector<std::vector<vtkm::Id>> blocksPerLevel(this->NumberOfLevels);
  unsigned int counter = 0;
  for (unsigned int l = 0; l < blocksPerLevel.size(); l++)
  {
    for (unsigned int b = 0; b < pow(2, l); b++)
    {
      blocksPerLevel.at(l).push_back(counter++);
    }
  }
  vtkm::cont::PartitionedDataSet amrDataSet;

  // Fill AMR with data from the wavelet
  for (unsigned int l = 0; l < blocksPerLevel.size(); l++)
  {
    for (unsigned int b = 0; b < blocksPerLevel.at(l).size(); b++)
    {
      if (this->Dimension == 2)
      {
        amrDataSet.AppendPartition(this->GenerateDataSet<2>(l, b));
      }
      else if (this->Dimension == 3)
      {
        amrDataSet.AppendPartition(this->GenerateDataSet<3>(l, b));
      }
    }
  }

  // Generate helper arrays
  vtkm::filter::AmrArrays amrArrays;
  amrDataSet = amrArrays.Execute(amrDataSet);

  return amrDataSet;
}

} // namespace source
} // namespace vtkm
