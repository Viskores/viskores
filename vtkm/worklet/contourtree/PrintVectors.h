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
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC. 
//  This software was produced under U.S. Government contract DE-AC52-06NA25396 
//  for Los Alamos National Laboratory (LANL), which is operated by 
//  Los Alamos National Security, LLC for the U.S. Department of Energy. 
//  The U.S. Government has rights to use, reproduce, and distribute this 
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC 
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE 
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works, 
//  such modified software should be clearly marked, so as not to confuse it 
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or 
//  without modification, are permitted provided that the following conditions 
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos 
//     National Laboratory, LANL, the U.S. Government, nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND 
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS 
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:  
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.” 
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens. 
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization 
//  (LDAV), October 2016, Baltimore, Maryland.

#ifndef vtkm_filter_print_vector
#define vtkm_filter_print_vector

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>

using namespace std;

// debug value for number of columns to print
vtkm::Id printCols = 10;

#define PRINT_WIDTH 12
#define PREFIX_WIDTH 20

// debug value for number of columns to print
extern vtkm::Id printCols;

// utility routine to convert number to a string
string NumString(vtkm::Id number);

// base routines for printing label & prefix bars
void printLabel(string label);
void printSeparatingBar(vtkm::Id howMany);

// routines to print out a single value
template<typename T>
void printDataType(T value);
void printIndexType(vtkm::Id value);

// header line
void printHeader(vtkm::Id howMany);

// base routines for reading & writing host vectors
template<typename T, typename StorageType>
void printValues(string label, vtkm::cont::ArrayHandle<T,StorageType> &dVec, vtkm::Id nValues = -1);
void printIndices(string label, vtkm::cont::ArrayHandle<vtkm::Id> &iVec, vtkm::Id nIndices = -1);

// routines for printing indices & data in blocks
template<typename T, typename StorageType>
void printLabelledBlock(string label, const vtkm::cont::ArrayHandle<T, StorageType> &dVec, vtkm::Id nRows, vtkm::Id nColumns);

// utility routine to convert number to a string
string NumString(vtkm::Id number)
	{ // NumString()
	char strBuf[20];
	sprintf(strBuf, "%1d", (int) number);
	return string(strBuf);
	} // NumString()

// base routines for printing label & prefix bars
void printLabel(string label)
	{ // printLabel()
	// print out the front end
	cout << setw(PREFIX_WIDTH) << left << label;
	// print out the vertical line
	cout << right << "|";
	} // printLabel()
	
void printSeparatingBar(vtkm::Id howMany)
	{ // printSeparatingBar()
	// print out the front end
	cout << setw(PREFIX_WIDTH) << setfill('-') << "";
	// now the + at the vertical line
	cout << "+";
	// now print out the tail end - fixed number of spaces per entry
	for (vtkm::Id block = 0; block < howMany; block++)
		cout << setw(PRINT_WIDTH) << setfill('-') << "";
	// now the endl, resetting the fill character
	cout << setfill(' ') << endl;
	} // printSeparatingBar()

// routine to print out a single value
template<typename T>
void printDataType(T value)
	{ // printDataType
	cout << setw(PRINT_WIDTH) << value;
	} // printDataType

// routine to print out a single value
void printIndexType(vtkm::Id value)
	{ // printIndexType
	cout << setw(PRINT_WIDTH) << value;
	} // printIndexType

// header line 
void printHeader(vtkm::Id howMany)
	{ // printHeader()
	if (howMany > 16) howMany = 16;
	// print out a separating bar
	printSeparatingBar(howMany);
	// print out a label
	printLabel("ID");
	// print out the ID numbers
	for (vtkm::Id entry = 0; entry < howMany; entry++)
		printIndexType(entry);
	// and an endl
	cout << endl;
	// print out another separating bar
	printSeparatingBar(howMany);
	} // printHeader()

// base routines for reading & writing host vectors
template<typename T, typename StorageType>
void printValues(string label, vtkm::cont::ArrayHandle<T,StorageType> &dVec, vtkm::Id nValues)
{
	// -1 means full size
	if (nValues == -1)
		nValues = dVec.GetNumberOfValues();
	if (nValues > 16) nValues = 16;
	
	// print the label
	printLabel(label);

	// now print the data
	for (vtkm::Id entry = 0; entry < nValues; entry++)
		printDataType(dVec.GetPortalControl().Get(entry));

	// and an endl
	std::cout << std::endl;
} // printValues()

// base routines for reading & writing host vectors
void printIndices(string label, vtkm::cont::ArrayHandle<vtkm::Id> &iVec, vtkm::Id nIndices)
{
	// -1 means full size
	if (nIndices == -1)
		nIndices = iVec.GetNumberOfValues();

	if (nIndices > 16) nIndices = 16;
	
	// print the label
	printLabel(label);

	// now print the data
	for (vtkm::Id entry = 0; entry < nIndices; entry++)
		printIndexType(iVec.GetPortalControl().Get(entry));

	// and an endl
	std::cout << std::endl;
} // printIndices()

template<typename T, typename StorageType>
void printLabelledBlock(string label, const vtkm::cont::ArrayHandle<T, StorageType> &dVec, vtkm::Id nRows, vtkm::Id nColumns)
{
	// start with a header
	printHeader(nColumns);	
	// loop control variable
	vtkm::Id entry = 0;
	// per row
	for (vtkm::Id row = 0; row < nRows; row++)
		{ // per row
		printLabel(label + "[" + NumString(row) + "]");
		// now print the data
		for (vtkm::Id col = 0; col < nColumns; col++, entry++) {
			printDataType(dVec.GetPortalConstControl().Get(entry));
                }
		cout << endl;
		} // per row
	cout << endl;
} // printLabelledBlock()

#endif
