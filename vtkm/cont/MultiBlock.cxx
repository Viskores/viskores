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

#include <vtkm/StaticAssert.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/MultiBlock.h>

namespace vtkm
{
namespace cont
{

VTKM_CONT
MultiBlock::MultiBlock(const vtkm::cont::DataSet& ds)
{
  this->Blocks.insert(this->Blocks.end(), ds);
}

VTKM_CONT
MultiBlock::MultiBlock(const vtkm::cont::MultiBlock& src)
{
  this->Blocks = src.GetBlocks();
}

VTKM_CONT
MultiBlock::MultiBlock(const std::vector<vtkm::cont::DataSet>& mblocks)
{
  this->Blocks = mblocks;
}

VTKM_CONT
MultiBlock::MultiBlock(vtkm::Id size)
{
  this->Blocks.reserve(static_cast<std::size_t>(size));
}

VTKM_CONT
MultiBlock::MultiBlock()
{
}

VTKM_CONT
MultiBlock::~MultiBlock()
{
}

VTKM_CONT
MultiBlock& MultiBlock::operator=(const vtkm::cont::MultiBlock& src)
{
  this->Blocks = src.GetBlocks();
  return *this;
}

VTKM_CONT
vtkm::cont::Field MultiBlock::GetField(const std::string& field_name, const int& block_index)
{
  assert(block_index >= 0);
  assert(static_cast<std::size_t>(block_index) < this->Blocks.size());
  return this->Blocks[static_cast<std::size_t>(block_index)].GetField(field_name);
}

VTKM_CONT
vtkm::Id MultiBlock::GetNumberOfBlocks() const
{
  return static_cast<vtkm::Id>(this->Blocks.size());
}

VTKM_CONT
const vtkm::cont::DataSet& MultiBlock::GetBlock(vtkm::Id blockId) const
{
  return this->Blocks[static_cast<std::size_t>(blockId)];
}

VTKM_CONT
const std::vector<vtkm::cont::DataSet>& MultiBlock::GetBlocks() const
{
  return this->Blocks;
}

VTKM_CONT
void MultiBlock::AddBlock(const vtkm::cont::DataSet& ds)
{
  this->Blocks.insert(this->Blocks.end(), ds);
  return;
}

void MultiBlock::AddBlocks(const std::vector<vtkm::cont::DataSet>& mblocks)
{
  this->Blocks.insert(this->Blocks.end(), mblocks.begin(), mblocks.end());
  return;
}

VTKM_CONT
void MultiBlock::InsertBlock(vtkm::Id index, const vtkm::cont::DataSet& ds)
{
  if (index <= static_cast<vtkm::Id>(this->Blocks.size()))
    this->Blocks.insert(this->Blocks.begin() + index, ds);
  else
  {
    std::string msg = "invalid insert position\n ";
    throw ErrorExecution(msg);
  }
}

VTKM_CONT
void MultiBlock::ReplaceBlock(vtkm::Id index, const vtkm::cont::DataSet& ds)
{
  if (index < static_cast<vtkm::Id>(this->Blocks.size()))
    this->Blocks.at(static_cast<std::size_t>(index)) = ds;
  else
  {
    std::string msg = "invalid replace position\n ";
    throw ErrorExecution(msg);
  }
}

VTKM_CONT
void MultiBlock::PrintSummary(std::ostream& stream) const
{
  stream << "block "
         << "\n";

  for (size_t block_index = 0; block_index < this->Blocks.size(); ++block_index)
  {
    stream << "block " << block_index << "\n";
    this->Blocks[block_index].PrintSummary(stream);
  }
}
}
} // namespace vtkm::cont
