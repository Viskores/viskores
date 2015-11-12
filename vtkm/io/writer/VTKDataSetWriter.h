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

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <vtkm/CellShape.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

namespace {
#define VTK_EMPTY_CELL     0
#define VTK_VERTEX         1
#define VTK_POLY_VERTEX    2
#define VTK_LINE           3
#define VTK_POLY_LINE      4
#define VTK_TRIANGLE       5
#define VTK_TRIANGLE_STRIP 6
#define VTK_POLYGON        7
#define VTK_PIXEL          8
#define VTK_QUAD           9
#define VTK_TETRA         10
#define VTK_VOXEL         11
#define VTK_HEXAHEDRON    12
#define VTK_WEDGE         13
#define VTK_PYRAMID       14
#define VTK_PENTAGONAL_PRISM 15
#define VTK_HEXAGONAL_PRISM  16

int CellShapeToVTK(vtkm::Id type)
{
    switch(type)
    {
      case vtkm::CELL_SHAPE_VERTEX:     return 1;
      case vtkm::CELL_SHAPE_LINE:       return 3;
      case vtkm::CELL_SHAPE_TRIANGLE:   return 5;
      case vtkm::CELL_SHAPE_QUAD:       return 9;
      //case vtkm::CELL_SHAPE_PIXEL:      return 8;
      case vtkm::CELL_SHAPE_TETRA:      return 10;
      case vtkm::CELL_SHAPE_PYRAMID:    return 14;
      case vtkm::CELL_SHAPE_WEDGE:      return 13;
      case vtkm::CELL_SHAPE_HEXAHEDRON: return 12;
      //case vtkm::CELL_SHAPE_VOXEL:      return 11;
      case vtkm::CELL_SHAPE_POLYGON:    return 7;
    }

    return 0;
}

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

template <typename T> struct DataTypeName
{
  static const char* Name() { return "unknown"; }
};
template <> struct DataTypeName<vtkm::Int8>
{
  static const char* Name() { return "char"; }
};
template <> struct DataTypeName<vtkm::UInt8>
{
  static const char* Name() { return "unsigned_char"; }
};
template <> struct DataTypeName<vtkm::Int16>
{
  static const char* Name() { return "short"; }
};
template <> struct DataTypeName<vtkm::UInt16>
{
  static const char* Name() { return "unsigned_short"; }
};
template <> struct DataTypeName<vtkm::Int32>
{
  static const char* Name() { return "int"; }
};
template <> struct DataTypeName<vtkm::UInt32>
{
  static const char* Name() { return "unsigned_int"; }
};
template <> struct DataTypeName<vtkm::Int64>
{
  static const char* Name() { return "long"; }
};
template <> struct DataTypeName<vtkm::UInt64>
{
  static const char* Name() { return "unsigned_long"; }
};
template <> struct DataTypeName<vtkm::Float32>
{
  static const char* Name() { return "float"; }
};
template <> struct DataTypeName<vtkm::Float64>
{
  static const char* Name() { return "double"; }
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
    *this->Name = DataTypeName<DataType>::Name();
  }
private:
  std::string *Name;
};

}

namespace vtkm
{
namespace io
{
namespace writer
{

struct VTKDataSetWriter
{
private:
  static void WritePoints(std::ostream &out,
                          vtkm::cont::DataSet ds)
  {
    ///\todo: support other coordinate systems
    int cindex = 0;

    vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem(cindex);
    vtkm::cont::DynamicArrayHandleCoordinateSystem cdata = coords.GetData();

    vtkm::Id npoints = cdata.GetNumberOfValues();

    std::string typeName;
    cdata.CastAndCall(GetDataTypeName(typeName));

    out << "POINTS " << npoints << " " << typeName << " " << std::endl;
    cdata.CastAndCall(OutputPointsFunctor(out));
  }

  template <class CellSetType>
  static void WriteExplicitCells(std::ostream &out,
                                 CellSetType cs)
  {
    vtkm::Id nCells = cs.GetNumberOfCells();

    vtkm::Id conn_length = 0;
    for (vtkm::Id i=0; i<nCells; ++i)
      conn_length += 1 + cs.GetNumberOfPointsInCell(i);

    out << "CELLS " << nCells << " " << conn_length << std::endl;

    vtkm::Vec<vtkm::Id,8> ids;
    for (vtkm::Id i=0; i<nCells; ++i)
    {
      vtkm::Id nids = cs.GetNumberOfPointsInCell(i);
      cs.GetIndices(i, ids);
      out << nids;
      for (int j=0; j<nids; ++j)
        out << " " << ids[j];
      out << std::endl;
    }

    out << "CELL_TYPES " << nCells << std::endl;
    for (vtkm::Id i=0; i<nCells; ++i)
    {
      vtkm::Id shape = cs.GetCellShape(i);
      out << CellShapeToVTK(shape) << std::endl;
    }
  }

  static void WriteVertexCells(std::ostream &out,
                               vtkm::cont::DataSet ds)
  {
    vtkm::Id n = ds.GetCoordinateSystem(0).GetData().GetNumberOfValues();

    out << "CELLS " << n << " " << n*2 << std::endl;
    for (int i = 0; i < n; i++)
    {
      out << "1 " << i << std::endl;
    }
    out << "CELL_TYPES " << n << std::endl;
    for (int i = 0; i < n; i++)
    {
      out << CellShapeToVTK(CELL_SHAPE_VERTEX) << std::endl;
    }
  }

