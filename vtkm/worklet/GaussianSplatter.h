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
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/cont/internal/ArrayPortalFromIterators.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#define __VTKM_GAUSSIAN_SPLATTER_BENCHMARK

//----------------------------------------------------------------------------
// Macros for timing
//----------------------------------------------------------------------------
#ifdef __VTKM_GAUSSIAN_SPLATTER_BENCHMARK
// start timer
# define START_TIMER_BLOCK(name) \
    vtkm::cont::Timer<DeviceAdapter> timer_##name;

// stop timer
# define END_TIMER_BLOCK(name) \
    std::cout << #name " : elapsed : " << timer_##name.GetElapsedTime() << "\n";
#else
# define START_TIMER_BLOCK(name)
# define END_TIMER_BLOCK(name)
#endif

//----------------------------------------------------------------------------
// Gaussian splatter worklet/filter
//----------------------------------------------------------------------------
namespace vtkm { namespace worklet
{

namespace debug {
#ifdef DEBUG_PRINT
//----------------------------------------------------------------------------
template<typename T, typename S = VTKM_DEFAULT_STORAGE_TAG>
void OutputArrayDebug(const vtkm::cont::ArrayHandle<T,S> &outputArray, const std::string &name)
{
    typedef T ValueType;
    typedef vtkm::cont::internal::Storage<T,S> StorageType;
    typedef typename StorageType::PortalConstType PortalConstType;
    PortalConstType readPortal = outputArray.GetPortalConstControl();
    vtkm::cont::ArrayPortalToIterators<PortalConstType> iterators(readPortal);
    std::vector<ValueType> result(readPortal.GetNumberOfValues());
    std::copy(iterators.GetBegin(), iterators.GetEnd(), result.begin());
    std::cout << name.c_str() << " " << outputArray.GetNumberOfValues() << "\n";
    std::copy(result.begin(), result.end(), std::ostream_iterator<ValueType>(std::cout, " "));
    std::cout << std::endl;
}

//----------------------------------------------------------------------------
template<typename T, int S>
void OutputArrayDebug(const vtkm::cont::ArrayHandle< vtkm::Vec<T,S> > &outputArray, const std::string &name)
{
    typedef T ValueType;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<T, S> >::PortalConstControl PortalConstType;
    PortalConstType readPortal = outputArray.GetPortalConstControl();
    vtkm::cont::ArrayPortalToIterators<PortalConstType> iterators(readPortal);
    std::cout << name.c_str() << " " << outputArray.GetNumberOfValues() << "\n";
    for (int i=0; i<outputArray.GetNumberOfValues(); ++i) {
        std::cout << outputArray.GetPortalConstControl().Get(i);
    }
    std::cout << std::endl;
}
//----------------------------------------------------------------------------
template<typename I, typename T, int S>
void OutputArrayDebug(const vtkm::cont::ArrayHandlePermutation<I, vtkm::cont::ArrayHandle<vtkm::Vec<T, S> > > &outputArray, const std::string &name)
{
    typedef typename vtkm::cont::ArrayHandlePermutation<I, vtkm::cont::ArrayHandle<vtkm::Vec<T, S> > >::PortalConstControl PortalConstType;
    PortalConstType readPortal = outputArray.GetPortalConstControl();
    vtkm::cont::ArrayPortalToIterators<PortalConstType> iterators(readPortal);
    std::cout << name.c_str() << " " << outputArray.GetNumberOfValues() << "\n";
    for (int i = 0; i<outputArray.GetNumberOfValues(); ++i) {
        std::cout << outputArray.GetPortalConstControl().Get(i);
    }
    std::cout << std::endl;
}

#else
template<typename T, typename S = VTKM_DEFAULT_STORAGE_TAG>
void OutputArrayDebug(const vtkm::cont::ArrayHandle<T,S> &outputArray, const std::string &name)
{}
//----------------------------------------------------------------------------
template<typename T, int S>
void OutputArrayDebug(const vtkm::cont::ArrayHandle< vtkm::Vec<T,S> > &outputArray, const std::string &name)
{}
//----------------------------------------------------------------------------
template<typename I, typename T, int S>
void OutputArrayDebug(const vtkm::cont::ArrayHandlePermutation<I, vtkm::cont::ArrayHandle<vtkm::Vec<T, S> > > &outputArray, const std::string &name)
{}
#endif
} // namespace debug

template<typename DeviceAdapter>
struct GaussianSplatterFilterUniformGrid
{
    typedef vtkm::cont::ArrayHandle<vtkm::Float64> DoubleHandleType;
    typedef vtkm::cont::ArrayHandle<vtkm::Float32> FloatHandleType;
    typedef vtkm::cont::ArrayHandle<vtkm::Id3>     VecHandleType;
    typedef vtkm::cont::ArrayHandle<vtkm::Id>      IdHandleType;
    //
    typedef vtkm::Vec<vtkm::Float32, 3>           FloatVec;
    typedef vtkm::Vec<vtkm::Float64, 3>           PointType;
    typedef vtkm::cont::ArrayHandle<PointType>    PointHandleType;
    //
    typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, VecHandleType>   VecPermType;
    typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, PointHandleType> PointVecPermType;
    typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, IdHandleType>    IdPermType;
    typedef vtkm::cont::ArrayHandlePermutation<IdHandleType, FloatHandleType> FloatPermType;
    //
    typedef vtkm::cont::ArrayHandleCounting<vtkm::Id> IdCountingType;

