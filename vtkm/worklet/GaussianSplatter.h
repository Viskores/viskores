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
#ifndef vtk_m_worklet_GaussianSplatter_h
#define vtk_m_worklet_GaussianSplatter_h

#include <vtkm/Math.h>

#include <vtkm/exec/ExecutionWholeArray.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#define __VTKM_GAUSSIAN_SPLATTER_BENCHMARK

namespace vtkm
{
namespace worklet
{

  template<typename DeviceAdapter>
  struct GaussianSplatter
  {

    //Return True if the bounds range of the first vector argument
    //is less than or equal to that of the second vector argument; otherwise, False
    struct DimBoundsCompare
    {
       template<typename T>
       VTKM_EXEC_CONT_EXPORT
       bool operator()(const vtkm::Vec<T,2> &a,
                       const vtkm::Vec<T,2> &b) const
       {
           bool isLessThan = false;
           if((a[1] - a[0]) <= (b[1] - b[0]))
           {
             isLessThan = true;
           }
           return isLessThan;
       }
    };

    //Return a "local" Id of a voxel within a splat point's footprint.
    //A splat point that affects 5 neighboring voxel gridpoints would
    //have local Ids 0,1,2,3,4
    class ComputeLocalNeighborId : public vtkm::worklet::WorkletMapField
    {
      typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>);
      typedef void ExecutionSignature(_1, _2, WorkIndex, _3);

      VTKM_CONT_EXPORT
      ComputeLocalNeighborId() {}

       template<typename T>
       VTKM_EXEC_CONT_EXPORT
       void operator()(const T &modulus, const T &splatPointId,
                    const vtkm::Id &index, T &localId) const
       {
           localId = (index - splatPointId) % modulus;
       }
    };

    //For each volume dimension, set the spacing and splat distance properties
    class ConfigureVolumeProps : public vtkm::worklet::WorkletMapField
    {
        private:
            vtkm::Float64 MaxDist;

        public:
            typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>, FieldOut<>);
            typedef void ExecutionSignature(_1, _2, _3, _4);

            VTKM_CONT_EXPORT
            ConfigureVolumeProps() {}

            VTKM_CONT_EXPORT
            ConfigureVolumeProps(const vtkm::Float64 &c) : MaxDist(c) {}

            template<typename S, typename T>
            VTKM_EXEC_CONT_EXPORT
            void operator()(const S &dimBounds,
                            const T &sampleDimValue,
                            T &spacing,
                            T &splatDistance) const
            {
                //Set the spacing
                spacing = static_cast<T>((dimBounds[1] - dimBounds[0]) / (sampleDimValue - 1));
                if (spacing <= T(0.0))
                {
                  spacing = T(1.0);
                }

                //Set the splat distance
                splatDistance = static_cast<T>(this->MaxDist / spacing);
            }
      };

      //Return the splat footprint/neighborhood of each sample point, as
      //represented by min and max boundaries in each dimension.  Also
      //return the size of this footprint and the gridpoint coordinates
      //of the splat point.
      class GetFootprint : public vtkm::worklet::WorkletMapField
      {
          private:
              vtkm::Vec<vtkm::Float64, 3> Spacing;
              vtkm::Vec<vtkm::Float64, 3> SplatDist;
              vtkm::Vec<vtkm::Float64, 3> Origin;
              vtkm::Id3 VolumeDimensions;

