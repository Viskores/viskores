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
#ifndef vtk_m_cont_arg_TypeCheckTagCellSetStructured_h
#define vtk_m_cont_arg_TypeCheckTagCellSetStructured_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/cont/CellSet.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// Check for a Structured CellSet-like object.
///
struct TypeCheckTagCellSetStructured
{
};


template <typename CellSetType>
struct TypeCheck<TypeCheckTagCellSetStructured, CellSetType>
{
  using is_3d_cellset = std::is_same<CellSetType, vtkm::cont::CellSetStructured<3>>;
  using is_2d_cellset = std::is_same<CellSetType, vtkm::cont::CellSetStructured<2>>;
  using is_1d_cellset = std::is_same<CellSetType, vtkm::cont::CellSetStructured<1>>;

  static constexpr bool value =
    is_3d_cellset::value || is_2d_cellset::value || is_1d_cellset::value;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagCellSetStructured_h
