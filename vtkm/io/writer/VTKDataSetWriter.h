//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_writer_DataSetWriter_h
#define vtk_m_io_writer_DataSetWriter_h

#include <vtkm/CellShape.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>

#include <vtkm/io/ErrorIO.h>

#include <vtkm/io/internal/VTKDataSetTypes.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace vtkm
{
namespace io
{
namespace writer
{

namespace detail
{

struct OutputPointsFunctor
{
private:
  std::ostream& out;

  template <typename PortalType>
  VTKM_CONT void Output(const PortalType& portal) const
  {
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      const int VTKDims = 3; // VTK files always require 3 dims for points

      using ValueType = typename PortalType::ValueType;
      using VecType = typename vtkm::VecTraits<ValueType>;

      const ValueType& value = portal.Get(index);

      vtkm::IdComponent numComponents = VecType::GetNumberOfComponents(value);
      for (vtkm::IdComponent c = 0; c < numComponents && c < VTKDims; c++)
      {
        out << (c == 0 ? "" : " ") << VecType::GetComponent(value, c);
      }
      for (vtkm::IdComponent c = numComponents; c < VTKDims; c++)
      {
        out << " 0";
      }
      out << std::endl;
    }
  }

public:
  VTKM_CONT
  OutputPointsFunctor(std::ostream& o)
    : out(o)
  {
  }

  template <typename T, typename Storage>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, Storage>& array) const
  {
    this->Output(array.GetPortalConstControl());
  }
};

struct OutputFieldFunctor
{
private:
  std::ostream& out;

  template <typename PortalType>
  VTKM_CONT void Output(const PortalType& portal) const
  {
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      using ValueType = typename PortalType::ValueType;
      using VecType = typename vtkm::VecTraits<ValueType>;

      const ValueType& value = portal.Get(index);

      vtkm::IdComponent numComponents = VecType::GetNumberOfComponents(value);
      for (vtkm::IdComponent c = 0; c < numComponents; c++)
      {
        out << (c == 0 ? "" : " ") << VecType::GetComponent(value, c);
      }
      out << std::endl;
    }
  }

public:
  VTKM_CONT
  OutputFieldFunctor(std::ostream& o)
    : out(o)
  {
  }

  template <typename T, typename Storage>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, Storage>& array) const
  {
    this->Output(array.GetPortalConstControl());
  }
};

class GetDataTypeName
{
public:
  GetDataTypeName(std::string& name)
    : Name(&name)
  {
  }

  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType&) const
  {
    using DataType = typename vtkm::VecTraits<typename ArrayHandleType::ValueType>::ComponentType;
    *this->Name = vtkm::io::internal::DataTypeName<DataType>::Name();
  }

private:
  std::string* Name;
};

} // namespace detail

struct VTKDataSetWriter
{
private:
  static void WritePoints(std::ostream& out, const vtkm::cont::DataSet& dataSet)
  {
    ///\todo: support other coordinate systems
    int cindex = 0;
    auto cdata = dataSet.GetCoordinateSystem(cindex).GetData();

    vtkm::Id npoints = cdata.GetNumberOfValues();
    out << "POINTS " << npoints << " "
        << vtkm::io::internal::DataTypeName<vtkm::FloatDefault>::Name() << " " << std::endl;

    detail::OutputPointsFunctor{ out }(cdata);
  }

  template <class CellSetType>
  static void WriteExplicitCells(std::ostream& out, const CellSetType& cellSet)
  {
    vtkm::Id nCells = cellSet.GetNumberOfCells();

    vtkm::Id conn_length = 0;
    for (vtkm::Id i = 0; i < nCells; ++i)
    {
      conn_length += 1 + cellSet.GetNumberOfPointsInCell(i);
    }

    out << "CELLS " << nCells << " " << conn_length << std::endl;

    for (vtkm::Id i = 0; i < nCells; ++i)
    {
      vtkm::cont::ArrayHandle<vtkm::Id> ids;
      vtkm::Id nids = cellSet.GetNumberOfPointsInCell(i);
      cellSet.GetIndices(i, ids);
      out << nids;
      auto IdPortal = ids.GetPortalConstControl();
      for (int j = 0; j < nids; ++j)
        out << " " << IdPortal.Get(j);
      out << std::endl;
    }

    out << "CELL_TYPES " << nCells << std::endl;
    for (vtkm::Id i = 0; i < nCells; ++i)
    {
      vtkm::Id shape = cellSet.GetCellShape(i);
      out << shape << std::endl;
    }
  }