  static void WritePointFields(std::ostream &out,
                               vtkm::cont::DataSet ds)
  {
    bool wrote_header = false;
    for (vtkm::Id f = 0; f < ds.GetNumberOfFields(); f++)
    {
      const vtkm::cont::Field field = ds.GetField(f);

      if (field.GetAssociation() != vtkm::cont::Field::ASSOC_POINTS)
        continue;

      vtkm::Id npoints = field.GetData().GetNumberOfValues();
      int ncomps = field.GetData().GetNumberOfComponents();
      if (ncomps > 4)
        continue;

      if (!wrote_header)
        out << "POINT_DATA " << npoints << std::endl;
      wrote_header = true;

      std::string typeName;
      field.GetData().CastAndCall(GetDataTypeName(typeName));

      out << "SCALARS " << field.GetName() << " " << typeName << " " << ncomps << std::endl;
      out << "LOOKUP_TABLE default" << std::endl;

      field.GetData().CastAndCall(OutputFieldFunctor(out));
    }
  }

  static void WriteCellFields(std::ostream &out,
                              vtkm::cont::DataSet ds,
                              vtkm::cont::DynamicCellSet cs)
  {
    bool wrote_header = false;
    for (vtkm::Id f = 0; f < ds.GetNumberOfFields(); f++)
    {
      const vtkm::cont::Field field = ds.GetField(f);

      if (field.GetAssociation() != vtkm::cont::Field::ASSOC_CELL_SET)
        continue;
      if (field.GetAssocCellSet() != cs.GetCellSet().GetName())
        continue;

      vtkm::Id ncells = field.GetData().GetNumberOfValues();
      int ncomps = field.GetData().GetNumberOfComponents();
      if (ncomps > 4)
        continue;

      if (!wrote_header)
        out << "CELL_DATA " << ncells << std::endl;
      wrote_header = true;

      std::string typeName;
      field.GetData().CastAndCall(GetDataTypeName(typeName));

      out << "SCALARS " << field.GetName() <<  " " << typeName << " " << ncomps << std::endl;
      out << "LOOKUP_TABLE default" << std::endl;

      field.GetData().CastAndCall(OutputFieldFunctor(out));
    }
  }

  static void WriteDataSetAsPoints(std::ostream &out,
                                   vtkm::cont::DataSet ds)
  {
    out << "DATASET UNSTRUCTURED_GRID" << std::endl;
    WritePoints(out, ds);
    WriteVertexCells(out, ds);
  }

  template <class CellSetType>
  static void WriteDataSetAsUnstructured(std::ostream &out,
                                         vtkm::cont::DataSet ds,
                                         CellSetType cs)
  {
    out << "DATASET UNSTRUCTURED_GRID" << std::endl;
    WritePoints(out, ds);
    WriteExplicitCells(out, cs);
  }

  template <vtkm::IdComponent DIM>
  static void WriteDataSetAsStructured(std::ostream &out,
                                       vtkm::cont::DataSet ds,
                                       vtkm::cont::CellSetStructured<DIM> cs)
  {
    ///\todo: support uniform/rectilinear
    out << "DATASET STRUCTURED_GRID" << std::endl;

    out << "DIMENSIONS ";
    out << cs.GetPointDimensions()[0] << " ";
    out << (DIM>1 ? cs.GetPointDimensions()[1] : 1) << " ";
    out << (DIM>2 ? cs.GetPointDimensions()[2] : 1) << std::endl;

    WritePoints(out, ds);
  }

public:
  static void Write(std::ostream &out, vtkm::cont::DataSet ds, int csindex=0)
  {
    VTKM_ASSERT_CONT(csindex < ds.GetNumberOfCellSets());

    out << "# vtk DataFile Version 3.0" << std::endl;
    out << "vtk output" << std::endl;
    out << "ASCII" << std::endl;

    if (csindex < 0)
    {
      WriteDataSetAsPoints(out, ds);
      WritePointFields(out, ds);
    }
    else
    {
      vtkm::cont::DynamicCellSet cs = ds.GetCellSet(csindex);
      if (cs.IsType<vtkm::cont::CellSetExplicit<> >())
      {
        WriteDataSetAsUnstructured(out, ds,
                                  cs.CastTo<vtkm::cont::CellSetExplicit<> >());
      }
      else if (cs.IsType<vtkm::cont::CellSetStructured<2> >())
      {
        WriteDataSetAsStructured(out, ds,
                                  cs.CastTo<vtkm::cont::CellSetStructured<2> >());
      }
      else if (cs.IsType<vtkm::cont::CellSetStructured<3> >())
      {
        WriteDataSetAsStructured(out, ds,
                                  cs.CastTo<vtkm::cont::CellSetStructured<3> >());
      }
      else if (cs.IsType<vtkm::cont::CellSetSingleType<> >())
      {
        // these function just like explicit cell sets
        WriteDataSetAsUnstructured(out, ds,
                                cs.CastTo<vtkm::cont::CellSetSingleType<> >());
      }
      else
      {
        VTKM_ASSERT_CONT(false);
      }

      WritePointFields(out, ds);
      WriteCellFields(out, ds, cs);
    }
  }

}; //struct VTKDataSetWriter

}}} //namespace vtkm::io::writer

#endif //vtk_m_io_writer_DataSetWriter_h