    //-----------------------------------------------------------------------
    // zero an array,
    // @TODO, get rid of this
    //-----------------------------------------------------------------------
    struct zero_voxel : public vtkm::worklet::WorkletMapField {
        typedef void ControlSignature(FieldIn<>, FieldOut<>);
        typedef void ExecutionSignature(_1, WorkIndex, _2);
        //
        VTKM_CONT_EXPORT 
        zero_voxel() {}

        template<typename T>
        VTKM_EXEC_CONT_EXPORT
        void operator()(const vtkm::Id &, const vtkm::Id &index, T &voxel_value) const
        {
            voxel_value = T(0);
        }
    };

    //-----------------------------------------------------------------------
    // Return the splat footprint/neighborhood of each sample point, as
    // represented by min and max boundaries in each dimension.
    // Also return the size of this footprint and the voxel coordinates
    // of the splat point (floating point).
    //-----------------------------------------------------------------------
    class GetFootprint : public vtkm::worklet::WorkletMapField
    {
    private:
        vtkm::Vec<vtkm::Float64, 3> Origin;
        vtkm::Vec<vtkm::Float64, 3> Spacing;
        vtkm::Id3 VolumeDimensions;

    public:
        typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>,
                FieldOut<>, FieldOut<>, FieldOut<>, FieldOut<>);
        typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);

        VTKM_CONT_EXPORT
        GetFootprint(
                const vtkm::Vec<vtkm::Float64, 3> &o,
                const vtkm::Vec<vtkm::Float64, 3> &s,
                const vtkm::Id3 &dim)
        : Origin(o), Spacing(s),
          VolumeDimensions(dim) { }

        template<typename T, typename T2>
        VTKM_EXEC_CONT_EXPORT
        void operator()(
                const T &xValue,
                const T &yValue,
                const T &zValue,
                const T2 &rValue,
                vtkm::Vec<vtkm::Float64, 3> &splatPoint,
                vtkm::Id3 &minFootprint,
                vtkm::Id3 &maxFootprint,
                vtkm::Id &footprintSize) const
        {
            PointType splat, min, max;
            vtkm::Vec<vtkm::Float64, 3> sample = vtkm::make_Vec(xValue, yValue, zValue);
            vtkm::Id size = 1;
            for (int i = 0; i < 3; i++)
            {
                splat[i] = (sample[i] - this->Origin[i]) / this->Spacing[i];
                min[i] = static_cast<vtkm::Id>(ceil(static_cast<double>(splat[i]) - rValue));
                max[i] = static_cast<vtkm::Id>(floor(static_cast<double>(splat[i]) + rValue));
                if (min[i] < 0) {
                    min[i] = 0;
                }
                if (max[i] >= this->VolumeDimensions[i]) {
                    max[i] = this->VolumeDimensions[i] - 1;
                }
                size = size * (1 + max[i] - min[i]);
            }
            splatPoint = splat;
            minFootprint = min;
            maxFootprint = max;
            footprintSize = size;
        }
    };