          public:
              typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldIn<>,
                                            FieldOut<>, FieldOut<>, FieldOut<>, FieldOut<>);
              typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);

              VTKM_CONT_EXPORT
              GetFootprint(const vtkm::Vec<vtkm::Float64, 3> &s,
                           const vtkm::Vec<vtkm::Float64, 3> &sd,
                           const vtkm::Vec<vtkm::Float64, 3> &o,
                           const vtkm::Id3 &dim)
                            : Spacing(s), SplatDist(sd), Origin(o),
                              VolumeDimensions(dim) { }

              template<typename T>
              VTKM_EXEC_CONT_EXPORT
              void operator()(const T &xValue,
                              const T &yValue,
                              const T &zValue,
                              vtkm::Id3 &splatPoint,
                              vtkm::Id3 &minFootprint,
                              vtkm::Id3 &maxFootprint,
                              vtkm::Id &footprintSize) const
              {
                  vtkm::Id3 splat, min, max;
                  vtkm::Vec<vtkm::Float64, 3> sample = vtkm::make_Vec(xValue, yValue, zValue);
                  vtkm::Id size = 1;
                  for(int i = 0; i < 3; i++)
                  {
                      splat[i] = static_cast<vtkm::Id>((sample[i] - this->Origin[i]) / this->Spacing[i]);
                      min[i] = static_cast<vtkm::Id>(floor(static_cast<double>(splat[i])-this->SplatDist[i]));
                      max[i] = static_cast<vtkm::Id>(ceil(static_cast<double>(splat[i])+this->SplatDist[i]));
                      if( min[i] < 0 )
                      {
                          min[i] = 0;
                      }
                      if( max[i] >= this->VolumeDimensions[i] )
                      {
                          max[i] = this->VolumeDimensions[i] - 1;
                      }
                      size = size * (max[i] - min[i]);
                  }
                  splatPoint = splat;
                  minFootprint = min;
                  maxFootprint = max;
                  footprintSize = size;
              }
        };

        //Compute the Gaussian splatter value of the input voxel
        //gridpoint.  The Id of this point within the volume is
        //also determined.
        class GetSplatValue : public vtkm::worklet::WorkletMapField
        {
            private:
                vtkm::Vec<vtkm::Float64, 3> Spacing;
                vtkm::Vec<vtkm::Float64, 3> Origin;
                vtkm::Id3 VolumeDim;
                vtkm::Float64 Radius2;
                vtkm::Float64 ExponentFactor;
                vtkm::Float64 ScalingFactor;

            public:
                typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldIn<>,
                                              FieldIn<>, FieldIn<>, FieldOut<>,
                                              FieldOut<>);
                typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);

                VTKM_CONT_EXPORT
                GetSplatValue(const vtkm::Vec<vtkm::Float64, 3> &s,
                              const vtkm::Vec<vtkm::Float64, 3> &orig,
                              const vtkm::Id3 &dim,
                              const vtkm::Float64 &rad,
                              const vtkm::Float64 &ef,
                              const vtkm::Float64 &sf)
                              : Spacing(s), Origin(orig), VolumeDim(dim),
                                Radius2(rad), ExponentFactor(ef), ScalingFactor(sf) {}

                template<typename T>
                VTKM_EXEC_CONT_EXPORT
                void operator()(const T &splatPoint,
                                const T &minBound,
                                const T &maxBound,
                                const vtkm::Id splatPointId,
                                const vtkm::Id localNeighborId,
                                vtkm::Id &neighborVoxelId,
                                vtkm::Float64 &splatValue) const
                {
                    vtkm::Id yRange = maxBound[1] - minBound[1];
                    vtkm::Id xRange = maxBound[0] - minBound[0];
                    vtkm::Id divisor = yRange * xRange;
                    vtkm::Id i = localNeighborId / divisor;
                    vtkm::Id remainder = localNeighborId % divisor;
                    vtkm::Id j = remainder / xRange;
                    vtkm::Id k = remainder % xRange;
                    vtkm::Id3 neighbor;
                    neighbor[2] = this->Origin[2] + this->Spacing[2]*i;
                    neighbor[1] = this->Origin[1] + this->Spacing[1]*j;
                    neighbor[0] = this->Origin[0] + this->Spacing[0]*k;

                    //Compute Gaussian splat value
                    splatValue = 0.0;
                    vtkm::Float64 dist2 = ((neighbor[0]-splatPoint[0])*(neighbor[0]-splatPoint[0]) +
                                           (neighbor[1]-splatPoint[1])*(neighbor[1]-splatPoint[1]) +
                                           (neighbor[2]-splatPoint[2])*(neighbor[2]-splatPoint[2]));

                    if(dist2 <= this->Radius2)
                    {
                        splatValue = this->SpacingFactor *
                                     vtkm::Exp((this->ExponentFactor*(dist2)/this->Radius2));

                    }
                    neighborVoxelId = (neighbor[0]*VolumeDim[1]*VolumeDim[2]) +
                                      (neighbor[1]*VolumeDim[2]) + neighbor[2];
                }
          };

      //Given an index of a gridpoint within the volume bounding
      //box, return the corresponding coordinates of this point.
      class GetVolumeCoords : public vtkm::worklet::WorkletMapField
      {
          private:
            vtkm::Id3 Dimensions;

          public:

            typedef void ControlSignature(FieldIn<>, FieldOut<>, FieldOut<>);
            typedef void ExecutionSignature(_1, _2, _3);

            VTKM_CONT_EXPORT
            GetVolumeCoords(const vtkm::Id3 &d) : Dimensions(d) {}

            template<typename T>
            VTKM_EXEC_CONT_EXPORT
            void operator()(const vtkm::Id &index,
                            T &voxelCoords,
                            vtkm::Float64 &splatValue) const
            {
                vtkm::Id divisor = Dimensions[1] * Dimensions[2];
                vtkm::Id x = index / divisor;
                vtkm::Id remainder = index % divisor;
                vtkm::Id y = remainder / Dimensions[2];
                vtkm::Id z = remainder % Dimensions[2];
                voxelCoords = vtkm::make_Vec(x, y, z);
                splatValue = 0.0;
            }
      };

      //Scatter worklet that writes a splat value into the larger,
      //master splat value array, using the splat value's voxel Id
      //as an index.
      class UpdateVoxelSplats : public vtkm::worklet::WorkletMapField
      {
            typedef void ControlSignature(FieldIn<>, FieldIn<>, ExecObject, FieldOut<>);
            typedef void ExecutionSignature(_1, _2, _3, _4);

            VTKM_CONT_EXPORT
            UpdateVoxelSplats() {}


            VTKM_EXEC_CONT_EXPORT
            void operator()(const vtkm::Id &voxelIndex,
                            const vtkm::Float64 &splatValue,
                            vtkm::exec::ExecutionWholeArray<vtkm::Float64> &execArg,
                            vtkm::Float64 &output) const
            {
                execArg.Set(voxelIndex, splatValue);
            }
      };


    public:

    template <typename StorageT,
              typename StorageU>
    void run(const vtkm::cont::ArrayHandle<vtkm::Float64,StorageT> xValues,
             const vtkm::cont::ArrayHandle<vtkm::Float64,StorageT> yValues,
             const vtkm::cont::ArrayHandle<vtkm::Float64,StorageT> zValues,
             vtkm::cont::ArrayHandle<vtkm::Id3,StorageU> &output_volume_points,
             vtkm::cont::ArrayHandle<vtkm::Float64,StorageT> &output_volume_splat_values)
    {
      //Define the constants for the algorithm
      vtkm::Id sampleDim [] = {50, 50, 50};
      vtkm::cont::ArrayHandle<vtkm::Id> sampleDimensions = vtkm::cont::make_ArrayHandle(sampleDim, 3);
      vtkm::Id3 volDimensions = vtkm::make_Vec(sampleDim[0], sampleDim[1], sampleDim[2]);

      const vtkm::Float64 radius = 0.1;
      const vtkm::Float64 exponentFactor = -5.0;
      const vtkm::Float64 scaleFactor = 1.0;

//----------Configure a volume bounding box------------------------//

      //Get the scalar value bounds - min and max - for each dimension
      vtkm::Float64 b1[2];
      xValues.GetBounds(b1);
      vtkm::Vec<vtkm::Float64,2> xBounds = vtkm::make_Vec(b1[0],b1[1]);

      vtkm::Float64 b2[2];
      yValues.GetBounds(b2);
      vtkm::Vec<vtkm::Float64,2> yBounds = vtkm::make_Vec(b2[0],b2[1]);

      vtkm::Float64 b3[2];
      zValues.GetBounds(b3);
      vtkm::Vec<vtkm::Float64,2> zBounds = vtkm::make_Vec(b3[0],b3[1]);

      //Add the bounds vectors to an ArrayHandle for sorting
      vtkm::Vec<vtkm::Float64,2> bounds [] = {xBounds, yBounds, zBounds};
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64,2> > allBounds =
          vtkm::cont::make_ArrayHandle(bounds, 3);

      //Sort the bounds vectors from smallest to largest range (max-min)
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(allBounds, DimBoundsCompare());

      vtkm::Vec<vtkm::Float64,2> maxBound = allBounds.GetPortalConstControl().Get(2);
      const vtkm::Float64 maxDist = (maxBound[1] - maxBound[0]) * radius;
      const vtkm::Float64 radius2 = maxDist * maxDist;

      //If desired, invoke a WorkletMapField here to adjust the bounds in
      //each direction by maxDist, so that the model fits strictly inside...

      //Set the volume origin
      vtkm::Vec<vtkm::Float64, 3> origin = vtkm::make_Vec(xBounds[0], yBounds[0], zBounds[0]);

      //Set the volume spacing and splat distance via a WorkletMapField
      vtkm::cont::ArrayHandle<vtkm::Float64> spacing;
      vtkm::cont::ArrayHandle<vtkm::Float64> splatDist;
      vtkm::worklet::DispatcherMapField<ConfigureVolumeProps> configVolumeDispatcher;
      configVolumeDispatcher.Invoke(allBounds, sampleDimensions, spacing, splatDist);

      vtkm::cont::ArrayHandle<vtkm::Float64>::PortalConstControl spcc = spacing.GetPortalConstControl();
      vtkm::cont::ArrayHandle<vtkm::Float64>::PortalConstControl dpcc = spacing.GetPortalConstControl();
      vtkm::Vec<vtkm::Float64, 3> vecSpacing = vtkm::make_Vec(spcc.Get(0),spcc.Get(1),spcc.Get(2));
      vtkm::Vec<vtkm::Float64, 3> vecSplatDist = vtkm::make_Vec(dpcc.Get(0),dpcc.Get(1),dpcc.Get(2));

      //Number of grid points in the volume bounding box
      const vtkm::Id numVolumePoints = sampleDim[0] * sampleDim[1] * sampleDim[2];

      //Number of points (x,y,z) that will be splat onto the volume grid
      const vtkm::Id numSamplePoints = xValues.GetNumberOfValues();


//------------------------Begin splatting phase-------------------------//

      typedef vtkm::cont::ArrayHandle<vtkm::Float64> FloatHandleType;
      typedef vtkm::cont::ArrayHandle<vtkm::Id3> VecHandleType;
      typedef vtkm::cont::ArrayHandle<vtkm::Id> IdHandleType;
      typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, VecHandleType> VecPermType;
      typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, IdHandleType> IdPermType;

      //Compute each of the volume gridpoint coordinates and assign
      //each voxel an initial splat value of 0
      VecHandleType allVoxelPoints; //of length numVolumePoints
      FloatHandleType allSplatValues; //of length numVolumePoints
      vtkm::cont::ArrayHandleCounting<vtkm::Id> indexArray(vtkm::Id(0), numVolumePoints);
      vtkm::worklet::DispatcherMapField<GetVolumeCoords> coordsDispatcher(volDimensions);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        vtkm::cont::Timer<DeviceAdapter> timer;
      #endif
      coordsDispatcher.Invoke(indexArray, allVoxelPoints, allSplatValues);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "GetVolumeCoords_Worklet," << timer.GetElapsedTime() << "\n";
      #endif
      indexArray.ReleaseResources();

      //Get the splat footprint/neighborhood of each sample point, as
      //represented by min and max boundaries in each dimension.
      VecHandleType splatPoints;
      VecHandleType footprintMin;
      VecHandleType footprintMax;
      IdHandleType numNeighbors;
      vtkm::worklet::DispatcherMapField<GetFootprint> footprintDispatcher(vecSpacing,
                                                                          vecSplatDist,
                                                                          origin,
                                                                          volDimensions);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        timer.Reset();
      #endif
      configVolumeDispatcher.Invoke(xValues, yValues, zValues,
                                    splatPoints, footprintMin,
                                    footprintMax, numNeighbors);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "GetFootprint_Worklet," << timer.GetElapsedTime() << "\n";
      #endif

      //Prefix sum of the number of affected splat voxels ("neighbors")
      //for each sample point.  The total sum represents the number of
      //voxels for which splat values will be computed.
      IdHandleType numNeighborsPrefixSum;
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        timer.Reset();
      #endif
      const vtkm::Id totalSplatSize =
            vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanInclusive(numNeighbors,
                                                                       numNeighborsPrefixSum);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "ScanInclusive_Adapter," << timer.GetElapsedTime() << "\n";
      #endif
      numNeighbors.ReleaseResources();

      //Generate a lookup array that, for each splat voxel, identifies
      //the Id of its corresponding (sample) splat point.
      //For example, if splat point 0 affects 5 neighbor voxels, then
      //the five entries in the lookup array would be 0,0,0,0,0
      IdHandleType neighbor2SplatId;
      vtkm::cont::ArrayHandleCounting<vtkm::Id> countingArray(vtkm::Id(1), vtkm::Id(totalSplatSize));
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        timer.Reset();
      #endif
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::UpperBounds(numNeighborsPrefixSum,
                                                                      countingArray,
                                                                      neighbor2SplatId);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "UpperBounds_Adapter," << timer.GetElapsedTime() << "\n";
      #endif
      countingArray.ReleaseResources();

      //Extract a "local" Id lookup array of the foregoing
      //neighbor2SplatId array.  So, the local version of 0,0,0,0,0
      //would be 0,1,2,3,4
      IdHandleType localNeighborIds;
      IdPermType modulii(neighbor2SplatId, numNeighborsPrefixSum);
      vtkm::worklet::DispatcherMapField<ComputeLocalNeighborId> idDispatcher;
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        timer.Reset();
      #endif
      idDispatcher.Invoke(modulii, neighbor2SplatId, localNeighborIds);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "ComputeLocalNeighborId_Worklet," << timer.GetElapsedTime() << "\n";
      #endif
      modulii.ReleaseResources();
      numNeighborsPrefixSum.ReleaseResources();

      //Perform gather operations via permutation arrays
      VecPermType ptSplatPoints(neighbor2SplatId, splatPoints);
      VecPermType ptFootprintMins(neighbor2SplatId, footprintMin);
      VecPermType ptFootprintMaxs(neighbor2SplatId, footprintMax);

      //Calculate the splat value of each affected voxel
      FloatHandleType splatValues;
      FloatHandleType voxelSplatSums;
      IdHandleType neighborVoxelIds;
      IdHandleType uniqueVoxelIds;
      vtkm::worklet::DispatcherMapField<GetSplatValue> splatterDispatcher(vecSpacing,
                                                                          origin,
                                                                          volDimensions,
                                                                          radius2,
                                                                          exponentFactor,
                                                                          scaleFactor);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        timer.Reset();
      #endif
      splatterDispatcher.Invoke(ptSplatPoints, ptFootprintMins,
                                ptFootprintMaxs, neighbor2SplatId,
                                localNeighborIds, neighborVoxelIds, splatValues);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "GetSplatValue_Worklet," << timer.GetElapsedTime() << "\n";
      #endif
      ptSplatPoints.ReleaseResources();
      ptFootprintMins.ReleaseResources();
      ptFootprintMaxs.ReleaseResources();
      neighbor2SplatId.ReleaseResources();
      localNeighborIds.ReleaseResources();
      splatPoints.ReleaseResources();
      footprintMin.ReleaseResources();
      footprintMax.ReleaseResources();

      //Sort the voxel Ids in ascending order
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        timer.Reset();
      #endif
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::SortByKey(neighborVoxelIds,
                                                                   splatValues);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "SortByKey_Adapter," << timer.GetElapsedTime() << "\n";
      #endif

      //Produces the sum of all splat values for each unique voxel
      //that was part of the splatter
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        timer.Reset();
      #endif
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ReduceByKey(neighborVoxelIds,
                                                                     splatValues,
                                                                     uniqueVoxelIds,
                                                                     voxelSplatSums,
                                                                     vtkm::internal::Add());
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "ReduceByKey_Adapter," << timer.GetElapsedTime() << "\n";
      #endif
      neighborVoxelIds.ReleaseResources();
      splatValues.ReleaseResources();

      //Scatter operation to write the previously-computed splat
      //value sums into their corresponding entries in the master
      //splat value array.  Any voxels (volume gridpoints) that weren't
      //affected by the splatter will still have the default value of 0.
      FloatHandleType finalSplatValues;
      vtkm::worklet::DispatcherMapField<UpdateVoxelSplats> scatterDispatcher;
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        timer.Reset();
      #endif
      scatterDispatcher.Invoke(uniqueVoxelIds, voxelSplatSums,
                               vtkm::exec::ExecutionWholeArrayConst<vtkm::Float64>(allSplatValues),
                               finalSplatValues);
      #ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
        std::cout << "UpdateVoxelSplats_Worklet," << timer.GetElapsedTime() << "\n";
      #endif
      uniqueVoxelIds.ReleaseResources();
      voxelSplatSums.ReleaseResources();
      finalSplatValues.ReleaseResources();

      //Assign the volume gridpoint coordinates and their splatter values
      //as the output of this worklet
      output_volume_points = allVoxelPoints;
      output_volume_splat_values = allSplatValues;

    }

  }; //struct GaussianSplatter


}} //namespace vtkm::worklet

#endif //vtk_m_worklet_GaussianSplatter_h
