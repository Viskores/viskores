//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_source_Amr_h
#define vtk_m_source_Amr_h

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/source/vtkm_source_export.h>

namespace vtkm
{
namespace source
{
/**
 * @brief The Amr source creates a dataset similar to VTK's
 * vtkRTAnalyticSource.
 *
 * This class generates a predictable structured dataset with a smooth yet
 * interesting set of scalars, which is useful for testing and benchmarking.
 *
 * The Execute method creates a complete structured dataset that have a
 * point field names 'scalars'
 *
 * The scalars are computed as:
 *
 * ```
 * MaxVal * Gauss + MagX * sin(FrqX*x) + MagY * sin(FrqY*y) + MagZ * cos(FrqZ*z)
 * ```
 *
 * The dataset properties are determined by:
 * - `Minimum/MaximumExtent`: The logical point extents of the dataset.
 * - `Spacing`: The distance between points of the dataset.
 * - `Center`: The center of the dataset.
 *
 * The scalar functions is control via:
 * - `Center`: The center of a Gaussian contribution to the scalars.
 * - `StandardDeviation`: The unscaled width of a Gaussian contribution.
 * - `MaximumValue`: Upper limit of the scalar range.
 * - `Frequency`: The Frq[XYZ] parameters of the periodic contributions.
 * - `Magnitude`: The Mag[XYZ] parameters of the periodic contributions.
 *
 * By default, the following parameters are used:
 * - `Extents`: { -10, -10, -10 } `-->` { 10, 10, 10 }
 * - `Spacing`: { 1, 1, 1 }
 * - `Center`: { 0, 0, 0 }
 * - `StandardDeviation`: 0.5
 * - `MaximumValue`: 255
 * - `Frequency`: { 60, 30, 40 }
 * - `Magnitude`: { 10, 18, 5 }
 */
class VTKM_SOURCE_EXPORT Amr
{
public:
  VTKM_CONT
  Amr(vtkm::IdComponent dimension = 2,
      vtkm::IdComponent cellsPerDimension = 6,
      vtkm::IdComponent numberOfLevels = 4);
  VTKM_CONT
  ~Amr();

  vtkm::cont::PartitionedDataSet Execute() const;

private:
  template <vtkm::IdComponent Dim>
  vtkm::cont::DataSet GenerateDataSet(unsigned int level, unsigned int amrIndex) const;

  vtkm::IdComponent Dimension;
  vtkm::IdComponent CellsPerDimension;
  vtkm::IdComponent NumberOfLevels;
};
} //namespace source
} //namespace vtkm

#endif //vtk_m_source_Amr_h
