//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_io_writer_DataSetWriter_h
#define vtk_m_io_writer_DataSetWriter_h

#include <vtkm/CellShape.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorControlBadType.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/Field.h>

#include <vtkm/io/ErrorIO.h>

#include <vtkm/io/internal/VTKDataSetTypes.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace vtkm {
namespace io {
namespace writer {

namespace detail {

struct OutputPointsFunctor
{
private:
  std::ostream &out;

  template <typename PortalType>
  VTKM_CONT_EXPORT
  void Output(const PortalType &portal) const
  {
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      const int VTKDims = 3; // VTK files always require 3 dims for points

      typedef typename PortalType::ValueType ValueType;
      typedef typename vtkm::VecTraits<ValueType> VecType;

      const ValueType &value = portal.Get(index);

      vtkm::IdComponent numComponents = VecType::GetNumberOfComponents(value);
      for (vtkm::IdComponent c = 0; c < numComponents && c < VTKDims; c++)
      {
        out << (c==0 ? "" : " ") << VecType::GetComponent(value, c);
      }
      for (vtkm::IdComponent c = numComponents; c < VTKDims; c++)
      {
        out << " 0";
      }
      out << std::endl;
    }
  }

public:
  VTKM_CONT_EXPORT
  OutputPointsFunctor(std::ostream &o) : out(o)
  {
  }

  template <typename T, typename Storage>
  VTKM_CONT_EXPORT
  void operator()(const vtkm::cont::ArrayHandle<T,Storage > &array) const
  {
    this->Output(array.GetPortalConstControl());
  }
};

struct OutputFieldFunctor
{
private:
  std::ostream &out;

  template <typename PortalType>
  VTKM_CONT_EXPORT
  void Output(const PortalType &portal) const
  {
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      typedef typename PortalType::ValueType ValueType;
      typedef typename vtkm::VecTraits<ValueType> VecType;

      const ValueType &value = portal.Get(index);

      vtkm::IdComponent numComponents = VecType::GetNumberOfComponents(value);
      for (vtkm::IdComponent c = 0; c < numComponents; c++)
      {
        out << (c==0 ? "" : " ") << VecType::GetComponent(value, c);
      }
      out << std::endl;
    }
  }
public:
  VTKM_CONT_EXPORT
  OutputFieldFunctor(std::ostream &o) : out(o)
  {
  }

  template <typename T, typename Storage>
  VTKM_CONT_EXPORT
  void operator()(const vtkm::cont::ArrayHandle<T,Storage > &array) const
  {
    this->Output(array.GetPortalConstControl());
  }
};

class GetDataTypeName
{
public:
  GetDataTypeName(std::string &name)
    : Name(&name)
  { }

  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType &) const
  {
    typedef typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType
        DataType;
    *this->Name = vtkm::io::internal::DataTypeName<DataType>::Name();
  }
private:
  std::string *Name;
};

} // namespace detail

struct VTKDataSetWriter
{
private:
  static void WritePoints(std::ostream &out,
                          vtkm::cont::DataSet dataSet)
  {
    ///\todo: support other coordinate systems
    int cindex = 0;

    vtkm::cont::CoordinateSystem coords = dataSet.GetCoordinateSystem(cindex);
    vtkm::cont::DynamicArrayHandleCoordinateSystem cdata = coords.GetData();

    vtkm::Id npoints = cdata.GetNumberOfValues();

    std::string typeName;
    cdata.CastAndCall(detail::GetDataTypeName(typeName));

    out << "POINTS " << npoints << " " << typeName << " " << std::endl;
    cdata.CastAndCall(detail::OutputPointsFunctor(out));
  }

