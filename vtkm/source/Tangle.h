//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_source_Tangle_h
#define vtk_m_source_Tangle_h

#include <vtkm/source/Source.h>

namespace vtkm
{
namespace source
{
/**
 * @brief The Tangle source creates a uniform dataset.
 *
 * This class generates a predictable uniform grid dataset with an
 * interesting set of point and cell scalar arrays, which is useful
 * for testing and benchmarking.
 *
 * The Execute method creates a complete structured dataset that have a
 * point field named 'nodevar', and a cell field named 'cellvar'.
 *
**/
class VTKM_SOURCE_EXPORT Tangle final : public vtkm::source::Source
{
public:
  ///Construct a Tangle with Cell Dimensions
  VTKM_CONT
  Tangle(vtkm::Id3 dims)
    : Dims(dims)
  {
  }

  vtkm::cont::DataSet Execute() const;

private:
  vtkm::Id3 Dims;
};
} //namespace source
} //namespace vtkm

#endif //vtk_m_source_Tangle_h
