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

//=======================================================================================
// 
// Second Attempt to Compute Contour Tree in Data-Parallel Mode
//
// Started August 19, 2015
//
// Copyright Hamish Carr, University of Leeds
//
// PrintVectors.h - pretty printing (mostly for debug)
//
//=======================================================================================
//
// COMMENTS:
//
// Emphasis on robustness / simplicity rather than speed
//
//=======================================================================================

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