  template <class CellSetType>
  static void WriteExplicitCells(std::ostream &out,
                                 CellSetType cellSet)
  {
    vtkm::Id nCells = cellSet.GetNumberOfCells();

    vtkm::Id conn_length = 0;
    for (vtkm::Id i=0; i<nCells; ++i)
    {
      conn_length += 1 + cellSet.GetNumberOfPointsInCell(i);
    }

    out << "CELLS " << nCells << " " << conn_length << std::endl;

    vtkm::Vec<vtkm::Id,8> ids;
    for (vtkm::Id i=0; i<nCells; ++i)
    {
      vtkm::Id nids = cellSet.GetNumberOfPointsInCell(i);
      cellSet.GetIndices(i, ids);
      out << nids;
      for (int j=0; j<nids; ++j)
        out << " " << ids[j];
      out << std::endl;
    }

    out << "CELL_TYPES " << nCells << std::endl;
    for (vtkm::Id i=0; i<nCells; ++i)
    {
      vtkm::Id shape = cellSet.GetCellShape(i);
      out << shape << std::endl;
    }
  }

  static void WriteVertexCells(std::ostream &out,
                               vtkm::cont::DataSet dataSet)
  {
    vtkm::Id nCells = dataSet.GetCoordinateSystem(0).GetData().GetNumberOfValues();

    out << "CELLS " << nCells << " " << nCells*2 << std::endl;
    for (int i = 0; i < nCells; i++)
    {
      out << "1 " << i << std::endl;
    }
    out << "CELL_TYPES " << nCells << std::endl;
    for (int i = 0; i < nCells; i++)
    {
      out << vtkm::CELL_SHAPE_VERTEX << std::endl;
    }
  }

