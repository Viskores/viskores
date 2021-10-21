//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/VTKPolyDataReader.h>

#include <vtkm/cont/ConvertNumComponentsToOffsets.h>

namespace
{

template <typename T>
inline vtkm::cont::ArrayHandle<T> ConcatinateArrayHandles(
  const std::vector<vtkm::cont::ArrayHandle<T>>& arrays)
{
  vtkm::Id size = 0;
  for (std::size_t i = 0; i < arrays.size(); ++i)
  {
    size += arrays[i].GetNumberOfValues();
  }

  vtkm::cont::ArrayHandle<T> out;
  out.Allocate(size);

  auto outp = vtkm::cont::ArrayPortalToIteratorBegin(out.WritePortal());
  for (std::size_t i = 0; i < arrays.size(); ++i)
  {
    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(arrays[i].ReadPortal()),
              vtkm::cont::ArrayPortalToIteratorEnd(arrays[i].ReadPortal()),
              outp);
    using DifferenceType = typename std::iterator_traits<decltype(outp)>::difference_type;
    std::advance(outp, static_cast<DifferenceType>(arrays[i].GetNumberOfValues()));
  }

  return out;
}
}

namespace vtkm
{
namespace io
{

VTKPolyDataReader::VTKPolyDataReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKPolyDataReader::VTKPolyDataReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

void VTKPolyDataReader::Read()
{
  if (this->DataFile->Structure != vtkm::io::internal::DATASET_POLYDATA)
  {
    throw vtkm::io::ErrorIO("Incorrect DataSet type");
  }

  //We need to be able to handle VisIt files which dump Field data
  //at the top of a VTK file
  std::string tag;
  this->DataFile->Stream >> tag;
  if (tag == "FIELD")
  {
    this->ReadGlobalFields();
    this->DataFile->Stream >> tag;
  }

  // Read the points
  internal::parseAssert(tag == "POINTS");
  this->ReadPoints();

  vtkm::Id numPoints = this->DataSet.GetNumberOfPoints();

  // Read the cellset
  std::vector<vtkm::cont::ArrayHandle<vtkm::Id>> connectivityArrays;
  std::vector<vtkm::cont::ArrayHandle<vtkm::IdComponent>> numIndicesArrays;
  std::vector<vtkm::UInt8> shapesBuffer;
  while (!this->DataFile->Stream.eof())
  {
    vtkm::UInt8 shape = vtkm::CELL_SHAPE_EMPTY;
    this->DataFile->Stream >> tag;
    if (tag == "VERTICES")
    {
      shape = vtkm::io::internal::CELL_SHAPE_POLY_VERTEX;
    }
    else if (tag == "LINES")
    {
      shape = vtkm::io::internal::CELL_SHAPE_POLY_LINE;
    }
    else if (tag == "POLYGONS")
    {
      shape = vtkm::CELL_SHAPE_POLYGON;
    }
    else if (tag == "TRIANGLE_STRIPS")
    {
      shape = vtkm::io::internal::CELL_SHAPE_TRIANGLE_STRIP;
    }
    else
    {
      this->DataFile->Stream.seekg(-static_cast<std::streamoff>(tag.length()), std::ios_base::cur);
      break;
    }

    vtkm::cont::ArrayHandle<vtkm::Id> cellConnectivity;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> cellNumIndices;
    this->ReadCells(cellConnectivity, cellNumIndices);

    connectivityArrays.push_back(cellConnectivity);
    numIndicesArrays.push_back(cellNumIndices);
    shapesBuffer.insert(
      shapesBuffer.end(), static_cast<std::size_t>(cellNumIndices.GetNumberOfValues()), shape);
  }

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity = ConcatinateArrayHandles(connectivityArrays);
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices = ConcatinateArrayHandles(numIndicesArrays);
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  shapes.Allocate(static_cast<vtkm::Id>(shapesBuffer.size()));
  std::copy(shapesBuffer.begin(),
            shapesBuffer.end(),
            vtkm::cont::ArrayPortalToIteratorBegin(shapes.WritePortal()));

  vtkm::cont::ArrayHandle<vtkm::Id> permutation;
  vtkm::io::internal::FixupCellSet(connectivity, numIndices, shapes, permutation);
  this->SetCellsPermutation(permutation);

  if (vtkm::io::internal::IsSingleShape(shapes))
  {
    vtkm::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(
      numPoints, shapes.ReadPortal().Get(0), numIndices.ReadPortal().Get(0), connectivity);
    this->DataSet.SetCellSet(cellSet);
  }
  else
  {
    auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numIndices);
    vtkm::cont::CellSetExplicit<> cellSet;
    cellSet.Fill(numPoints, shapes, connectivity, offsets);
    this->DataSet.SetCellSet(cellSet);
  }

  // Read points and cell attributes
  this->ReadAttributes();
}
}
} // namespace vtkm::io
