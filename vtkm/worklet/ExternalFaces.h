//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_ExternalFaces_h
#define vtk_m_worklet_ExternalFaces_h

#include <vtkm/CellShape.h>
#include <vtkm/Hash.h>
#include <vtkm/Math.h>

#include <vtkm/exec/CellFace.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

namespace vtkm
{
namespace worklet
{

struct ExternalFaces
{
  //Worklet that returns the number of external faces for each structured cell
  class NumExternalFacesPerStructuredCell : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet,
                                  FieldOut numFacesInCell,
                                  FieldInPoint pointCoordinates);
    using ExecutionSignature = _2(CellShape, _3);
    using InputDomain = _1;

    VTKM_CONT
    NumExternalFacesPerStructuredCell(const vtkm::Vec3f_64& min_point,
                                      const vtkm::Vec3f_64& max_point)
      : MinPoint(min_point)
      , MaxPoint(max_point)
    {
    }

    VTKM_EXEC
    inline vtkm::IdComponent CountExternalFacesOnDimension(vtkm::Float64 grid_min,
                                                           vtkm::Float64 grid_max,
                                                           vtkm::Float64 cell_min,
                                                           vtkm::Float64 cell_max) const
    {
      vtkm::IdComponent count = 0;

      bool cell_min_at_grid_boundary = cell_min <= grid_min;
      bool cell_max_at_grid_boundary = cell_max >= grid_max;

      if (cell_min_at_grid_boundary && !cell_max_at_grid_boundary)
      {
        count++;
      }
      else if (!cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        count++;
      }
      else if (cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        count += 2;
      }

      return count;
    }

    template <typename CellShapeTag, typename PointCoordVecType>
    VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape,
                                           const PointCoordVecType& pointCoordinates) const
    {
      (void)shape; // C4100 false positive workaround
      VTKM_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      vtkm::IdComponent count = 0;

      count += CountExternalFacesOnDimension(
        MinPoint[0], MaxPoint[0], pointCoordinates[0][0], pointCoordinates[1][0]);

      count += CountExternalFacesOnDimension(
        MinPoint[1], MaxPoint[1], pointCoordinates[0][1], pointCoordinates[3][1]);

      count += CountExternalFacesOnDimension(
        MinPoint[2], MaxPoint[2], pointCoordinates[0][2], pointCoordinates[4][2]);

      return count;
    }