    //-----------------------------------------------------------------------
    // Return the "local" Id of a voxel within a splat point's footprint.
    // A splat point that affects 5 neighboring voxel gridpoints would
    // have local Ids 0,1,2,3,4
    //-----------------------------------------------------------------------
    class ComputeLocalNeighborId : public vtkm::worklet::WorkletMapField
    {
    public:
        typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>);
        typedef void ExecutionSignature(_1, _2, WorkIndex, _3);

        VTKM_CONT_EXPORT
        ComputeLocalNeighborId() {}

        template<typename T>
        VTKM_EXEC_CONT_EXPORT
        void operator()(const T &modulus, const T &offset,
                const vtkm::Id &index, T &localId) const
        {
            localId = (index - offset) % modulus;
        }
    };

    //-----------------------------------------------------------------------
    // Compute the Gaussian splat value of the input neighbour point.
    // The voxel Id of this point within the volume is also determined.
    // @TODO : fix kernel to behave with correct radious, scale exponent etc
    //-----------------------------------------------------------------------
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
        typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>,
                FieldIn<>, FieldIn<>, FieldOut<>,
                FieldOut<>);
        typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);

        VTKM_CONT_EXPORT
        GetSplatValue(
                const vtkm::Vec<vtkm::Float64, 3> &orig,
                const vtkm::Vec<vtkm::Float64, 3> &s,
                const vtkm::Id3 &dim,
                const vtkm::Float64 &ef,
                const vtkm::Float64 &sf)
        : Spacing(s), Origin(orig), VolumeDim(dim),
          ExponentFactor(ef), ScalingFactor(sf) {}

        template<typename T, typename T2, typename P>
        VTKM_EXEC_CONT_EXPORT
        void operator()(
                const vtkm::Vec<P, 3> &splatPoint,
                const T &minBound,
                const T &maxBound,
                const T2 &radius,
                const vtkm::Id splatPointId,
                const vtkm::Id localNeighborId,
                vtkm::Id &neighborVoxelId,
                vtkm::Float32 &splatValue) const
        {
            vtkm::Id yRange = 1 + maxBound[1] - minBound[1];
            vtkm::Id xRange = 1 + maxBound[0] - minBound[0];
            vtkm::Id divisor = yRange * xRange;
            vtkm::Id i = localNeighborId / divisor;
            vtkm::Id remainder = localNeighborId % divisor;
            vtkm::Id j = remainder / xRange;
            vtkm::Id k = remainder % xRange;
            vtkm::Float64 radius2 = radius*radius;
            vtkm::Id3     voxel = minBound + vtkm::make_Vec(k, j, i);
            PointType dist = vtkm::make_Vec(
                    (splatPoint[0] - voxel[0])*Spacing[0],
                    (splatPoint[1] - voxel[1])*Spacing[0],
                    (splatPoint[2] - voxel[2])*Spacing[0]
            );
            vtkm::Float64 dist2 = vtkm::dot(dist,dist);

            // Compute Gaussian splat value
            if (dist2 <= radius2) {
                splatValue = this->ScalingFactor *
                        vtkm::Exp((this->ExponentFactor*(dist2) / radius2));
            }
            else {
                splatValue = 0.0;
            }
            neighborVoxelId = (voxel[2] * VolumeDim[0] * VolumeDim[1]) +
                              (voxel[1] * VolumeDim[0]) + voxel[0];
            if (neighborVoxelId<0) neighborVoxelId = -1;
            else if (neighborVoxelId >= VolumeDim[0] * VolumeDim[1] * VolumeDim[2])
                neighborVoxelId = VolumeDim[0] * VolumeDim[1] * VolumeDim[2] - 1;
        }
    };

    //-----------------------------------------------------------------------
    // Scatter worklet that writes a splat value into the larger,
    // master splat value array, using the splat value's voxel Id as an index.
    //-----------------------------------------------------------------------
    class UpdateVoxelSplats : public vtkm::worklet::WorkletMapField
    {
    public:
        typedef void ControlSignature(FieldIn<>, FieldIn<>, ExecObject);
        typedef void ExecutionSignature(_1, _2, _3);

        VTKM_CONT_EXPORT
        UpdateVoxelSplats() {}


        VTKM_EXEC_CONT_EXPORT
        void operator()(const vtkm::Id &voxelIndex,
                const vtkm::Float64 &splatValue,
                vtkm::exec::ExecutionWholeArray<vtkm::Float32> &execArg) const
        {
            execArg.Set(voxelIndex, splatValue);
        }
    };

    //-----------------------------------------------------------------------
    // Construct a splatter filter/object
    //
    // @TODO, get the origin and spacing from the dataset coordinates
    // instead of requiring them to be passed as parameters.
    //-----------------------------------------------------------------------
    GaussianSplatterFilterUniformGrid(const vtkm::Id3 &dims,
            vtkm::Vec<vtkm::FloatDefault, 3> origin,
            vtkm::Vec<vtkm::FloatDefault, 3> spacing,
            const vtkm::cont::DataSet &dataSet) :
                CDims(dims),
                Origin(origin),
                Spacing(spacing),
                DataSet(dataSet)
    {
    }

    //-----------------------------------------------------------------------
    // class variables for the splat filter
    //-----------------------------------------------------------------------
    vtkm::Id3           CDims;
    FloatVec            Origin, Spacing;
    vtkm::cont::DataSet DataSet;

    //-----------------------------------------------------------------------
    // Run the filter, given the input params
    //-----------------------------------------------------------------------
    template <typename StorageT, typename StorageU>
    void run(
            const vtkm::cont::ArrayHandle<vtkm::Float64, StorageT> xValues,
            const vtkm::cont::ArrayHandle<vtkm::Float64, StorageT> yValues,
            const vtkm::cont::ArrayHandle<vtkm::Float64, StorageT> zValues,
            const vtkm::cont::ArrayHandle<vtkm::Float32, StorageT> rValues,
            FloatHandleType scalarSplatOutput)
    {
        // Define the constants for the algorithm
        const vtkm::Float64 exponentFactor = -5.0;
        const vtkm::Float64 scaleFactor = 1.0;

        // Number of grid points in the volume bounding box
        vtkm::Id3 pointDimensions = vtkm::make_Vec(CDims[0]+1, CDims[1]+1, CDims[2]+1);
        const vtkm::Id numVolumePoints = (CDims[0] + 1) * (CDims[1] + 1) * (CDims[2] + 1);

        //---------------------------------------------------------------
        // Get the splat footprint/neighborhood of each sample point, as
        // represented by min and max boundaries in each dimension.
        //---------------------------------------------------------------
        PointHandleType splatPoints;
        VecHandleType   footprintMin;
        VecHandleType   footprintMax;
        IdHandleType    numNeighbors;
        IdHandleType localNeighborIds;

        GetFootprint footprint_worklet(Origin, Spacing, pointDimensions);
        vtkm::worklet::DispatcherMapField<GetFootprint> footprintDispatcher(footprint_worklet);

        START_TIMER_BLOCK(GetFootprint)
        footprintDispatcher.Invoke(
                xValues, yValues, zValues, rValues,
                splatPoints,
                footprintMin,
                footprintMax,
                numNeighbors);
        END_TIMER_BLOCK(GetFootprint)

        debug::OutputArrayDebug(numNeighbors, "numNeighbours");
        debug::OutputArrayDebug(footprintMin, "footprintMin");
        debug::OutputArrayDebug(footprintMax, "footprintMax");
        debug::OutputArrayDebug(splatPoints,  "splatPoints");

        //---------------------------------------------------------------
        // Prefix sum of the number of affected splat voxels ("neighbors")
        // for each sample point.  The total sum represents the number of
        // voxels for which splat values will be computed.
        // prefix sum is used in neighbour id lookup
        //---------------------------------------------------------------
        IdHandleType numNeighborsPrefixSum;

        START_TIMER_BLOCK(numNeighborsPrefixSum)
        const vtkm::Id totalSplatSize =
                vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanInclusive(
                        numNeighbors,
                        numNeighborsPrefixSum);
        END_TIMER_BLOCK(numNeighborsPrefixSum)

        std::cout << "totalSplatSize " << totalSplatSize << "\n";
        debug::OutputArrayDebug(numNeighborsPrefixSum, "numNeighborsPrefixSum");

        // also get the neighbour counts exclusive sum for use in lookup of local neighbour id
        IdHandleType numNeighborsExclusiveSum;
        START_TIMER_BLOCK(numNeighborsExclusiveSum)
        vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanExclusive(
                numNeighbors,
                numNeighborsExclusiveSum);
        END_TIMER_BLOCK(numNeighborsExclusiveSum)
        debug::OutputArrayDebug(numNeighborsExclusiveSum, "numNeighborsExclusiveSum");

        //---------------------------------------------------------------
        // Generate a lookup array that, for each splat voxel, identifies
        // the Id of its corresponding (sample) splat point.
        // For example, if splat point 0 affects 5 neighbor voxels, then
        // the five entries in the lookup array would be 0,0,0,0,0
        //---------------------------------------------------------------
        IdHandleType neighbor2SplatId;
        IdCountingType countingArray(vtkm::Id(0), vtkm::Id(totalSplatSize));
        START_TIMER_BLOCK(Upper_bounds)
        vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::UpperBounds(
                numNeighborsPrefixSum,
                countingArray,
                neighbor2SplatId);
        END_TIMER_BLOCK(Upper_bounds)
        countingArray.ReleaseResources();
        debug::OutputArrayDebug(neighbor2SplatId, "neighbor2SplatId");

        //---------------------------------------------------------------
        // Extract a "local" Id lookup array of the foregoing
        // neighbor2SplatId array.  So, the local version of 0,0,0,0,0
        // would be 0,1,2,3,4
        //---------------------------------------------------------------
        IdPermType modulii(neighbor2SplatId, numNeighbors);
        debug::OutputArrayDebug(modulii, "modulii");

        IdPermType offsets(neighbor2SplatId, numNeighborsExclusiveSum);
        debug::OutputArrayDebug(offsets, "offsets");

        vtkm::worklet::DispatcherMapField<ComputeLocalNeighborId> idDispatcher;
        START_TIMER_BLOCK(idDispatcher)
        idDispatcher.Invoke(
                modulii,
                offsets,
                localNeighborIds);
        END_TIMER_BLOCK(idDispatcher)
        debug::OutputArrayDebug(localNeighborIds, "localNeighborIds");

        numNeighbors.ReleaseResources();
        numNeighborsPrefixSum.ReleaseResources();
        numNeighborsExclusiveSum.ReleaseResources();

        //---------------------------------------------------------------
        // We will perform gather operations for the generated splat points
        // using permutation arrays
        //---------------------------------------------------------------
        PointVecPermType ptSplatPoints(neighbor2SplatId, splatPoints);
        VecPermType ptFootprintMins(neighbor2SplatId, footprintMin);
        VecPermType ptFootprintMaxs(neighbor2SplatId, footprintMax);
        FloatPermType radii(neighbor2SplatId, rValues);

        debug::OutputArrayDebug(radii, "radii");
        debug::OutputArrayDebug(ptSplatPoints,   "ptSplatPoints");
        debug::OutputArrayDebug(ptFootprintMins, "ptFootprintMins");

        //---------------------------------------------------------------
        // Calculate the splat value of each affected voxel
        //---------------------------------------------------------------
        FloatHandleType voxelSplatSums;
        IdHandleType neighborVoxelIds;
        IdHandleType uniqueVoxelIds;
        FloatHandleType splatValues;

        GetSplatValue splatterDispatcher_worklet(
                Origin,
                Spacing,
                pointDimensions,
                exponentFactor,
                scaleFactor);
        vtkm::worklet::DispatcherMapField<GetSplatValue> splatterDispatcher(splatterDispatcher_worklet);

        START_TIMER_BLOCK(GetSplatValue)
        splatterDispatcher.Invoke(
                ptSplatPoints,
                ptFootprintMins,
                ptFootprintMaxs,
                radii,
                neighbor2SplatId,
                localNeighborIds, neighborVoxelIds, splatValues);
        END_TIMER_BLOCK(GetSplatValue)

        debug::OutputArrayDebug(splatValues, "splatValues");
        debug::OutputArrayDebug(neighborVoxelIds, "neighborVoxelIds");

        ptSplatPoints.ReleaseResources();
        ptFootprintMins.ReleaseResources();
        ptFootprintMaxs.ReleaseResources();
        neighbor2SplatId.ReleaseResources();
        localNeighborIds.ReleaseResources();
        splatPoints.ReleaseResources();
        footprintMin.ReleaseResources();
        footprintMax.ReleaseResources();
        radii.ReleaseResources();

        //---------------------------------------------------------------
        // Sort the voxel Ids in ascending order
        //---------------------------------------------------------------
        START_TIMER_BLOCK(SortByKey)
        vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::SortByKey(
                neighborVoxelIds,
                splatValues);
        END_TIMER_BLOCK(SortByKey)
        debug::OutputArrayDebug(splatValues, "splatValues");

        //---------------------------------------------------------------
        // Do a reduction to sum all contributions for each affected voxel
        //---------------------------------------------------------------
        START_TIMER_BLOCK(ReduceByKey)
        vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ReduceByKey(
                neighborVoxelIds,
                splatValues,
                uniqueVoxelIds,
                voxelSplatSums,
                vtkm::internal::Add());
        END_TIMER_BLOCK(ReduceByKey)

        debug::OutputArrayDebug(neighborVoxelIds, "neighborVoxelIds");
        debug::OutputArrayDebug(uniqueVoxelIds, "uniqueVoxelIds");
        debug::OutputArrayDebug(voxelSplatSums, "voxelSplatSums");
        //
        neighborVoxelIds.ReleaseResources();
        splatValues.ReleaseResources();

        //---------------------------------------------------------------
        // initialize each field value to zero to begin with
        //---------------------------------------------------------------
        IdCountingType indexArray(vtkm::Id(0), numVolumePoints);
        vtkm::worklet::DispatcherMapField<zero_voxel> zeroDispatcher;
        zeroDispatcher.Invoke(indexArray, scalarSplatOutput);
        //
        indexArray.ReleaseResources();

        //---------------------------------------------------------------
        // Scatter operation to write the previously-computed splat
        // value sums into their corresponding entries in the output array
        //---------------------------------------------------------------
        vtkm::worklet::DispatcherMapField<UpdateVoxelSplats> scatterDispatcher;

        START_TIMER_BLOCK(UpdateVoxelSplats)
        scatterDispatcher.Invoke(
                uniqueVoxelIds,
                voxelSplatSums,
                vtkm::exec::ExecutionWholeArray<vtkm::Float32>(scalarSplatOutput));
        END_TIMER_BLOCK(UpdateVoxelSplats)
        debug::OutputArrayDebug(scalarSplatOutput, "scalarSplatOutput");
        //
        uniqueVoxelIds.ReleaseResources();
        voxelSplatSums.ReleaseResources();
    }

}; //struct GaussianSplatter


}
} //namespace vtkm::worklet

#endif //vtk_m_worklet_GaussianSplatter_h
