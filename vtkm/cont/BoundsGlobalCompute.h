//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_BoundsGlobalCompute_h
#define vtk_m_cont_BoundsGlobalCompute_h

#include <vtkm/Bounds.h>
#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

class DataSet;
class MultiBlock;

//@{
/// \brief Functions to compute bounds for a dataset or multiblock globally
///
/// These are utility functions that compute bounds for the dataset or
/// multiblock globally i.e. across all ranks when operating in a distributed
/// environment. When VTK-m not operating in an distributed environment, these
/// behave same as `vtkm::cont::BoundsCompute`.
///
/// Note that if the provided CoordinateSystem does not exists, empty bounds
/// are returned. Likewise, for MultiBlock, blocks without the chosen CoordinateSystem
/// are skipped.
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::DataSet& dataset,
                                 vtkm::Id coordinate_system_index = 0);

VTKM_CONT_EXPORT
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::MultiBlock& multiblock,
                                 vtkm::Id coordinate_system_index = 0);

VTKM_CONT_EXPORT
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::DataSet& dataset,
                                 const std::string& coordinate_system_name);

VTKM_CONT_EXPORT
VTKM_CONT
vtkm::Bounds BoundsGlobalCompute(const vtkm::cont::MultiBlock& multiblock,
                                 const std::string& coordinate_system_name);
//@}
}
}
#endif
