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
#ifndef vtk_m_worklet_ExternalFaces_h
#define vtk_m_worklet_ExternalFaces_h

#include <vtkm/CellShape.h>
#include <vtkm/Math.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

// #define __VTKM_EXTERNAL_FACES_BENCHMARK

namespace vtkm
{
namespace worklet
{

struct ExternalFaces
{
  //Unary predicate operator
  //Returns True if the argument is equal to 1; False otherwise.
  struct IsUnity
  {
    template<typename T>
    VTKM_EXEC_CONT_EXPORT bool operator()(const T &x) const
    {
        return x == T(1);
    }
  };

  //Functor that returns an index into the cell-point connectivity array,
  //given an index for the face-point connectivity array.
  struct GetConnIndex
  {
  private:
    vtkm::Id FacesPerCell;
    vtkm::Id PointsPerCell;

  public:

    VTKM_CONT_EXPORT
    GetConnIndex() {};

    VTKM_CONT_EXPORT
    GetConnIndex(const vtkm::Id &f,
                 const vtkm::Id &p) :
      FacesPerCell(f),
      PointsPerCell(p)
    {};

    VTKM_EXEC_CONT_EXPORT
    vtkm::Id operator()(vtkm::Id index) const
    {
      vtkm::Id divisor = FacesPerCell*PointsPerCell;
      vtkm::Id cellIndex = index / divisor;
      vtkm::Id vertexIndex = (index % divisor) % PointsPerCell;
      return PointsPerCell*cellIndex + vertexIndex;
    }
  };

  //Returns True if the first vector argument is less than the second
  //vector argument; otherwise, False
  struct Id3LessThan
  {
    template<typename T>
    VTKM_EXEC_CONT_EXPORT bool operator()(const vtkm::Vec<T,3> &a,
                                          const vtkm::Vec<T,3> &b) const
    {
    bool isLessThan = false;
    if(a[0] < b[0])
    {
      isLessThan = true;
    }
    else if(a[0] == b[0])
    {
      if(a[1] < b[1])
      {
        isLessThan = true;
      }
      else if(a[1] == b[1])
      {
        if(a[2] < b[2])
        {
          isLessThan = true;
        }
      }
    }
    return isLessThan;
    }
  };

  //Binary operator
  //Returns (a-b) mod c
  class SubtractAndModulus : public vtkm::worklet::WorkletMapField
  {
  private:
    vtkm::Id Modulus;

  public:
    typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>);
    typedef _3 ExecutionSignature(_1, _2);

    VTKM_CONT_EXPORT
    SubtractAndModulus(const vtkm::Id &c) : Modulus(c) { };

    template<typename T>
    VTKM_EXEC_CONT_EXPORT
    T operator()(const T &a, const T &b) const
    {
      return (a - b) % Modulus;
    }
  };

  //Worklet that returns the number of faces for each cell/shape
  class NumFacesPerCell : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<>, FieldOut<>);
    typedef _2 ExecutionSignature(_1);
    typedef _1 InputDomain;

    template<typename T>
    VTKM_EXEC_EXPORT
    T operator()(const T &cellType) const
    {
      if (cellType == vtkm::CELL_SHAPE_TETRA) return 4;
      else if (cellType == vtkm::CELL_SHAPE_PYRAMID) return 5;
      else if (cellType == vtkm::CELL_SHAPE_WEDGE) return 5;
      else if (cellType == vtkm::CELL_SHAPE_HEXAHEDRON) return 6;
      else return CELL_SHAPE_EMPTY;
    }
  };


  //Worklet that identifies the vertices of a cell face
  class GetFace : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(FieldInTo<AllTypes> localFaceIds,
                                  CellSetIn cellset,
                                  FieldOut<VecCommon> faceVertices
                                  );
    typedef void ExecutionSignature(_1, _3, CellShape, FromIndices);
    typedef _2 InputDomain;

    VTKM_CONT_EXPORT
    GetFace() { }

    template<typename T,
             typename FaceValueVecType,
             typename CellShapeTag,
             typename CellNodeVecType>
    VTKM_EXEC_EXPORT
    void operator()(const T &cellFaceId,
                    FaceValueVecType &faceVertices,
                    CellShapeTag shape,
                    const CellNodeVecType &cellNodeIds) const
    {
      if (shape.Id == vtkm::CELL_SHAPE_TETRA)
      {
        vtkm::IdComponent faceIdTable[12] = {0,1,2,0,1,3,0,2,3,1,2,3};

        //Assign cell points/nodes to this face
        vtkm::Id faceP1 = cellNodeIds[faceIdTable[cellFaceId*3]];
        vtkm::Id faceP2 = cellNodeIds[faceIdTable[cellFaceId*3 + 1]];
        vtkm::Id faceP3 = cellNodeIds[faceIdTable[cellFaceId*3 + 2]];

        //Sort the face points/nodes in ascending order
        vtkm::Id sorted[3] = {faceP1, faceP2, faceP3};
        vtkm::Id temp;
        if (sorted[0] > sorted[2])
        {
            temp = sorted[0];
            sorted[0] = sorted[2];
            sorted[2] = temp;
        }
        if (sorted[0] > sorted[1])
        {
            temp = sorted[0];
            sorted[0] = sorted[1];
            sorted[1] = temp;
        }
        if (sorted[1] > sorted[2])
        {
            temp = sorted[1];
            sorted[1] = sorted[2];
            sorted[2] = temp;
        }

        faceVertices[0] = static_cast<T>(sorted[0]);
        faceVertices[1] = static_cast<T>(sorted[1]);
        faceVertices[2] = static_cast<T>(sorted[2]);
      }
    }
  };