  static void WritePointFields(std::ostream &out,
                               vtkm::cont::DataSet dataSet)
  {
    bool wrote_header = false;
    for (vtkm::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
    {
      const vtkm::cont::Field field = dataSet.GetField(f);

      if (field.GetAssociation() != vtkm::cont::Field::ASSOC_POINTS) {continue;}

      vtkm::Id npoints = field.GetData().GetNumberOfValues();
      int ncomps = field.GetData().GetNumberOfComponents();
      if (ncomps > 4) { continue; }

      if (!wrote_header)
      {
        out << "POINT_DATA " << npoints << std::endl;
        wrote_header = true;
      }

      std::string typeName;
      field.GetData().CastAndCall(detail::GetDataTypeName(typeName));

      out << "SCALARS " << field.GetName() << " "
          << typeName << " " << ncomps << std::endl;
      out << "LOOKUP_TABLE default" << std::endl;

      field.GetData().CastAndCall(detail::OutputFieldFunctor(out));
    }
  }

  static void WriteCellFields(std::ostream &out,
                              vtkm::cont::DataSet dataSet,
                              vtkm::cont::DynamicCellSet cellSet)
  {
    bool wrote_header = false;
    for (vtkm::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
    {
      const vtkm::cont::Field field = dataSet.GetField(f);

      if (field.GetAssociation() != vtkm::cont::Field::ASSOC_CELL_SET)
      {
        continue;
      }
      if (field.GetAssocCellSet() != cellSet.GetCellSet().GetName()) {
        continue;
      }

      vtkm::Id ncells = field.GetData().GetNumberOfValues();
      int ncomps = field.GetData().GetNumberOfComponents();
      if (ncomps > 4)
        continue;

      if (!wrote_header)
      {
        out << "CELL_DATA " << ncells << std::endl;
        wrote_header = true;
      }

      std::string typeName;
      field.GetData().CastAndCall(detail::GetDataTypeName(typeName));

      out << "SCALARS " << field.GetName() <<  " "
          << typeName << " " << ncomps << std::endl;
      out << "LOOKUP_TABLE default" << std::endl;

      field.GetData().CastAndCall(detail::OutputFieldFunctor(out));
    }
  }

  static void WriteDataSetAsPoints(std::ostream &out,
                                   vtkm::cont::DataSet dataSet)
  {
    out << "DATASET UNSTRUCTURED_GRID" << std::endl;
    WritePoints(out, dataSet);
    WriteVertexCells(out, dataSet);
  }

  template <class CellSetType>
  static void WriteDataSetAsUnstructured(std::ostream &out,
                                         vtkm::cont::DataSet dataSet,
                                         CellSetType cellSet)
  {
    out << "DATASET UNSTRUCTURED_GRID" << std::endl;
    WritePoints(out, dataSet);
    WriteExplicitCells(out, cellSet);
  }

  template <vtkm::IdComponent DIM>
  static void WriteDataSetAsStructured(
      std::ostream &out,
      vtkm::cont::DataSet dataSet,
      vtkm::cont::CellSetStructured<DIM> cellSet)
  {
    ///\todo: support uniform/rectilinear
    out << "DATASET STRUCTURED_GRID" << std::endl;

    out << "DIMENSIONS ";
    out << cellSet.GetPointDimensions()[0] << " ";
    out << (DIM>1 ? cellSet.GetPointDimensions()[1] : 1) << " ";
    out << (DIM>2 ? cellSet.GetPointDimensions()[2] : 1) << std::endl;

    WritePoints(out, dataSet);
  }

  static void Write(std::ostream &out,
                    vtkm::cont::DataSet dataSet,
                    vtkm::Id csindex=0)
  {
    VTKM_ASSERT(csindex < dataSet.GetNumberOfCellSets());

    out << "# vtk DataFile Version 3.0" << std::endl;
    out << "vtk output" << std::endl;
    out << "ASCII" << std::endl;

    if (csindex < 0)
    {
      WriteDataSetAsPoints(out, dataSet);
      WritePointFields(out, dataSet);
    }
    else
    {
      vtkm::cont::DynamicCellSet cellSet = dataSet.GetCellSet(csindex);
      if (cellSet.IsType<vtkm::cont::CellSetExplicit<> >())
      {
        WriteDataSetAsUnstructured(out,
                                   dataSet,
                                   cellSet.Cast<vtkm::cont::CellSetExplicit<> >());
      }
      else if (cellSet.IsType<vtkm::cont::CellSetStructured<2> >())
      {
        WriteDataSetAsStructured(out,
                                 dataSet,
                                 cellSet.Cast<vtkm::cont::CellSetStructured<2> >());
      }
      else if (cellSet.IsType<vtkm::cont::CellSetStructured<3> >())
      {
        WriteDataSetAsStructured(out,
                                 dataSet,
                                 cellSet.Cast<vtkm::cont::CellSetStructured<3> >());
      }
      else if (cellSet.IsType<vtkm::cont::CellSetSingleType<> >())
      {
        // these function just like explicit cell sets
        WriteDataSetAsUnstructured(out,
                                   dataSet,
                                   cellSet.Cast<vtkm::cont::CellSetSingleType<> >());
      }
      else
      {
        throw vtkm::cont::ErrorControlBadType(
              "Could not determine type to write out.");
      }

      WritePointFields(out, dataSet);
      WriteCellFields(out, dataSet, cellSet);
    }
  }

public:
  VTKM_CONT_EXPORT
  explicit VTKDataSetWriter(const std::string &filename)
    : FileName(filename) {  }

  VTKM_CONT_EXPORT
  void WriteDataSet(vtkm::cont::DataSet dataSet,
                    vtkm::Id cellSetIndex = 0) const
  {
    if (cellSetIndex >= dataSet.GetNumberOfCellSets())
    {
      if (cellSetIndex == 0)
      {
        // Special case where there are no cell sets. In this case, write out
        // the data as points.
        cellSetIndex = -1;
      }
      else
      {
        throw vtkm::cont::ErrorControlBadValue(
              "Selected invalid cell set index.");
      }
    }

    if (dataSet.GetNumberOfCoordinateSystems() < 1)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "DataSet has no coordinate system, which is not supported by VTK file format.");
    }

    try
    {
      std::ofstream fileStream(this->FileName.c_str(), std::fstream::trunc);
      this->Write(fileStream, dataSet, cellSetIndex);
      fileStream.close();
    }
    catch (std::ofstream::failure error)
    {
      throw vtkm::io::ErrorIO(error.what());
    }
  }

  VTKM_CONT_EXPORT
  void WriteDataSet(vtkm::cont::DataSet dataSet,
                    const std::string &cellSetName)
  {
    this->WriteDataSet(dataSet, dataSet.GetCellSetIndex(cellSetName));
  }

private:
  std::string FileName;

}; //struct VTKDataSetWriter

}
}
} //namespace vtkm::io::writer

#endif //vtk_m_io_writer_DataSetWriter_h
