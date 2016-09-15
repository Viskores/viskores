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

#ifndef vtk_m_worklet_wavelets_waveletfilter_h
#define vtk_m_worklet_wavelets_waveletfilter_h

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/worklet/wavelets/FilterBanks.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {

namespace wavelets {

enum WaveletName {
  CDF9_7,
  CDF5_3,
  CDF8_4,
  HAAR,
  BIOR4_4,  // the same as CDF9_7
  BIOR3_3,  // the same as CDF8_4
  BIOR2_2,  // the same as CDF5_3
  BIOR1_1   // the same as HAAE
};

// Wavelet filter class;
// functionally equivalent to WaveFiltBase and its subclasses in VAPoR.
class WaveletFilter
{
public:
  // constructor
  WaveletFilter( WaveletName wtype ) : symmetricity(true),
                                       filterLength(0),
                                       lowDecomposeFilter(nullptr),
                                       highDecomposeFilter(nullptr),
                                       lowReconstructFilter(nullptr),
                                       highReconstructFilter(nullptr)
  {
    if( wtype == CDF9_7 || wtype == BIOR4_4 )
    {
      this->filterLength = 9;
      this->AllocateFilterMemory();
      this->wrev( vtkm::worklet::wavelets::hm4_44,      lowDecomposeFilter,     filterLength );
      this->qmf_wrev( vtkm::worklet::wavelets::h4,      highDecomposeFilter,    filterLength );
      this->verbatim_copy( vtkm::worklet::wavelets::h4, lowReconstructFilter,   filterLength );
      this->qmf_even( vtkm::worklet::wavelets::hm4_44,  highReconstructFilter,  filterLength );
    }
    else if( wtype == CDF8_4 || wtype == BIOR3_3 )
    {
      this->filterLength = 8;
      this->AllocateFilterMemory();
      this->wrev( vtkm::worklet::wavelets::hm3_33,         lowDecomposeFilter,    filterLength );
      this->qmf_wrev( vtkm::worklet::wavelets::h3+6,       highDecomposeFilter,   filterLength );
      this->verbatim_copy( vtkm::worklet::wavelets::h3+6,  lowReconstructFilter,  filterLength );
      this->qmf_even( vtkm::worklet::wavelets::hm3_33,     highReconstructFilter, filterLength );
    }
    else if( wtype == CDF5_3 || wtype == BIOR2_2 )
    {
      this->filterLength = 5;
      this->AllocateFilterMemory();
      this->wrev( vtkm::worklet::wavelets::hm2_22,         lowDecomposeFilter,    filterLength );
      this->qmf_wrev( vtkm::worklet::wavelets::h2+6,       highDecomposeFilter,   filterLength );
      this->verbatim_copy( vtkm::worklet::wavelets::h2+6,  lowReconstructFilter,  filterLength );
      this->qmf_even( vtkm::worklet::wavelets::hm2_22,     highReconstructFilter, filterLength );
    }
    else if( wtype == HAAR || wtype == BIOR1_1 )
    {
      this->filterLength = 2;
      this->AllocateFilterMemory();
      this->wrev( vtkm::worklet::wavelets::hm1_11,         lowDecomposeFilter,    filterLength );
      this->qmf_wrev( vtkm::worklet::wavelets::h1+4,       highDecomposeFilter,   filterLength );
      this->verbatim_copy( vtkm::worklet::wavelets::h1+4,  lowReconstructFilter,  filterLength );
      this->qmf_even( vtkm::worklet::wavelets::hm1_11,     highReconstructFilter, filterLength );
    }
  }

  // destructor
  virtual ~WaveletFilter()
  {
    if(  lowDecomposeFilter )
    {
      delete[] lowDecomposeFilter;
      lowDecomposeFilter    = highDecomposeFilter =
      lowReconstructFilter  = highReconstructFilter =  nullptr ;
    }
  }

  vtkm::Id GetFilterLength()    { return filterLength; }
  bool     isSymmetric()        { return symmetricity; }

  typedef vtkm::cont::ArrayHandle<vtkm::Float64> FilterType;
  FilterType GetLowDecomposeFilter() const
  {
    return vtkm::cont::make_ArrayHandle( lowDecomposeFilter, filterLength );
  }
  FilterType GetHighDecomposeFilter() const
  {
    return vtkm::cont::make_ArrayHandle( highDecomposeFilter, filterLength );
  }
  FilterType GetLowReconstructFilter() const
  {
    return vtkm::cont::make_ArrayHandle( lowReconstructFilter, filterLength);
  }
  FilterType GetHighReconstructFilter() const
  {
    return vtkm::cont::make_ArrayHandle( highReconstructFilter, filterLength );
  }

private:
  bool              symmetricity;
  vtkm::Id          filterLength;
  vtkm::Float64*    lowDecomposeFilter;
  vtkm::Float64*    highDecomposeFilter;
  vtkm::Float64*    lowReconstructFilter;
  vtkm::Float64*    highReconstructFilter;

  void AllocateFilterMemory()
  {
    lowDecomposeFilter    = new vtkm::Float64[ filterLength * 4 ];
    highDecomposeFilter   = lowDecomposeFilter    + filterLength;
    lowReconstructFilter  = highDecomposeFilter   + filterLength;
    highReconstructFilter = lowReconstructFilter  + filterLength;
  }

  // Flipping operation; helper function to initialize a filter.
  void wrev( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
  {
    for( vtkm::Id count = 0; count < sigLength; count++)
      sigOut[count] = sigIn[sigLength - count - 1];
  }

  // Quadrature mirror filtering operation: helper function to initialize a filter.
  void qmf_even ( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
  {
    if( sigLength % 2 == 0 )
    {
      for (vtkm::Id count = 0; count < sigLength; count++)
      {
        sigOut[count] = sigIn[sigLength - count - 1];
        if (count % 2 != 0)
          sigOut[count] = -1.0 * sigOut[count];
      }
    }
    else
    {
      for (vtkm::Id count = 0; count < sigLength; count++)
      {
        sigOut[count] = sigIn[sigLength - count - 1];
        if (count % 2 == 0)
          sigOut[count] = -1.0 * sigOut[count];
      }
    }
  }

  // Flipping and QMF at the same time: helper function to initialize a filter.
  void qmf_wrev ( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
  {
    qmf_even( sigIn, sigOut, sigLength );

    vtkm::Float64 tmp;
    for (vtkm::Id count = 0; count < sigLength/2; count++) {
      tmp = sigOut[count];
      sigOut[count] = sigOut[sigLength - count - 1];
      sigOut[sigLength - count - 1] = tmp;
    }
  }

  // Verbatim Copying: helper function to initialize a filter.
  void verbatim_copy ( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
  {
    for (vtkm::Id count = 0; count < sigLength; count++)
      sigOut[count] = sigIn[count];
  }

};    // class WaveletFilter.
}     // namespace wavelets.


}     // namespace worklet
}     // namespace vtkm

#endif
