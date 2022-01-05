//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/NewFilterField.h>

namespace vtkm
{
namespace filter
{

void NewFilterField::ResizeIfNeeded(size_t index_st)
{
  if (this->ActiveFieldNames.size() <= index_st)
  {
    auto oldSize = this->ActiveFieldNames.size();
    this->ActiveFieldNames.resize(index_st + 1);
    this->ActiveFieldAssociation.resize(index_st + 1);
    this->UseCoordinateSystemAsField.resize(index_st + 1);
    this->ActiveCoordinateSystemIndices.resize(index_st + 1);
    for (std::size_t i = oldSize; i <= index_st; ++i)
    {
      this->ActiveFieldAssociation[i] = cont::Field::Association::ANY;
      this->UseCoordinateSystemAsField[i] = false;
      this->ActiveCoordinateSystemIndices[i] = 0;
    }
  }
}

} // namespace filter
} // namespace vtkm