  static void WriteVertexCells(std::ostream& out, const vtkm::cont::DataSet& dataSet)
  {
    vtkm::Id nCells = dataSet.GetCoordinateSystem(0).GetNumberOfPoints();

    out << "CELLS " << nCells << " " << nCells * 2 << std::endl;
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

  static void WritePointFields(std::ostream& out, const vtkm::cont::DataSet& dataSet)
  {
    bool wrote_header = false;
    for (vtkm::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
    {
      const vtkm::cont::Field field = dataSet.GetField(f);

      if (field.GetAssociation() != vtkm::cont::Field::Association::POINTS)
      {
        continue;
      }

      vtkm::Id npoints = field.GetNumberOfValues();
      int ncomps = field.GetData().GetNumberOfComponents();
      if (ncomps > 4)
      {
        continue;
      }

      if (!wrote_header)
      {
        out << "POINT_DATA " << npoints << std::endl;
        wrote_header = true;
      }

      std::string typeName;
      vtkm::cont::CastAndCall(field, detail::GetDataTypeName(typeName));

      out << "SCALARS " << field.GetName() << " " << typeName << " " << ncomps << std::endl;
      out << "LOOKUP_TABLE default" << std::endl;

      vtkm::cont::CastAndCall(field, detail::OutputFieldFunctor(out));
    }
  }

  static void WriteCellFields(std::ostream& out, const vtkm::cont::DataSet& dataSet)
  {
    bool wrote_header = false;
    for (vtkm::Id f = 0; f < dataSet.GetNumberOfFields(); f++)
    {
      const vtkm::cont::Field field = dataSet.GetField(f);
      if (!field.IsFieldCell())
      {
        continue;
      }


      vtkm::Id ncells = field.GetNumberOfValues();
      int ncomps = field.GetData().GetNumberOfComponents();
      if (ncomps > 4)
        continue;

      if (!wrote_header)
      {
        out << "CELL_DATA " << ncells << std::endl;
        wrote_header = true;
      }

      std::string typeName;
      vtkm::cont::CastAndCall(field, detail::GetDataTypeName(typeName));

      out << "SCALARS " << field.GetName() << " " << typeName << " " << ncomps << std::endl;
      out << "LOOKUP_TABLE default" << std::endl;

      vtkm::cont::CastAndCall(field, detail::OutputFieldFunctor(out));
    }
  }

  static void WriteDataSetAsPoints(std::ostream& out, const vtkm::cont::DataSet& dataSet)
  {
    out << "DATASET UNSTRUCTURED_GRID" << std::endl;
    WritePoints(out, dataSet);
    WriteVertexCells(out, dataSet);
  }

  template <class CellSetType>
  static void WriteDataSetAsUnstructured(std::ostream& out,
                                         const vtkm::cont::DataSet& dataSet,
                                         const CellSetType& cellSet)
  {
    out << "DATASET UNSTRUCTURED_GRID" << std::endl;
    WritePoints(out, dataSet);
    WriteExplicitCells(out, cellSet);
  }

  template <vtkm::IdComponent DIM>
  static void WriteDataSetAsStructured(std::ostream& out,
                                       const vtkm::cont::DataSet& dataSet,
                                       const vtkm::cont::CellSetStructured<DIM>& cellSet)
  {
    ///\todo: support uniform/rectilinear
    out << "DATASET STRUCTURED_GRID" << std::endl;

    auto pointDimensions = cellSet.GetPointDimensions();
    using VTraits = vtkm::VecTraits<decltype(pointDimensions)>;

    out << "DIMENSIONS ";
    out << VTraits::GetComponent(pointDimensions, 0) << " ";
    out << (DIM > 1 ? VTraits::GetComponent(pointDimensions, 1) : 1) << " ";
    out << (DIM > 2 ? VTraits::GetComponent(pointDimensions, 2) : 1) << " ";

    WritePoints(out, dataSet);
  }

  static void Write(std::ostream& out, const vtkm::cont::DataSet& dataSet, bool just_points = false)
  {
    out << "# vtk DataFile Version 3.0" << std::endl;
    out << "vtk output" << std::endl;
    out << "ASCII" << std::endl;

    if (just_points)
    {
      WriteDataSetAsPoints(out, dataSet);
      WritePointFields(out, dataSet);
    }
    else
    {
      vtkm::cont::DynamicCellSet cellSet = dataSet.GetCellSet();
      if (cellSet.IsType<vtkm::cont::CellSetExplicit<>>())
      {
        WriteDataSetAsUnstructured(out, dataSet, cellSet.Cast<vtkm::cont::CellSetExplicit<>>());
      }
      else if (cellSet.IsType<vtkm::cont::CellSetStructured<1>>())
      {
        WriteDataSetAsStructured(out, dataSet, cellSet.Cast<vtkm::cont::CellSetStructured<1>>());
      }
      else if (cellSet.IsType<vtkm::cont::CellSetStructured<2>>())
      {
        WriteDataSetAsStructured(out, dataSet, cellSet.Cast<vtkm::cont::CellSetStructured<2>>());
      }
      else if (cellSet.IsType<vtkm::cont::CellSetStructured<3>>())
      {
        WriteDataSetAsStructured(out, dataSet, cellSet.Cast<vtkm::cont::CellSetStructured<3>>());
      }
      else if (cellSet.IsType<vtkm::cont::CellSetSingleType<>>())
      {
        // these function just like explicit cell sets
        WriteDataSetAsUnstructured(out, dataSet, cellSet.Cast<vtkm::cont::CellSetSingleType<>>());
      }
      else
      {
        throw vtkm::cont::ErrorBadType("Could not determine type to write out.");
      }

      WritePointFields(out, dataSet);
      WriteCellFields(out, dataSet);
    }
  }

public:
  VTKM_CONT
  explicit VTKDataSetWriter(const std::string& filename)
    : FileName(filename)
  {
  }

  VTKM_CONT
  void WriteDataSet(const vtkm::cont::DataSet& dataSet, bool just_points = false) const
  {
    if (dataSet.GetNumberOfCoordinateSystems() < 1)
    {
      throw vtkm::cont::ErrorBadValue(
        "DataSet has no coordinate system, which is not supported by VTK file format.");
    }
    try
    {
      std::ofstream fileStream(this->FileName.c_str(), std::fstream::trunc);
      this->Write(fileStream, dataSet, just_points);
      fileStream.close();
    }
    catch (std::ofstream::failure& error)
    {
      throw vtkm::io::ErrorIO(error.what());
    }
  }

private:
  std::string FileName;

}; //struct VTKDataSetWriter
}
}
} //namespace vtkm::io::writer

#endif //vtk_m_io_writer_DataSetWriter_h
