/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

/// @file ArrayView.h
/// This file contains the ArrayView class, a class that allows to wrap any
/// contiguous raw data into
/// a view which is accessible with multiple indices.
/// All it needs is the strides for each index, and the shape of each index.
/// ATTENTION: The last index is stride 1
///
/// Bounds-checking can be turned ON by defining
/// "ATLAS_ARRAYVIEW_BOUNDS_CHECKING"
/// before including this header.
///
/// Example 1:
///     int[] array = { 1, 2, 3, 4, 5, 6, 7, 8, 9};
///     int[2] strides = { 3, 1 };
///     int[2] shape = { 3, 3 };
///     ArrayView<int,2> matrix( array, shape, strides );
///     for( size_t i=0; i<matrix.shape(0); ++i ) {
///       for( size_t j=0; j<matrix.shape(1); ++j ) {
///         matrix(i,j) *= 10;
///       }
///     }
///
/// Strides can also be omitted as for most common cases it can be inferred
/// from the shape.
///
/// Example 2:
///     int[] array = { 1, 2, 3, 4, 5, 6, 7, 8, 9};
///     int[2] shape = { 3, 3 };
///     ArrayView<int,2> matrix( array, shape );
/// which is identical for this matrix to previous Example 1
///
/// There is also an easier way to wrap Field and Array classes:
///
/// Example 3:
///     ArrayView<int,3> fieldview( Field );
///     ArrayView<int,2> arrayview( Array );
///
/// @author Willem Deconinck

#pragma once

#include "atlas/library/config.h"

#if ATLAS_HAVE_GRIDTOOLS_STORAGE
#include "atlas/array/gridtools/GridToolsArrayView.h"
#else
#include "atlas/array/native/NativeArrayView.h"
#endif
