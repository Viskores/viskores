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

#include <vtkm/Math.h>
#include <vtkm/CellType.h>



#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/ExplicitConnectivity.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/Assert.h>
#include <vtkm/exec/arg/TopologyIdSet.h>
#include <vtkm/exec/arg/TopologyIdCount.h>
#include <vtkm/exec/arg/TopologyElementType.h>

#define __VTKM_EXTERNAL_FACES_BENCHMARK

namespace vtkm
{
namespace worklet
{

  template<typename DeviceAdapter>
  struct ExternalFaces{
    //Unary predicate operator
    //Returns True if the argument is equal to 1; False otherwise.
    struct IsUnity
    {
        template<typename T>
        VTKM_EXEC_CONT_EXPORT bool operator()(const T &x) const
        {
            return x == 1;
        }
    };

    //Binary operator
    //Returns (a-b) mod c
    class Subtract : public vtkm::worklet::WorkletMapField
    {
        private:
            vtkm::Id modulus;

        public:
            typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>);
            typedef _3 ExecutionSignature(_1, _2);

            VTKM_CONT_EXPORT
            Subtract(const vtkm::Id &c) : modulus(c) { };

            template<typename T>
            VTKM_EXEC_CONT_EXPORT
            T operator()(const T &a, const T &b) const
            {
                return (a - b) % modulus;
            }
    };

    //Worklet that returns the number of faces for each cell/shape
    class NumFacesPerCell : public vtkm::worklet::WorkletMapField
    {
        public:
            typedef void ControlSignature(FieldIn<>, FieldOut<>);
            typedef _2 ExecutionSignature(_1);
            typedef _1 InputDomain;

            VTKM_CONT_EXPORT
            NumFacesPerCell() { };

            template<typename T>
            VTKM_EXEC_EXPORT
            T operator()(const T &cellType) const
            {
                if (cellType == vtkm::VTKM_TETRA) return 4;
                else if (cellType == vtkm::VTKM_PYRAMID) return 5;
                else if (cellType == vtkm::VTKM_WEDGE) return 5;
                else if (cellType == vtkm::VTKM_HEXAHEDRON) return 6;
                else return -1;
            }
    };


    //Worklet that returns a hash key for a face
    class FaceHashKey : public vtkm::worklet::WorkletMapTopology
    {
        static const int LEN_IDS = 4; //The max num of nodes in a cell

        public:
            typedef void ControlSignature(//FieldSrcIn<Scalar> inNodes,
                                          FieldDestIn<AllTypes> localFaceIds,
                                          TopologyIn<LEN_IDS> topology,
                                          FieldDestOut<AllTypes> faceHashes
                                          );
            typedef void ExecutionSignature(//_1, _2, _4, _5, _6,
                                            _1, _3,
                                            vtkm::exec::arg::TopologyIdCount,
                                            vtkm::exec::arg::TopologyElementType,
                                            vtkm::exec::arg::TopologyIdSet);
            //typedef _3 InputDomain;
            typedef _2 InputDomain;

            VTKM_CONT_EXPORT
            FaceHashKey() { };

            template<typename T>
            VTKM_EXEC_EXPORT
            void operator()(//const vtkm::exec::TopologyData<T,LEN_IDS> & vtkmNotUsed(nodevals),
                            const T &cellFaceId,
                            T &faceHash,
                            const vtkm::Id & vtkmNotUsed(numNodes),
                            const vtkm::Id &cellType,
                            const vtkm::exec::TopologyData<vtkm::Id,LEN_IDS> &cellNodeIds) const
            {
                if (cellType == vtkm::VTKM_TETRA)
                {
                    //Assign cell points/nodes to this face
                    vtkm::Id faceP1, faceP2, faceP3;
                    if(cellFaceId == 0)
                    {
                        //Face A: (0, 1, 2)
                        faceP1 = cellNodeIds[0];
                        faceP2 = cellNodeIds[1];
                        faceP3 = cellNodeIds[2];
                    }
                    else if (cellFaceId == 1)
                    {
                        //Face B: (0, 1, 3)
                        faceP1 = cellNodeIds[0];
                        faceP2 = cellNodeIds[1];
                        faceP3 = cellNodeIds[3];
                    }
                    else if (cellFaceId == 2)
                    {
                        //Face C: (0, 2, 3)
                        faceP1 = cellNodeIds[0];
                        faceP2 = cellNodeIds[2];
                        faceP3 = cellNodeIds[3];
                    }
                    else if (cellFaceId == 3)
                    {
                        //Face D: (1, 2, 3)
                        faceP1 = cellNodeIds[1];
                        faceP2 = cellNodeIds[2];
                        faceP3 = cellNodeIds[3];
                    }

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

                    //Calculate a hash key for the sorted points of this face
                    unsigned int h  = 2166136261;
                    for(int i = 0; i < 3; i++)
                        h = (h * 16777619) ^ sorted[i];

                    faceHash = h;
                }
            }
    };

    public:

