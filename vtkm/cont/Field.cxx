//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/Field.h>

namespace vtkm {
namespace cont {

void Field::PrintSummary(std::ostream &out) const
{
    out<<"   "<<this->Name;
    out<<" assoc= ";
    switch (this->GetAssociation())
    {
    case ASSOC_ANY: out<<"Any "; break;
    case ASSOC_WHOLE_MESH: out<<"Mesh "; break;
    case ASSOC_POINTS: out<<"Points "; break;
    case ASSOC_CELL_SET: out<<"Cells "; break;
    case ASSOC_LOGICAL_DIM: out<<"LogicalDim "; break;
    }
    this->Data.PrintSummary(out);
    out<<"\n";
}

const vtkm::cont::DynamicArrayHandle &Field::GetData() const
{
return this->Data;
}

vtkm::cont::DynamicArrayHandle &Field::GetData()
{
this->ModifiedFlag = true;
return this->Data;
}

}
} // namespace vtkm::cont