  private:
    vtkm::Vec3f_64 MinPoint;
    vtkm::Vec3f_64 MaxPoint;
  };


  //Worklet that finds face connectivity for each structured cell
  class BuildConnectivityStructured : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet,
                                  WholeCellSetIn<> inputCell,
                                  FieldOut faceShapes,
                                  FieldOut facePointCount,
                                  FieldOut faceConnectivity,
                                  FieldInPoint pointCoordinates);
    using ExecutionSignature = void(CellShape, VisitIndex, InputIndex, _2, _3, _4, _5, _6);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename CountArrayType>
    VTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
      return ScatterType(countArray);
    }

    VTKM_CONT
    BuildConnectivityStructured(const vtkm::Vec3f_64& min_point, const vtkm::Vec3f_64& max_point)
      : MinPoint(min_point)
      , MaxPoint(max_point)
    {
    }

    enum FaceType
    {
      FACE_GRID_MIN,
      FACE_GRID_MAX,
      FACE_GRID_MIN_AND_MAX,
      FACE_NONE
    };

    VTKM_EXEC
    inline bool FoundFaceOnDimension(vtkm::Float64 grid_min,
                                     vtkm::Float64 grid_max,
                                     vtkm::Float64 cell_min,
                                     vtkm::Float64 cell_max,
                                     vtkm::IdComponent& faceIndex,
                                     vtkm::IdComponent& count,
                                     vtkm::IdComponent dimensionFaceOffset,
                                     vtkm::IdComponent visitIndex) const
    {
      bool cell_min_at_grid_boundary = cell_min <= grid_min;
      bool cell_max_at_grid_boundary = cell_max >= grid_max;

      FaceType Faces = FaceType::FACE_NONE;

      if (cell_min_at_grid_boundary && !cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MIN;
      }
      else if (!cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MAX;
      }
      else if (cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MIN_AND_MAX;
      }

      if (Faces == FaceType::FACE_NONE)
        return false;

      if (Faces == FaceType::FACE_GRID_MIN)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset;
          return true;
        }
        else
        {
          count++;
        }
      }
      else if (Faces == FaceType::FACE_GRID_MAX)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset + 1;
          return true;
        }
        else
        {
          count++;
        }
      }
      else if (Faces == FaceType::FACE_GRID_MIN_AND_MAX)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset;
          return true;
        }
        count++;
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset + 1;
          return true;
        }
        count++;
      }

      return false;
    }

    template <typename PointCoordVecType>
    VTKM_EXEC inline vtkm::IdComponent FindFaceIndexForVisit(
      vtkm::IdComponent visitIndex,
      const PointCoordVecType& pointCoordinates) const
    {
      vtkm::IdComponent count = 0;
      vtkm::IdComponent faceIndex = 0;
      // Search X dimension
      if (!FoundFaceOnDimension(MinPoint[0],
                                MaxPoint[0],
                                pointCoordinates[0][0],
                                pointCoordinates[1][0],
                                faceIndex,
                                count,
                                0,
                                visitIndex))
      {
        // Search Y dimension
        if (!FoundFaceOnDimension(MinPoint[1],
                                  MaxPoint[1],
                                  pointCoordinates[0][1],
                                  pointCoordinates[3][1],
                                  faceIndex,
                                  count,
                                  2,
                                  visitIndex))
        {
          // Search Z dimension
          FoundFaceOnDimension(MinPoint[2],
                               MaxPoint[2],
                               pointCoordinates[0][2],
                               pointCoordinates[4][2],
                               faceIndex,
                               count,
                               4,
                               visitIndex);
        }
      }
      return faceIndex;
    }

    template <typename CellShapeTag,
              typename CellSetType,
              typename PointCoordVecType,
              typename ConnectivityType>
    VTKM_EXEC void operator()(CellShapeTag shape,
                              vtkm::IdComponent visitIndex,
                              vtkm::Id inputIndex,
                              const CellSetType& cellSet,
                              vtkm::UInt8& shapeOut,
                              vtkm::IdComponent& numFacePointsOut,
                              ConnectivityType& faceConnectivity,
                              const PointCoordVecType& pointCoordinates) const
    {
      VTKM_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      vtkm::IdComponent faceIndex = FindFaceIndexForVisit(visitIndex, pointCoordinates);

      const vtkm::IdComponent numFacePoints =
        vtkm::exec::CellFaceNumberOfPoints(faceIndex, shape, *this);
      VTKM_ASSERT(numFacePoints == faceConnectivity.GetNumberOfComponents());

      typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(inputIndex);

      shapeOut = vtkm::CELL_SHAPE_QUAD;
      numFacePointsOut = 4;

      for (vtkm::IdComponent facePointIndex = 0; facePointIndex < numFacePoints; facePointIndex++)
      {
        faceConnectivity[facePointIndex] =
          inCellIndices[vtkm::exec::CellFaceLocalIndex(facePointIndex, faceIndex, shape, *this)];
      }
    }

  private:
    vtkm::Vec3f_64 MinPoint;
    vtkm::Vec3f_64 MaxPoint;
  };

  //Worklet that returns the number of faces for each cell/shape
  class NumFacesPerCell : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet, FieldOut numFacesInCell);
    using ExecutionSignature = _2(CellShape);
    using InputDomain = _1;

    template <typename CellShapeTag>
    VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape) const
    {
      return vtkm::exec::CellFaceNumberOfFaces(shape, *this);
    }
  };

  //Worklet that identifies a cell face by a hash value. Not necessarily completely unique.
  class FaceHash : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldOut faceHashes,
                                  FieldOut originCells,
                                  FieldOut originFaces);
    using ExecutionSignature = void(_2, _3, _4, CellShape, PointIndices, InputIndex, VisitIndex);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename CellShapeTag, typename CellNodeVecType>
    VTKM_EXEC void operator()(vtkm::HashType& faceHash,
                              vtkm::Id& cellIndex,
                              vtkm::IdComponent& faceIndex,
                              CellShapeTag shape,
                              const CellNodeVecType& cellNodeIds,
                              vtkm::Id inputIndex,
                              vtkm::IdComponent visitIndex) const
    {
      faceHash = vtkm::Hash(vtkm::exec::CellFaceCanonicalId(visitIndex, shape, cellNodeIds, *this));

      cellIndex = inputIndex;
      faceIndex = visitIndex;
    }
  };

  // Worklet that identifies the number of cells written out per face.
  // Because there can be collisions in the face ids, this instance might
  // represent multiple faces, which have to be checked. The resulting
  // number is the total number of external faces.
  class FaceCounts : public vtkm::worklet::WorkletReduceByKey
  {
  public:
    using ControlSignature = void(KeysIn keys,
                                  WholeCellSetIn<> inputCells,
                                  ValuesIn originCells,
                                  ValuesIn originFaces,
                                  ReducedValuesOut numOutputCells);
    using ExecutionSignature = _5(_2, _3, _4);
    using InputDomain = _1;

    template <typename CellSetType, typename OriginCellsType, typename OriginFacesType>
    VTKM_EXEC vtkm::IdComponent operator()(const CellSetType& cellSet,
                                           const OriginCellsType& originCells,
                                           const OriginFacesType& originFaces) const
    {
      vtkm::IdComponent numCellsOnHash = originCells.GetNumberOfComponents();
      VTKM_ASSERT(originFaces.GetNumberOfComponents() == numCellsOnHash);

      // Start by assuming all faces are unique, then remove one for each
      // face we find a duplicate for.
      vtkm::IdComponent numExternalFaces = numCellsOnHash;

      for (vtkm::IdComponent myIndex = 0;
           myIndex < numCellsOnHash - 1; // Don't need to check last face
           myIndex++)
      {
        vtkm::Id3 myFace =
          vtkm::exec::CellFaceCanonicalId(originFaces[myIndex],
                                          cellSet.GetCellShape(originCells[myIndex]),
                                          cellSet.GetIndices(originCells[myIndex]),
                                          *this);
        for (vtkm::IdComponent otherIndex = myIndex + 1; otherIndex < numCellsOnHash; otherIndex++)
        {
          vtkm::Id3 otherFace =
            vtkm::exec::CellFaceCanonicalId(originFaces[otherIndex],
                                            cellSet.GetCellShape(originCells[otherIndex]),
                                            cellSet.GetIndices(originCells[otherIndex]),
                                            *this);
          if (myFace == otherFace)
          {
            // Faces are the same. Must be internal. Remove 2, one for each face. We don't have to
            // worry about otherFace matching anything else because a proper topology will have at
            // most 2 cells sharing a face, so there should be no more matches.
            numExternalFaces -= 2;
            break;
          }
        }
      }

      return numExternalFaces;
    }
  };

