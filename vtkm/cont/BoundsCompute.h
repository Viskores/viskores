//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_BoundsCompute_h
#define vtk_m_cont_BoundsCompute_h

#include <vtkm/Bounds.h>
#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

class DataSet;
class MultiBlock;

//@{
/// \brief Functions to compute bounds for a dataset or multiblock
///
/// These are utility functions that compute bounds for the dataset or
/// multiblock. When VTK-m is operating in an distributed environment, these
/// are bounds on the local process. To get global bounds across all ranks,
/// use `vtkm::cont::BoundsGlobalCompute` instead.
///
/// Note that if the provided CoordinateSystem does not exists, empty bounds
/// are returned. Likewise, for MultiBlock, blocks without the chosen CoordinateSystem
/// are skipped.
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::Bounds BoundsCompute(const vtkm::cont::DataSet& dataset,
                           vtkm::Id coordinate_system_index = 0);

VTKM_CONT_EXPORT
VTKM_CONT
vtkm::Bounds BoundsCompute(const vtkm::cont::MultiBlock& multiblock,
                           vtkm::Id coordinate_system_index = 0);

VTKM_CONT_EXPORT
VTKM_CONT
vtkm::Bounds BoundsCompute(const vtkm::cont::DataSet& dataset,
                           const std::string& coordinate_system_name);

VTKM_CONT_EXPORT
VTKM_CONT
vtkm::Bounds BoundsCompute(const vtkm::cont::MultiBlock& multiblock,
                           const std::string& coordinate_system_name);
//@}
}
}

#endif
