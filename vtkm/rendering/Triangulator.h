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
#ifndef vtk_m_rendering_Triangulator_h
#define vtk_m_rendering_Triangulator_h

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm {
namespace rendering {
/// \brief Triangulator creates a minimal set of triangles from a cell set.
///
///  This class creates a array of triangle indices from both 3D and 2D
///  explicit cell sets. This list can serve as input to opengl and the
///  ray tracer scene renderers. TODO: Add regular grid support
///
template<typename DeviceAdapter>
class Triangulator
{
private:
  typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
  typedef typename vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id,4> > Vec4ArrayHandle;
  typedef typename Vec4ArrayHandle::ExecutionTypes<DeviceAdapter>::Portal Vec4ArrayPortalType;
  typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalConstType;
public:
  template<class T>
  class MemSet : public vtkm::worklet::WorkletMapField
  {
    T Value;
  public:
    VTKM_CONT_EXPORT
    MemSet(T value)
      : Value(value)
    {}
    typedef void ControlSignature(FieldOut<>);
    typedef void ExecutionSignature(_1);
    VTKM_EXEC_EXPORT
    void operator()(T &outValue) const
    {
      outValue = Value;
    }
  }; //class MemSet
  class CountTriangles : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT_EXPORT
    CountTriangles(){}
    typedef void ControlSignature(FieldIn<>,
                                  FieldOut<>);
    typedef void ExecutionSignature(_1,
                                    _2);
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &shapeType,
                    vtkm::Id &triangles) const
    {
      if( shapeType == vtkm::CELL_SHAPE_TRIANGLE ) triangles = 1;
      else if( shapeType == vtkm::CELL_SHAPE_QUAD ) triangles = 2;
      else if( shapeType == vtkm::CELL_SHAPE_TETRA ) triangles = 4;
      else if( shapeType == vtkm::CELL_SHAPE_HEXAHEDRON ) triangles = 12;
      else if( shapeType == vtkm::CELL_SHAPE_WEDGE ) triangles = 8;
      else if( shapeType == vtkm::CELL_SHAPE_PYRAMID ) triangles = 6;
      else triangles = 0;
    }
  }; //class CountTriangles

  class TrianglulateStructured :
        public vtkm::worklet::WorkletMapPointToCell
  {
  private:
    Vec4ArrayPortalType OutputIndices;
  public:
    typedef void ControlSignature(TopologyIn topology,
                                  FieldInTo<>);
    typedef void ExecutionSignature(FromIndices, _2);
    //typedef _1 InputDomain;
    VTKM_CONT_EXPORT
    TrianglulateStructured(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id,4> > &outputIndices)
    {
      this->OutputIndices = outputIndices.PrepareForOutput(outputIndices.GetNumberOfValues(), DeviceAdapter() );
    }
    template<typename CellNodeVecType>
    VTKM_EXEC_EXPORT
    void operator()(const CellNodeVecType &cellIndices,
                    const vtkm::Id &cellIndex) const
    {
      const vtkm::Id triangleOffset = cellIndex * 12;
      vtkm::Vec<vtkm::Id,4> triangle;
      // 0-1-2
      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[2];
      triangle[0] = cellIndex;
      OutputIndices.Set(triangleOffset, triangle);
      // 0-3-2
      triangle[2] = cellIndices[3];
      OutputIndices.Set(triangleOffset+1, triangle);
      // 0-3-7
      triangle[3] = cellIndices[7];
      OutputIndices.Set(triangleOffset+2, triangle);
      // 0-4-7
      triangle[2] = cellIndices[4];
      OutputIndices.Set(triangleOffset+3, triangle);
      // 5-4-7
      triangle[1] = cellIndices[5];
      OutputIndices.Set(triangleOffset+4, triangle);
      // 5-6-7
      triangle[2] = cellIndices[6];
      OutputIndices.Set(triangleOffset+5, triangle);
      // 3-6-7
      triangle[1] = cellIndices[3];
      OutputIndices.Set(triangleOffset+6, triangle);
      // 3-6-2
      triangle[3] = cellIndices[2];
      OutputIndices.Set(triangleOffset+7, triangle);
      // 1-6-2
      triangle[1] = cellIndices[1];
      OutputIndices.Set(triangleOffset+8, triangle);
      // 1-6-5
      triangle[3] = cellIndices[5];
      OutputIndices.Set(triangleOffset+9, triangle);
      // 1-4-5
      triangle[2] = cellIndices[4];
      OutputIndices.Set(triangleOffset+10, triangle);
      // 1-4-0
      triangle[3] = cellIndices[0];
      OutputIndices.Set(triangleOffset+11, triangle);
    }

  };

  class IndicesSort : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT_EXPORT
    IndicesSort(){}
    typedef void ControlSignature(FieldInOut<>);
    typedef void ExecutionSignature(_1);
    VTKM_EXEC_EXPORT
    void operator()( vtkm::Vec<vtkm::Id,4> &triangleIndices) const
    {
      // first field contains the id of the cell the
      // trianlge belongs to
      vtkm::Id temp;
      if (triangleIndices[1] > triangleIndices[3])
      {
          temp = triangleIndices[1];
          triangleIndices[1] = triangleIndices[3];
          triangleIndices[3] = temp;
      }
      if (triangleIndices[1] > triangleIndices[2])
      {
          temp = triangleIndices[1];
          triangleIndices[1] = triangleIndices[2];
          triangleIndices[2] = temp;
      }
      if (triangleIndices[2] > triangleIndices[3])
      {
          temp = triangleIndices[2];
          triangleIndices[2] = triangleIndices[3];
          triangleIndices[3] = temp;
      }
    }
  }; //class IndicesSort

  struct IndicesLessThan
  {
    VTKM_EXEC_CONT_EXPORT
    bool operator()(const vtkm::Vec<vtkm::Id,4> &a,
                    const vtkm::Vec<vtkm::Id,4> &b) const
    {
      if(a[1] < b[1]) return true;
      if(a[1] > b[1]) return false;
      if(a[2] < b[2]) return true;
      if(a[2] > b[2]) return false;
      if(a[3] < b[3]) return true;
      return false;
    }
  };

  class UniqueTriangles : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT_EXPORT
    UniqueTriangles(){}
    typedef void ControlSignature(ExecObject,
                                  ExecObject);
    typedef void ExecutionSignature(_1,_2,WorkIndex);
    VTKM_EXEC_EXPORT
    bool IsTwin(const vtkm::Vec<vtkm::Id,4> &a, const vtkm::Vec<vtkm::Id,4> &b) const
    {
      return (a[1] == b[1] && a[2] == b[2] && a[3] == b[3]);
    }
    VTKM_EXEC_EXPORT
    void operator()(vtkm::exec::ExecutionWholeArrayConst<vtkm::Vec<vtkm::Id,4> > &indices,
                    vtkm::exec::ExecutionWholeArray<vtkm::UInt8> &outputFlags,
                    const vtkm::Id &index) const
    {
      if (index == 0) return;
      //if we are a shared face, mark ourself and neighbor for desctruction
      if(IsTwin(indices.Get(index), indices.Get(index-1) ))
      {
        outputFlags.Set(index, 0);
        outputFlags.Set(index - 1, 0);
      }
    }
  }; //class UniqueTriangles


  class Trianglulate : public vtkm::worklet::WorkletMapField
  {
  private:
    IdPortalConstType Indices;
    Vec4ArrayPortalType OutputIndices;

    vtkm::Int32 TRIANGLE_INDICES;
    vtkm::Int32 QUAD_INDICES;
    vtkm::Int32 TETRA_INDICES;
    vtkm::Int32 HEX_INDICES;
    vtkm::Int32 WEDGE_INDICES;
    vtkm::Int32 PYRAMID_INDICES;
  public:
    VTKM_CONT_EXPORT
    Trianglulate(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id,4> > &outputIndices,
                 const vtkm::cont::ArrayHandle<vtkm::Id> &indices,
                 const vtkm::Id &size)
      :  Indices(indices.PrepareForInput(DeviceAdapter()))
    {
      this->OutputIndices = outputIndices.PrepareForOutput(size, DeviceAdapter() );
      TRIANGLE_INDICES = 3;
      QUAD_INDICES = 4;
      TETRA_INDICES = 4;
      HEX_INDICES = 8;
      WEDGE_INDICES = 6;
      PYRAMID_INDICES = 5;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>);
    typedef void ExecutionSignature(_1,
                                    _2,
                                    _3,
                                    WorkIndex);
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &shapeType,
                    const vtkm::Id &indexOffset,
                    const vtkm::Id &triangleOffset,
                    const vtkm::Id &cellId) const
    {
      vtkm::Vec<vtkm::Id,4> triangle;
      if( shapeType == vtkm::CELL_SHAPE_TRIANGLE )
      {

         triangle[1] = Indices.Get(indexOffset+0);
         triangle[2] = Indices.Get(indexOffset+1);
         triangle[3] = Indices.Get(indexOffset+2);
         triangle[0] = cellId;
         OutputIndices.Set(triangleOffset, triangle);
      }
      if( shapeType == vtkm::CELL_SHAPE_QUAD )
      {

        triangle[1] = Indices.Get(indexOffset+0);
        triangle[2] = Indices.Get(indexOffset+1);
        triangle[3] = Indices.Get(indexOffset+2);
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);

        triangle[2] = Indices.Get(indexOffset+3);
        OutputIndices.Set(triangleOffset+1, triangle);
      }
      if( shapeType == vtkm::CELL_SHAPE_TETRA )
      {
        // 0-1-2
        triangle[1] = Indices.Get(indexOffset+1);
        triangle[2] = Indices.Get(indexOffset+0);
        triangle[3] = Indices.Get(indexOffset+2);
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);
        // 0-1-3
        triangle[1] = Indices.Get(indexOffset+1);
        triangle[2] = Indices.Get(indexOffset+0);
        triangle[3] = Indices.Get(indexOffset+3);

        OutputIndices.Set(triangleOffset+1, triangle);
        // 2-1-3
        triangle[1] = Indices.Get(indexOffset+1);
        triangle[2] = Indices.Get(indexOffset+2);
        triangle[3] = Indices.Get(indexOffset+3);
        OutputIndices.Set(triangleOffset+2, triangle);
        // 2-0-3
        triangle[1] = Indices.Get(indexOffset+2);
        triangle[2] = Indices.Get(indexOffset+0);
        triangle[3] = Indices.Get(indexOffset+3);
        OutputIndices.Set(triangleOffset+3, triangle);
      }
      if( shapeType == vtkm::CELL_SHAPE_HEXAHEDRON )
      {
        // 0-1-2
        triangle[1] = Indices.Get(indexOffset+0);
        triangle[2] = Indices.Get(indexOffset+1);
        triangle[3] = Indices.Get(indexOffset+2);
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);
        // 0-3-2
        triangle[2] = Indices.Get(indexOffset+3);
        OutputIndices.Set(triangleOffset+1, triangle);
        // 0-3-7
        triangle[3] = Indices.Get(indexOffset+7);
        OutputIndices.Set(triangleOffset+2, triangle);
        // 0-4-7
        triangle[2] = Indices.Get(indexOffset+4);
        OutputIndices.Set(triangleOffset+3, triangle);
        // 5-4-7
        triangle[1] = Indices.Get(indexOffset+5);
        OutputIndices.Set(triangleOffset+4, triangle);
        // 5-6-7
        triangle[2] = Indices.Get(indexOffset+6);
        OutputIndices.Set(triangleOffset+5, triangle);
        // 3-6-7
        triangle[1] = Indices.Get(indexOffset+3);
        OutputIndices.Set(triangleOffset+6, triangle);
        // 3-6-2
        triangle[3] = Indices.Get(indexOffset+2);
        OutputIndices.Set(triangleOffset+7, triangle);
        // 1-6-2
        triangle[1] = Indices.Get(indexOffset+1);
        OutputIndices.Set(triangleOffset+8, triangle);
        // 1-6-5
        triangle[3] = Indices.Get(indexOffset+5);
        OutputIndices.Set(triangleOffset+9, triangle);
        // 1-4-5
        triangle[2] = Indices.Get(indexOffset+4);
        OutputIndices.Set(triangleOffset+10, triangle);
        // 1-4-0
        triangle[3] = Indices.Get(indexOffset+0);
        OutputIndices.Set(triangleOffset+11, triangle);
      }
      if( shapeType == vtkm::CELL_SHAPE_WEDGE )
      {
        // 0-1-2
        triangle[1] = Indices.Get(indexOffset+0);
        triangle[2] = Indices.Get(indexOffset+1);
        triangle[3] = Indices.Get(indexOffset+2);
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);
        // 0-3-2
        triangle[2] = Indices.Get(indexOffset+3);
        OutputIndices.Set(triangleOffset+1, triangle);
        // 5-3-2
        triangle[1] = Indices.Get(indexOffset+5);
        OutputIndices.Set(triangleOffset+2, triangle);
        // 5-3-4
        triangle[3] = Indices.Get(indexOffset+4);
        OutputIndices.Set(triangleOffset+3, triangle);
        // 5-2-4
        triangle[2] = Indices.Get(indexOffset+2);
        OutputIndices.Set(triangleOffset+4, triangle);
        // 5-1-4
        triangle[2] = Indices.Get(indexOffset+1);
        OutputIndices.Set(triangleOffset+5, triangle);
      }
      if( shapeType == vtkm::CELL_SHAPE_PYRAMID )
      {
        // 0-1-2
        triangle[1] = Indices.Get(indexOffset+0);
        triangle[2] = Indices.Get(indexOffset+1);
        triangle[3] = Indices.Get(indexOffset+2);
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);
        // 0-3-2
        triangle[2] = Indices.Get(indexOffset+3);
        OutputIndices.Set(triangleOffset+1, triangle);
        // 0-3-4
        triangle[3] = Indices.Get(indexOffset+4);
        OutputIndices.Set(triangleOffset+2, triangle);
        // 2-3-4
        triangle[1] = Indices.Get(indexOffset+2);
        OutputIndices.Set(triangleOffset+3, triangle);
        // 2-1-4
        triangle[2] = Indices.Get(indexOffset+1);
        OutputIndices.Set(triangleOffset+4, triangle);
        // 0-3-4
        triangle[1] = Indices.Get(indexOffset+0);
        OutputIndices.Set(triangleOffset+5, triangle);
      }
    }
  }; //class Trianglulate