public:

  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry
  /// \param pointArray : Input points
  /// \param pointIdArray: Input point-ids
  /// \param cellToConnectivityIndexArray : Connectivity
  /// \param bounds : Bounds of the input dataset
  /// \param nDivisions: Number of divisions
  /// \param output_pointArray: Output points
  /// \param output_pointId3Array: Output point-ids
  template <typename StorageT,
            typename StorageU,
            typename StorageV,
            typename DeviceAdapter>
  void run(const vtkm::cont::ArrayHandle<vtkm::UInt8, StorageT>       shapes,
           const vtkm::cont::ArrayHandle<vtkm::IdComponent, StorageU> numIndices,
           const vtkm::cont::ArrayHandle<vtkm::Id, StorageV>          conn,
           vtkm::cont::ArrayHandle<vtkm::UInt8, StorageT>             &output_shapes,
           vtkm::cont::ArrayHandle<vtkm::IdComponent, StorageU>       &output_numIndices,
           vtkm::cont::ArrayHandle<vtkm::Id, StorageV>                &output_conn,
           DeviceAdapter)
  {
    //Create a worklet to map the number of faces to each cell
    vtkm::cont::ArrayHandle<vtkm::Id> facesPerCell;
    vtkm::worklet::DispatcherMapField<NumFacesPerCell> numFacesDispatcher;

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
      vtkm::cont::Timer<DeviceAdapter> timer;
#endif
    numFacesDispatcher.Invoke(shapes, facesPerCell);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "NumFacesPerCell_Worklet," << timer.GetElapsedTime() << "\n";
#endif

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    //Inclusive scan of the number of faces per cell
    vtkm::cont::ArrayHandle<vtkm::Id> numFacesPrefixSum;
    const vtkm::Id totalFaces =
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanInclusive(facesPerCell,
                                                                     numFacesPrefixSum);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "ScanInclusive_Adapter," << timer.GetElapsedTime() << "\n";
#endif
    facesPerCell.ReleaseResources();
    if(totalFaces == 0) return;

    //Generate reverse lookup: face index to cell index
    //terminate if no cells have triangles left
    //For 2 tetrahedron cells with 4 faces each (array of size 8): 0,0,0,0,1,1,1,1
    vtkm::cont::ArrayHandle<vtkm::Id> face2CellId;
    vtkm::cont::ArrayHandle<vtkm::Id> localFaceIds;
    localFaceIds.Allocate(static_cast<vtkm::Id>(totalFaces));
    vtkm::cont::ArrayHandleIndex countingArray =
        vtkm::cont::ArrayHandleIndex(vtkm::Id(totalFaces));

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::UpperBounds(numFacesPrefixSum,
                                                                 countingArray,
                                                                 face2CellId);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "UpperBounds_Adapter," << timer.GetElapsedTime() << "\n";
#endif
    numFacesPrefixSum.ReleaseResources();

    vtkm::worklet::DispatcherMapField<SubtractAndModulus> subtractDispatcher(SubtractAndModulus(4));

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    subtractDispatcher.Invoke(countingArray, face2CellId, localFaceIds);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "SubtractAndModulus_Worklet," << timer.GetElapsedTime() << "\n";
#endif
    countingArray.ReleaseResources();

    //Extract the point/vertices for each cell face
    typedef vtkm::cont::ArrayHandle<vtkm::UInt8> UInt8HandleType;
    typedef vtkm::cont::ArrayHandle<vtkm::IdComponent> IdCompHandleType;
    typedef vtkm::cont::ArrayHandle<vtkm::Id> IdHandleType;

    typedef vtkm::cont::ArrayHandleImplicit<vtkm::Id, GetConnIndex> IdImplicitType;

    typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, UInt8HandleType>    UInt8PermutationHandleType;
    typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, IdCompHandleType>   IdCompPermutationHandleType;
    typedef vtkm::cont::ArrayHandlePermutation<IdImplicitType, IdHandleType> IdPermutationHandleType;


    typedef vtkm::cont::CellSetExplicit<UInt8PermutationHandleType::StorageTag,
                                        IdCompPermutationHandleType::StorageTag,
                                        typename IdPermutationHandleType::StorageTag> PermutedCellSetExplicit;

    UInt8PermutationHandleType pt1(face2CellId, shapes);
    IdCompPermutationHandleType pt2(face2CellId, numIndices);

    //Construct an augmented connectivity output array of length 4*totalFaces
    //Repeat the 4 cell vertices for each cell face: 4763 4763 4763 4763 (cell 1) | 4632 4632...(cell 2)...
    //First, compute indices into the original connectivity array
    IdImplicitType connIndices(GetConnIndex(4, 4), 4*totalFaces);
    IdPermutationHandleType faceConn(connIndices, conn);

    PermutedCellSetExplicit permutedCellSet;
    permutedCellSet.Fill(pt1, pt2, faceConn);
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 3> > faceVertices;
    vtkm::worklet::DispatcherMapTopology<GetFace> faceHashDispatcher;

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    faceHashDispatcher.Invoke(localFaceIds, permutedCellSet, faceVertices);
    //faceVertices Output: <476> <473> <463> <763> (cell 1) | <463> <462> <432> <632> (cell 2) ...
  #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "GetFace_Worklet," << timer.GetElapsedTime() << "\n";
#endif
    pt1.ReleaseResources();
    pt2.ReleaseResources();
    faceConn.ReleaseResources();
    face2CellId.ReleaseResources();
    localFaceIds.ReleaseResources();
    connIndices.ReleaseResources();

    //Sort the faces in ascending order by point/vertex indices
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(faceVertices, Id3LessThan());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "Sort_Adapter," << timer.GetElapsedTime() << "\n";
#endif

    //Search neighboring faces for duplicates - the internal faces
    vtkm::cont::ArrayHandle<vtkm::Id3> uniqueFaceVertices;
    vtkm::cont::ArrayHandle<vtkm::Id> uniqueFaceCounts;
    vtkm::cont::ArrayHandle<vtkm::Id3> externalFaces;
    vtkm::cont::ArrayHandleConstant<vtkm::Id> ones(1, totalFaces); //Initially all 1's

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ReduceByKey(faceVertices,
                                                                   ones,
                                                                   uniqueFaceVertices,
                                                                   uniqueFaceCounts,
                                                                   vtkm::internal::Add());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "ReduceByKey_Adapter," << timer.GetElapsedTime() << "\n";
#endif
    ones.ReleaseResources();
    faceVertices.ReleaseResources();

    //Removes all faces that have counts not equal to 1 (unity)
    //The faces with counts of 1 are the external faces
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::StreamCompact(uniqueFaceVertices,
                                                                     uniqueFaceCounts,
                                                                     externalFaces,
                                                                     IsUnity());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "StreamCompact_Adapter," << timer.GetElapsedTime() << "\n";
#endif
    uniqueFaceVertices.ReleaseResources();
    uniqueFaceCounts.ReleaseResources();

    //Generate output - the number of external faces
    const vtkm::Id output_numExtFaces = externalFaces.GetNumberOfValues();
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "Total External Faces = " << output_numExtFaces << std::endl;
#endif

    //Populate the output data arrays with just the external faces
    //A cell set of triangle faces for tetrahedral cells
    typename vtkm::cont::ArrayHandle<vtkm::Id3>::PortalConstControl extFacePortal =
              externalFaces.GetPortalConstControl();
    output_shapes.Allocate(output_numExtFaces);
    output_numIndices.Allocate(output_numExtFaces);
    output_conn.Allocate(3 * output_numExtFaces);
    for(int face = 0; face < output_numExtFaces; face++)
    {
      output_shapes.GetPortalControl().Set(face, vtkm::CELL_SHAPE_TRIANGLE);
      output_numIndices.GetPortalControl().Set(face, static_cast<vtkm::IdComponent>(3));
      output_conn.GetPortalControl().Set(3*face, extFacePortal.Get(face)[0]);
      output_conn.GetPortalControl().Set(3*face + 1, extFacePortal.Get(face)[1]);
      output_conn.GetPortalControl().Set(3*face + 2, extFacePortal.Get(face)[2]);
    }
    externalFaces.ReleaseResources();

    //End of algorithm
  }

}; //struct ExternalFaces


}} //namespace vtkm::worklet

#endif //vtk_m_worklet_ExternalFaces_h
