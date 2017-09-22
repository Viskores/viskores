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
#ifndef vtk_m_io_reader_VTKPolyDataReader_h
#define vtk_m_io_reader_VTKPolyDataReader_h

#include <vtkm/io/reader/VTKDataSetReaderBase.h>

#include <vtkm/cont/ArrayPortalToIterators.h>

#include "iterator"

namespace vtkm
{
namespace io
{
namespace reader
{

namespace internal
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

  using IteratorType = typename vtkm::cont::ArrayPortalToIterators<
    typename vtkm::cont::ArrayHandle<T>::PortalControl>::IteratorType;
  IteratorType outp = vtkm::cont::ArrayPortalToIteratorBegin(out.GetPortalControl());
  for (std::size_t i = 0; i < arrays.size(); ++i)
  {
    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(arrays[i].GetPortalConstControl()),
              vtkm::cont::ArrayPortalToIteratorEnd(arrays[i].GetPortalConstControl()),
              outp);
    using DifferenceType = typename std::iterator_traits<IteratorType>::difference_type;
    std::advance(outp, static_cast<DifferenceType>(arrays[i].GetNumberOfValues()));
  }

  return out;
}

} // namespace internal

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

class VTKPolyDataReader : public VTKDataSetReaderBase
{
public:
  explicit VTKPolyDataReader(const char* fileName)
    : VTKDataSetReaderBase(fileName)
  {
  }

private:
  virtual void Read()
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
      std::string name;
      this->ReadFields(name);
      this->DataFile->Stream >> tag;
    }

    // Read the points
    internal::parseAssert(tag == "POINTS");
    this->ReadPoints();

    vtkm::Id numPoints = this->DataSet.GetCoordinateSystem().GetData().GetNumberOfValues();

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
        this->DataFile->Stream.seekg(-static_cast<std::streamoff>(tag.length()),
                                     std::ios_base::cur);
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

    vtkm::cont::ArrayHandle<vtkm::Id> connectivity =
      internal::ConcatinateArrayHandles(connectivityArrays);
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices =
      internal::ConcatinateArrayHandles(numIndicesArrays);
    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    shapes.Allocate(static_cast<vtkm::Id>(shapesBuffer.size()));
    std::copy(shapesBuffer.begin(),
              shapesBuffer.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(shapes.GetPortalControl()));

    vtkm::cont::ArrayHandle<vtkm::Id> permutation;
    vtkm::io::internal::FixupCellSet(connectivity, numIndices, shapes, permutation);
    this->SetCellsPermutation(permutation);

    if (vtkm::io::internal::IsSingleShape(shapes))
    {
      vtkm::cont::CellSetSingleType<> cellSet("cells");
      cellSet.Fill(numPoints,
                   shapes.GetPortalConstControl().Get(0),
                   numIndices.GetPortalConstControl().Get(0),
                   connectivity);
      this->DataSet.AddCellSet(cellSet);
    }
    else
    {
      vtkm::cont::CellSetExplicit<> cellSet("cells");
      cellSet.Fill(numPoints, shapes, numIndices, connectivity);
      this->DataSet.AddCellSet(cellSet);
    }

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace vtkm::io:reader

#endif // vtk_m_io_reader_VTKPolyDataReader_h