private:
  // Resolves duplicate hashes by finding a specified unique face for a given hash.
  // Given a cell set (from a WholeCellSetIn) and the cell/face id pairs for each face
  // associated with a given hash, returns the index of the cell/face provided of the
  // visitIndex-th unique face. Basically, this method searches through all the cell/face
  // pairs looking for unique sets and returns the one associated with visitIndex.
  template <typename CellSetType, typename OriginCellsType, typename OriginFacesType>
  VTKM_EXEC static vtkm::IdComponent FindUniqueFace(const CellSetType& cellSet,
                                                    const OriginCellsType& originCells,
                                                    const OriginFacesType& originFaces,
                                                    vtkm::IdComponent visitIndex,
                                                    const vtkm::exec::FunctorBase* self)
  {
    vtkm::IdComponent numCellsOnHash = originCells.GetNumberOfComponents();
    VTKM_ASSERT(originFaces.GetNumberOfComponents() == numCellsOnHash);

    // Find the visitIndex-th unique face.
    vtkm::IdComponent numFound = 0;
    vtkm::IdComponent myIndex = 0;
    while (true)
    {
      VTKM_ASSERT(myIndex < numCellsOnHash);
      vtkm::Id3 myFace = vtkm::exec::CellFaceCanonicalId(originFaces[myIndex],
                                                         cellSet.GetCellShape(originCells[myIndex]),
                                                         cellSet.GetIndices(originCells[myIndex]),
                                                         *self);
      bool foundPair = false;
      for (vtkm::IdComponent otherIndex = myIndex + 1; otherIndex < numCellsOnHash; otherIndex++)
      {
        vtkm::Id3 otherFace =
          vtkm::exec::CellFaceCanonicalId(originFaces[otherIndex],
                                          cellSet.GetCellShape(originCells[otherIndex]),
                                          cellSet.GetIndices(originCells[otherIndex]),
                                          *self);
        if (myFace == otherFace)
        {
          // Faces are the same. Must be internal.
          foundPair = true;
          break;
        }
      }

      if (!foundPair)
      {
        if (numFound == visitIndex)
        {
          break;
        }
        else
        {
          numFound++;
        }
      }

      myIndex++;
    }

    return myIndex;
  }