public:
  VTKM_CONT_EXPORT
  Triangulator() {}

  VTKM_CONT_EXPORT
  void ExternalTrianlges(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id,4> > &outputIndices,
                         vtkm::Id &outputTriangles)
  {
    //Eliminate unseen triangles
    vtkm::worklet::DispatcherMapField<IndicesSort>()
      .Invoke(outputIndices);
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(outputIndices, IndicesLessThan());
    vtkm::cont::ArrayHandle<vtkm::UInt8> flags;
    flags.Allocate(outputTriangles);
    vtkm::worklet::DispatcherMapField< MemSet< vtkm::UInt8 > >( MemSet< vtkm::UInt8>( 1 ) )
      .Invoke( flags );
    //Unique triangles will have a flag = 1
    vtkm::worklet::DispatcherMapField< UniqueTriangles >()
      .Invoke( vtkm::exec::ExecutionWholeArrayConst< vtkm::Vec<vtkm::Id,4> >(outputIndices),
               vtkm::exec::ExecutionWholeArray< vtkm::UInt8 >(flags));

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id,4> > subset;
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::StreamCompact(outputIndices,
                                                                     flags,
                                                                     subset);
    outputIndices = subset;
    outputTriangles = subset.GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void run(const vtkm::cont::DynamicCellSet &cellset,
           vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id,4> > &outputIndices,
           vtkm::Id &outputTriangles)
  {

    if(cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      vtkm::cont::CellSetStructured<3> cellSetStructured3D = cellset.Cast<vtkm::cont::CellSetStructured<3> >();
      const vtkm::Id numCells = cellSetStructured3D.GetNumberOfCells();

      vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIdxs(0,1,numCells);
      outputIndices.Allocate(numCells * 12);
      vtkm::worklet::DispatcherMapTopology<TrianglulateStructured>(TrianglulateStructured(outputIndices))
        .Invoke(cellSetStructured3D,
                cellIdxs);

      outputTriangles = numCells * 12;
    }
    else if(cellset.IsSameType(vtkm::cont::CellSetExplicit<>()))
    {
      vtkm::cont::CellSetExplicit<> cellSetExplicit = cellset.Cast<vtkm::cont::CellSetExplicit<> >();
      const vtkm::cont::ArrayHandle<vtkm::UInt8> shapes = cellSetExplicit.GetShapesArray( vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell());
      const vtkm::cont::ArrayHandle<vtkm::Int32> indices = cellSetExplicit.GetNumIndicesArray( vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell());
      vtkm::cont::ArrayHandle<vtkm::Id> conn = cellSetExplicit.GetConnectivityArray( vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell());
      vtkm::cont::ArrayHandle<vtkm::Id> offsets = cellSetExplicit.GetIndexOffsetArray( vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell());

      // We need to somehow force the data set to build the index offsets
      //vtkm::IdComponent c = indices.GetPortalControl().Get(0);
      vtkm::Vec< vtkm::Id, 3> forceBuildIndices;
      cellSetExplicit.GetIndices(0, forceBuildIndices );


      vtkm::cont::ArrayHandle<vtkm::Id> trianglesPerCell;
      vtkm::worklet::DispatcherMapField<CountTriangles>( CountTriangles() )
        .Invoke(shapes,
                trianglesPerCell);

      vtkm::Id totalTriangles = 0;
      totalTriangles = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(trianglesPerCell,
                                                                                 vtkm::Id(0));

      vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanExclusive(trianglesPerCell,
                                                                       cellOffsets);
      outputIndices.Allocate(totalTriangles);
      vtkm::worklet::DispatcherMapField<Trianglulate>( Trianglulate(outputIndices,
                                                                    conn,
                                                                    totalTriangles) )
        .Invoke(shapes,
                offsets,
                cellOffsets);

      outputTriangles = totalTriangles;
    }
    else if(cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))
    {
      typedef vtkm::TopologyElementTagPoint PointTag;
      typedef vtkm::TopologyElementTagCell CellTag;

      vtkm::cont::CellSetSingleType<> cellSetSingleType = cellset.Cast<vtkm::cont::CellSetSingleType<> >();

      //fetch and see if we are all triangles, that currently is the only
      //cell set single type we support.
      vtkm::Id shapeTypeAsId = cellSetSingleType.GetCellShape(0);

      if(shapeTypeAsId == vtkm::CellShapeTagTriangle::Id)
        {
        //generate the outputIndices
        vtkm::Id totalTriangles = cellSetSingleType.GetNumberOfCells();
        vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIdxs(0,1,totalTriangles);

        outputIndices.Allocate(totalTriangles);
        vtkm::worklet::DispatcherMapField<Trianglulate>( Trianglulate(outputIndices,
                                                                      cellSetSingleType.GetConnectivityArray( PointTag(), CellTag() ),
                                                                      totalTriangles) )
        .Invoke(cellSetSingleType.GetShapesArray( PointTag(), CellTag() ),
                cellSetSingleType.GetIndexOffsetArray( PointTag(), CellTag()),
                cellIdxs );

        outputTriangles = totalTriangles;
        }
      else
        {
        throw vtkm::cont::ErrorControlBadType("Unsupported cell type for trianglulation with CellSetSingleType");
        }
    }
    else
    {
      throw vtkm::cont::ErrorControlBadType("Unsupported cell set type for trianglulation");
    }

    //get rid of any triagles we cannot see
    ExternalTrianlges(outputIndices, outputTriangles);
  }
}; // class Triangulator
}}//namespace vtkm::rendering
#endif //vtk_m_rendering_Triangulator_h
