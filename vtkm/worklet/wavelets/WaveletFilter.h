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

// Wavelet filter class; 
// functionally equivalent to WaveFiltBase and its subclasses in VAPoR.
class WaveletFilter
{
public:
  // constructor
  WaveletFilter( const std::string &wname )
  {
    lowDecomposeFilter = highDecomposeFilter = 
      lowReconstructFilter = highReconstructFilter = NULL;
    this->filterLength = 0;
    if( wname.compare("CDF9/7") == 0 )
    {
      this->symmetricity= true;
      this->filterLength = 9;
      this->AllocateFilterMemory();
      wrev( vtkm::worklet::wavelets::hm4_44,      lowDecomposeFilter, filterLength );
      qmf_wrev( vtkm::worklet::wavelets::h4,      highDecomposeFilter, filterLength );
      verbatim_copy( vtkm::worklet::wavelets::h4, lowReconstructFilter, filterLength );
      qmf_even( vtkm::worklet::wavelets::hm4_44,  highReconstructFilter, filterLength );
    }
    else if( wname.compare("CDF5/3") == 0 )
    {
      this->symmetricity = true;
      this->filterLength = 5;
      this->AllocateFilterMemory();
      wrev( vtkm::worklet::wavelets::hm2_22,         lowDecomposeFilter, filterLength );
      qmf_wrev( vtkm::worklet::wavelets::h2+6,       highDecomposeFilter, filterLength );
      verbatim_copy( vtkm::worklet::wavelets::h2+6,  lowReconstructFilter, filterLength );
      qmf_even( vtkm::worklet::wavelets::hm2_22,     highReconstructFilter, filterLength );
    }
    else
    {
      std::cerr << "Not supported wavelet kernel: " << wname << std::endl;
      // TODO: throw an error here
    }
  }

  // destructor
  virtual ~WaveletFilter()
  {
    if(  lowDecomposeFilter )        delete[] lowDecomposeFilter;
    if(  highDecomposeFilter )       delete[] highDecomposeFilter;
    if(  lowReconstructFilter )      delete[] lowReconstructFilter; 
    if(  highReconstructFilter )     delete[] highReconstructFilter;
  }

  vtkm::Id GetFilterLength()    { return this->filterLength; }
  bool     isSymmetric()        { return this->symmetricity;   }

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

protected:
  bool              symmetricity;
  vtkm::Id          filterLength;
  vtkm::Float64*    lowDecomposeFilter;
  vtkm::Float64*    highDecomposeFilter;
  vtkm::Float64*    lowReconstructFilter;
  vtkm::Float64*    highReconstructFilter;

  void AllocateFilterMemory()
  {
    lowDecomposeFilter    = new vtkm::Float64[ this->filterLength ];
    highDecomposeFilter   = new vtkm::Float64[ this->filterLength ];
    lowReconstructFilter  = new vtkm::Float64[ this->filterLength ];
    highReconstructFilter = new vtkm::Float64[ this->filterLength ];
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
    for (vtkm::Id count = 0; count < sigLength; count++) 
    {
      sigOut[count] = sigIn[sigLength - count - 1];

      if (sigLength % 2 == 0) {
        if (count % 2 != 0) 
          sigOut[count] = -1.0 * sigOut[count];
      }
      else {
        if (count % 2 == 0) 
          sigOut[count] = -1.0 * sigOut[count];
      }
    }
  }
  
  // Flipping and QMF at the same time: helper function to initialize a filter.
  void qmf_wrev ( const vtkm::Float64* sigIn, vtkm::Float64* sigOut, vtkm::Id sigLength )
  {
    for (vtkm::Id count = 0; count < sigLength; count++) {
      sigOut[count] = sigIn[sigLength - count - 1];

      if (sigLength % 2 == 0) {
        if (count % 2 != 0) 
          sigOut[count] = -1 * sigOut[count];
      }
      else {
        if (count % 2 == 0) 
          sigOut[count] = -1 * sigOut[count];
      }
    }

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