public:
  // Worklet that returns the number of points for each outputted face.
  // Have to manage the case where multiple faces have the same hash.
  class NumPointsPerFace : public vtkm::worklet::WorkletReduceByKey
  {
  public:
    using ControlSignature = void(KeysIn keys,
                                  WholeCellSetIn<> inputCells,
                                  ValuesIn originCells,
                                  ValuesIn originFaces,
                                  ReducedValuesOut numPointsInFace);
    using ExecutionSignature = _5(_2, _3, _4, VisitIndex);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename CountArrayType>
    VTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
      return ScatterType(countArray);
    }

    template <typename CellSetType, typename OriginCellsType, typename OriginFacesType>
    VTKM_EXEC vtkm::IdComponent operator()(const CellSetType& cellSet,
                                           const OriginCellsType& originCells,
                                           const OriginFacesType& originFaces,
                                           vtkm::IdComponent visitIndex) const
    {
      vtkm::IdComponent myIndex =
        ExternalFaces::FindUniqueFace(cellSet, originCells, originFaces, visitIndex, this);

      return vtkm::exec::CellFaceNumberOfPoints(
        originFaces[myIndex], cellSet.GetCellShape(originCells[myIndex]), *this);
    }
  };

  // Worklet that returns the shape and connectivity for each external face
  class BuildConnectivity : public vtkm::worklet::WorkletReduceByKey
  {
  public:
    using ControlSignature = void(KeysIn keys,
                                  WholeCellSetIn<> inputCells,
                                  ValuesIn originCells,
                                  ValuesIn originFaces,
                                  ReducedValuesOut shapesOut,
                                  ReducedValuesOut connectivityOut,
                                  ReducedValuesOut cellIdMapOut);
    using ExecutionSignature = void(_2, _3, _4, VisitIndex, _5, _6, _7);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename CellSetType,
              typename OriginCellsType,
              typename OriginFacesType,
              typename ConnectivityType>
    VTKM_EXEC void operator()(const CellSetType& cellSet,
                              const OriginCellsType& originCells,
                              const OriginFacesType& originFaces,
                              vtkm::IdComponent visitIndex,
                              vtkm::UInt8& shapeOut,
                              ConnectivityType& connectivityOut,
                              vtkm::Id& cellIdMapOut) const
    {
      const vtkm::IdComponent myIndex =
        ExternalFaces::FindUniqueFace(cellSet, originCells, originFaces, visitIndex, this);
      const vtkm::IdComponent myFace = originFaces[myIndex];


      typename CellSetType::CellShapeTag shapeIn = cellSet.GetCellShape(originCells[myIndex]);
      shapeOut = vtkm::exec::CellFaceShape(myFace, shapeIn, *this);
      cellIdMapOut = originCells[myIndex];

      const vtkm::IdComponent numFacePoints =
        vtkm::exec::CellFaceNumberOfPoints(myFace, shapeIn, *this);

      VTKM_ASSERT(numFacePoints == connectivityOut.GetNumberOfComponents());

      typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(originCells[myIndex]);

      for (vtkm::IdComponent facePointIndex = 0; facePointIndex < numFacePoints; facePointIndex++)
      {
        connectivityOut[facePointIndex] =
          inCellIndices[vtkm::exec::CellFaceLocalIndex(facePointIndex, myFace, shapeIn, *this)];
      }
    }
  };

  class IsPolyDataCell : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet, FieldOut isPolyDataCell);
    using ExecutionSignature = _2(CellShape);
    using InputDomain = _1;

    template <typename CellShapeTag>
    VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape) const
    {
      return !vtkm::exec::CellFaceNumberOfFaces(shape, *this);
    }
  };

  class CountPolyDataCellPoints : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ScatterType = vtkm::worklet::ScatterCounting;

    using ControlSignature = void(CellSetIn inCellSet, FieldOut numPoints);
    using ExecutionSignature = _2(PointCount);
    using InputDomain = _1;

    VTKM_EXEC vtkm::Id operator()(vtkm::Id count) const { return count; }
  };

  class PassPolyDataCells : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ScatterType = vtkm::worklet::ScatterCounting;

    using ControlSignature = void(CellSetIn inputTopology,
                                  FieldOut shapes,
                                  FieldOut pointIndices,
                                  FieldOut cellIdMapOut);
    using ExecutionSignature = void(CellShape, PointIndices, InputIndex, _2, _3, _4);

    template <typename CellShape, typename InPointIndexType, typename OutPointIndexType>
    VTKM_EXEC void operator()(const CellShape& inShape,
                              const InPointIndexType& inPoints,
                              vtkm::Id inputIndex,
                              vtkm::UInt8& outShape,
                              OutPointIndexType& outPoints,
                              vtkm::Id& cellIdMapOut) const
    {
      cellIdMapOut = inputIndex;
      outShape = inShape.Id;

      vtkm::IdComponent numPoints = inPoints.GetNumberOfComponents();
      VTKM_ASSERT(numPoints == outPoints.GetNumberOfComponents());
      for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        outPoints[pointIndex] = inPoints[pointIndex];
      }
    }
  };

  template <typename T>
  struct BiasFunctor
  {
    VTKM_EXEC_CONT
    BiasFunctor(T bias = T(0))
      : Bias(bias)
    {
    }

    VTKM_EXEC_CONT
    T operator()(T x) const { return x + this->Bias; }

    T Bias;
  };