      template <typename StorageT,
                typename StorageU,
                typename StorageV>
      void run(const vtkm::cont::ArrayHandle<vtkm::Id, StorageT> shapes,
               const vtkm::cont::ArrayHandle<vtkm::Id, StorageU> numIndices,
               const vtkm::cont::ArrayHandle<vtkm::Id, StorageV> conn,
               vtkm::Id &output_numExtFaces)
    {

      //Create a worklet to map the number of faces to each cell
      vtkm::cont::ArrayHandle<vtkm::Id> facesPerCell;
      vtkm::worklet::DispatcherMapField<NumFacesPerCell> numFacesDispatcher;

      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        vtkm::cont::DeviceAdapterTimerImplementation<DeviceAdapter> timer;
      #endif
      numFacesDispatcher.Invoke(shapes, facesPerCell);
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "NumFacesWorklet," << timer.GetElapsedTime() << "\n";
      #endif

      //Exclusive scan of the number of faces per cell
      vtkm::cont::ArrayHandle<vtkm::Id> numFacesPrefixSum;
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
      #endif
      const vtkm::Id totalFaces =
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanInclusive(facesPerCell,
                                                                       numFacesPrefixSum);
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "ScanExclusive," << timer.GetElapsedTime() << "\n";
      #endif
      if(totalFaces == 0) return;

      //Generate reverse lookup: face index to cell index
      //terminate if no cells have triangles left
      //For 2 tetrahedron cells with 4 faces each (array of size 8): 0,0,0,0,1,1,1,1
      vtkm::cont::ArrayHandle<vtkm::Id> face2CellId;
      vtkm::cont::ArrayHandle<vtkm::Id> localFaceIds;
      localFaceIds.Allocate(static_cast<vtkm::Id>(totalFaces));
      vtkm::cont::ArrayHandleCounting<vtkm::Id> countingArray(vtkm::Id(0), vtkm::Id(totalFaces));
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
      #endif
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::UpperBounds(numFacesPrefixSum,
                                                                   countingArray,
                                                                   face2CellId);
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "UpperBounds," << timer.GetElapsedTime() << "\n";
      #endif
      vtkm::worklet::DispatcherMapField<Subtract> subtractDispatcher(Subtract(4));
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
      #endif
      subtractDispatcher.Invoke(countingArray, face2CellId, localFaceIds);
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "SubtractWorklet," << timer.GetElapsedTime() << "\n";
      #endif
      countingArray.ReleaseResources();

      //Construct a connectivity array of length 4*totalFaces (4 repeat entries for each tet)
      vtkm::cont::ArrayHandle<vtkm::Id> faceConn;
      typename vtkm::cont::ArrayHandle<vtkm::Id>::PortalConstControl portal =
                conn.GetPortalConstControl();
      faceConn.Allocate(static_cast<vtkm::Id>(4 * totalFaces));
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
      #endif
      int index = 0;
      for(int i = 0; i < facesPerCell.GetNumberOfValues(); i++)
          for(int j = 0; j < facesPerCell.GetPortalConstControl().Get(i); j++)
              for(int k = 0; k < 4; k++)
                  faceConn.GetPortalControl().Set(index++, portal.Get(4*i + k));
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "FaceConnectivityLoop," << timer.GetElapsedTime() << "\n";
      #endif

      //Calculate a hash key for each cell face
      typedef vtkm::cont::ArrayHandle<vtkm::Id> IdHandleType;
      typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, IdHandleType> IdPermutationType;
      typedef vtkm::cont::ExplicitConnectivity<IdPermutationType::StorageTag,
                                               IdPermutationType::StorageTag,
                                               IdHandleType::StorageTag> PermutedExplicitConnectivity;
      IdPermutationType pt1(face2CellId, shapes);
      IdPermutationType pt2(face2CellId, numIndices);
      PermutedExplicitConnectivity permConn;
      permConn.Fill(pt1, pt2, faceConn);
      vtkm::cont::ArrayHandle<vtkm::Id> faceHashes;
      vtkm::worklet::DispatcherMapTopology<FaceHashKey> faceHashDispatcher;
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
      #endif
      faceHashDispatcher.Invoke(localFaceIds, permConn, faceHashes);
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "FaceHashKeyWorklet," << timer.GetElapsedTime() << "\n";
      #endif
      face2CellId.ReleaseResources();
      localFaceIds.ReleaseResources();

      //Sort the faces in ascending order by hash key
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
      #endif
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(faceHashes);
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "Sort," << timer.GetElapsedTime() << "\n";
      #endif

      //Search neighboring faces/hashes for duplicates - the internal faces
      vtkm::cont::ArrayHandle<vtkm::Id> uniqueFaceHashes;
      vtkm::cont::ArrayHandle<vtkm::Id> uniqueHashCounts;
      vtkm::cont::ArrayHandle<vtkm::Id> externalFaceHashes;
      vtkm::cont::ArrayHandleConstant<vtkm::Id> ones(1, totalFaces); //Initially all 1's
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
      #endif
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ReduceByKey(faceHashes,
                                                                     ones,
                                                                     uniqueFaceHashes,
                                                                     uniqueHashCounts,
                                                                     vtkm::internal::Add());
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "ReduceByKey," << timer.GetElapsedTime() << "\n";
      #endif
      faceHashes.ReleaseResources();
      ones.ReleaseResources();

      //Removes all faces/keys that have counts not equal to 1 (unity)
      //The faces with counts of 1 are the external faces
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
      #endif
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::StreamCompact(uniqueFaceHashes,
                                                                       uniqueHashCounts,
                                                                       externalFaceHashes,
                                                                       IsUnity());
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "StreamCompact," << timer.GetElapsedTime() << "\n";
      #endif
      uniqueFaceHashes.ReleaseResources();
      uniqueHashCounts.ReleaseResources();

      //Generate output - the number of external faces
      output_numExtFaces = externalFaceHashes.GetNumberOfValues();
      #ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "Total External Faces = " << output_numExtFaces << std::endl;
      #endif
      externalFaceHashes.ReleaseResources();

      //End of algorithm
    }

}; //struct ExternalFaces


}} //namespace vtkm::worklet

#endif //vtk_m_worklet_ExternalFaces_h
