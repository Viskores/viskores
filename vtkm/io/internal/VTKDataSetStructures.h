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
#ifndef vtk_m_io_internal_VTKDataSetStructures_h
#define vtk_m_io_internal_VTKDataSetStructures_h

#include <string>

namespace vtkm
{
namespace io
{
namespace internal
{

enum DataSetStructure
{
  DATASET_UNKNOWN = 0,
  DATASET_STRUCTURED_POINTS,
  DATASET_STRUCTURED_GRID,
  DATASET_UNSTRUCTURED_GRID,
  DATASET_POLYDATA,
  DATASET_RECTILINEAR_GRID,
  DATASET_FIELD
};

inline const char* DataSetStructureString(int id)
{
  static const char* strings[] = { "",
                                   "STRUCTURED_POINTS",
                                   "STRUCTURED_GRID",
                                   "UNSTRUCTURED_GRID",
                                   "POLYDATA",
                                   "RECTILINEAR_GRID",
                                   "FIELD" };
  return strings[id];
}

inline DataSetStructure DataSetStructureId(const std::string& str)
{
  DataSetStructure structure = DATASET_UNKNOWN;
  for (int id = 1; id < 7; ++id)
  {
    if (str == DataSetStructureString(id))
    {
      structure = static_cast<DataSetStructure>(id);
    }
  }

  return structure;
}
}
}
} // namespace vtkm::io::internal

#endif // vtk_m_io_internal_VTKDataSetStructures_h