public:
  VTKM_CONT
  ExternalFaces()
    : PassPolyData(true)
  {
  }

  VTKM_CONT
  void SetPassPolyData(bool flag) { this->PassPolyData = flag; }

  VTKM_CONT
  bool GetPassPolyData() const { return this->PassPolyData; }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& in) const
  {

    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->CellIdMap, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    vtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

  void ReleaseCellMapArrays() { this->CellIdMap.ReleaseResources(); }


  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry for regular grids.
  ///
  /// Faster Run() method for uniform and rectilinear grid types.
  /// Uses grid extents to find cells on the boundaries of the grid.
  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT void Run(
    const vtkm::cont::CellSetStructured<3>& inCellSet,
    const vtkm::cont::CoordinateSystem& coord,
    vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    vtkm::Vec3f_64 MinPoint;
    vtkm::Vec3f_64 MaxPoint;

    vtkm::Id3 PointDimensions = inCellSet.GetPointDimensions();

    using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
    using CartesianArrayHandle =
      vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

    auto coordData = coord.GetData();
    if (coordData.IsType<CartesianArrayHandle>())
    {
      const auto vertices = coordData.Cast<CartesianArrayHandle>();
      const auto vertsSize = vertices.GetNumberOfValues();
      const auto tmp = vtkm::cont::ArrayGetValues({ 0, vertsSize - 1 }, vertices);
      MinPoint = tmp[0];
      MaxPoint = tmp[1];
    }
    else
    {
      auto vertices = coordData.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      auto Coordinates = vertices.GetPortalConstControl();

      MinPoint = Coordinates.GetOrigin();
      vtkm::Vec3f_64 spacing = Coordinates.GetSpacing();

      vtkm::Vec3f_64 unitLength;
      unitLength[0] = static_cast<vtkm::Float64>(PointDimensions[0] - 1);
      unitLength[1] = static_cast<vtkm::Float64>(PointDimensions[1] - 1);
      unitLength[2] = static_cast<vtkm::Float64>(PointDimensions[2] - 1);
      MaxPoint = MinPoint + spacing * unitLength;
    }

    // Create a worklet to count the number of external faces on each cell
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numExternalFaces;
    vtkm::worklet::DispatcherMapTopology<NumExternalFacesPerStructuredCell>
      numExternalFacesDispatcher((NumExternalFacesPerStructuredCell(MinPoint, MaxPoint)));

    numExternalFacesDispatcher.Invoke(inCellSet, numExternalFaces, coordData);

    vtkm::Id numberOfExternalFaces =
      vtkm::cont::Algorithm::Reduce(numExternalFaces, 0, vtkm::Sum());

    auto scatterCellToExternalFace = BuildConnectivityStructured::MakeScatter(numExternalFaces);

    // Maps output cells to input cells. Store this for cell field mapping.
    this->CellIdMap = scatterCellToExternalFace.GetOutputToInputMap();

    numExternalFaces.ReleaseResources();

    vtkm::Id connectivitySize = 4 * numberOfExternalFaces;
    vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage> faceConnectivity;
    vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage> faceShapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> facePointCount;
    // Must pre allocate because worklet invocation will not have enough
    // information to.
    faceConnectivity.Allocate(connectivitySize);

    vtkm::worklet::DispatcherMapTopology<BuildConnectivityStructured>
      buildConnectivityStructuredDispatcher(BuildConnectivityStructured(MinPoint, MaxPoint),
                                            scatterCellToExternalFace);

    buildConnectivityStructuredDispatcher.Invoke(
      inCellSet,
      inCellSet,
      faceShapes,
      facePointCount,
      vtkm::cont::make_ArrayHandleGroupVec<4>(faceConnectivity),
      coordData);

    auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(facePointCount);

    outCellSet.Fill(inCellSet.GetNumberOfPoints(), faceShapes, faceConnectivity, offsets);
  }

  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry
  template <typename InCellSetType,
            typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage>
  VTKM_CONT void Run(
    const InCellSetType& inCellSet,
    vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    using PointCountArrayType = vtkm::cont::ArrayHandle<vtkm::IdComponent>;
    using ShapeArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage>;
    using OffsetsArrayType = vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>;
    using ConnectivityArrayType = vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage>;

    //Create a worklet to map the number of faces to each cell
    vtkm::cont::ArrayHandle<vtkm::IdComponent> facesPerCell;
    vtkm::worklet::DispatcherMapTopology<NumFacesPerCell> numFacesDispatcher;

    numFacesDispatcher.Invoke(inCellSet, facesPerCell);

    vtkm::worklet::ScatterCounting scatterCellToFace(facesPerCell);
    facesPerCell.ReleaseResources();

    PointCountArrayType polyDataPointCount;
    ShapeArrayType polyDataShapes;
    OffsetsArrayType polyDataOffsets;
    ConnectivityArrayType polyDataConnectivity;
    vtkm::cont::ArrayHandle<vtkm::Id> polyDataCellIdMap;
    vtkm::Id polyDataConnectivitySize = 0;
    if (this->PassPolyData)
    {
      vtkm::cont::ArrayHandle<vtkm::IdComponent> isPolyDataCell;
      vtkm::worklet::DispatcherMapTopology<IsPolyDataCell> isPolyDataCellDispatcher;

      isPolyDataCellDispatcher.Invoke(inCellSet, isPolyDataCell);

      vtkm::worklet::ScatterCounting scatterPolyDataCells(isPolyDataCell);

      isPolyDataCell.ReleaseResources();

      if (scatterPolyDataCells.GetOutputRange(inCellSet.GetNumberOfCells()) != 0)
      {
        vtkm::worklet::DispatcherMapTopology<CountPolyDataCellPoints>
          countPolyDataCellPointsDispatcher(scatterPolyDataCells);

        countPolyDataCellPointsDispatcher.Invoke(inCellSet, polyDataPointCount);

        vtkm::cont::ConvertNumIndicesToOffsets(
          polyDataPointCount, polyDataOffsets, polyDataConnectivitySize);

        vtkm::worklet::DispatcherMapTopology<PassPolyDataCells> passPolyDataCellsDispatcher(
          scatterPolyDataCells);

        polyDataConnectivity.Allocate(polyDataConnectivitySize);

        auto pdOffsetsTrim = vtkm::cont::make_ArrayHandleView(
          polyDataOffsets, 0, polyDataOffsets.GetNumberOfValues() - 1);

        passPolyDataCellsDispatcher.Invoke(
          inCellSet,
          polyDataShapes,
          vtkm::cont::make_ArrayHandleGroupVecVariable(polyDataConnectivity, pdOffsetsTrim),
          polyDataCellIdMap);
      }
    }

    if (scatterCellToFace.GetOutputRange(inCellSet.GetNumberOfCells()) == 0)
    {
      if (!polyDataConnectivitySize)
      {
        // Data has no faces. Output is empty.
        outCellSet.PrepareToAddCells(0, 0);
        outCellSet.CompleteAddingCells(inCellSet.GetNumberOfPoints());
        return;
      }
      else
      {
        // Pass only input poly data to output
        outCellSet.Fill(
          inCellSet.GetNumberOfPoints(), polyDataShapes, polyDataConnectivity, polyDataOffsets);
        this->CellIdMap = polyDataCellIdMap;
        return;
      }
    }

    vtkm::cont::ArrayHandle<vtkm::HashType> faceHashes;
    vtkm::cont::ArrayHandle<vtkm::Id> originCells;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> originFaces;
    vtkm::worklet::DispatcherMapTopology<FaceHash> faceHashDispatcher(scatterCellToFace);

    faceHashDispatcher.Invoke(inCellSet, faceHashes, originCells, originFaces);

    vtkm::worklet::Keys<vtkm::HashType> faceKeys(faceHashes);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> faceOutputCount;
    vtkm::worklet::DispatcherReduceByKey<FaceCounts> faceCountDispatcher;

    faceCountDispatcher.Invoke(faceKeys, inCellSet, originCells, originFaces, faceOutputCount);

    auto scatterCullInternalFaces = NumPointsPerFace::MakeScatter(faceOutputCount);

    PointCountArrayType facePointCount;
    vtkm::worklet::DispatcherReduceByKey<NumPointsPerFace> pointsPerFaceDispatcher(
      scatterCullInternalFaces);

    pointsPerFaceDispatcher.Invoke(faceKeys, inCellSet, originCells, originFaces, facePointCount);

    ShapeArrayType faceShapes;

    OffsetsArrayType faceOffsets;
    vtkm::Id connectivitySize;
    vtkm::cont::ConvertNumIndicesToOffsets(facePointCount, faceOffsets, connectivitySize);

    ConnectivityArrayType faceConnectivity;
    // Must pre allocate because worklet invocation will not have enough
    // information to.
    faceConnectivity.Allocate(connectivitySize);

    vtkm::worklet::DispatcherReduceByKey<BuildConnectivity> buildConnectivityDispatcher(
      scatterCullInternalFaces);

    vtkm::cont::ArrayHandle<vtkm::Id> faceToCellIdMap;

    // Create a view that doesn't have the last offset:
    auto faceOffsetsTrim =
      vtkm::cont::make_ArrayHandleView(faceOffsets, 0, faceOffsets.GetNumberOfValues() - 1);

    buildConnectivityDispatcher.Invoke(
      faceKeys,
      inCellSet,
      originCells,
      originFaces,
      faceShapes,
      vtkm::cont::make_ArrayHandleGroupVecVariable(faceConnectivity, faceOffsetsTrim),
      faceToCellIdMap);

    if (!polyDataConnectivitySize)
    {
      outCellSet.Fill(inCellSet.GetNumberOfPoints(), faceShapes, faceConnectivity, faceOffsets);
      this->CellIdMap = faceToCellIdMap;
    }
    else
    {
      // Join poly data to face data output
      vtkm::cont::ArrayHandleConcatenate<ShapeArrayType, ShapeArrayType> faceShapesArray(
        faceShapes, polyDataShapes);
      ShapeArrayType joinedShapesArray;
      vtkm::cont::ArrayCopy(faceShapesArray, joinedShapesArray);

      vtkm::cont::ArrayHandleConcatenate<PointCountArrayType, PointCountArrayType> pointCountArray(
        facePointCount, polyDataPointCount);
      PointCountArrayType joinedPointCountArray;
      vtkm::cont::ArrayCopy(pointCountArray, joinedPointCountArray);

      vtkm::cont::ArrayHandleConcatenate<ConnectivityArrayType, ConnectivityArrayType>
        connectivityArray(faceConnectivity, polyDataConnectivity);
      ConnectivityArrayType joinedConnectivity;
      vtkm::cont::ArrayCopy(connectivityArray, joinedConnectivity);

      // Adjust poly data offsets array with face connectivity size before join
      auto adjustedPolyDataOffsets = vtkm::cont::make_ArrayHandleTransform(
        polyDataOffsets, BiasFunctor<vtkm::Id>(faceConnectivity.GetNumberOfValues()));

      auto offsetsArray =
        vtkm::cont::make_ArrayHandleConcatenate(faceOffsetsTrim, adjustedPolyDataOffsets);
      OffsetsArrayType joinedOffsets;
      vtkm::cont::ArrayCopy(offsetsArray, joinedOffsets);

      vtkm::cont::ArrayHandleConcatenate<vtkm::cont::ArrayHandle<vtkm::Id>,
                                         vtkm::cont::ArrayHandle<vtkm::Id>>
        cellIdMapArray(faceToCellIdMap, polyDataCellIdMap);
      vtkm::cont::ArrayHandle<vtkm::Id> joinedCellIdMap;
      vtkm::cont::ArrayCopy(cellIdMapArray, joinedCellIdMap);

      outCellSet.Fill(
        inCellSet.GetNumberOfPoints(), joinedShapesArray, joinedConnectivity, joinedOffsets);
      this->CellIdMap = joinedCellIdMap;
    }
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
  bool PassPolyData;

}; //struct ExternalFaces
}
} //namespace vtkm::worklet

#endif //vtk_m_worklet_ExternalFaces_h
