/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <type_traits>
#include <vector>

#include "atlas/array.h"
#include "atlas/array/ArrayUtil.h"
#include "atlas/array/ArrayView.h"
#include "atlas/array/DataType.h"
#include "atlas/array/MakeView.h"
#include "atlas/array/gridtools/GridToolsArrayHelpers.h"
#include "atlas/array/gridtools/GridToolsDataStore.h"
#include "atlas/array/gridtools/GridToolsTraits.h"
#include "atlas/array/helpers/ArrayInitializer.h"
#include "atlas/array_fwd.h"
#if ATLAS_HAVE_ACC
#include "atlas_acc_support/atlas_acc_map_data.h"
#endif
#include "eckit/exception/Exceptions.h"

//------------------------------------------------------------------------------

using namespace atlas::array::gridtools;
using namespace atlas::array::helpers;

namespace atlas {
namespace array {

//------------------------------------------------------------------------------

namespace gridtools {

//------------------------------------------------------------------------------

template <int Rank>
using UintSequence = ::gridtools::make_gt_integer_sequence<unsigned int, Rank>;

template <typename Value, template <class> class Storage, typename StorageInfo>
static Array* wrap_array(::gridtools::data_store<Storage<Value>, StorageInfo>* ds, const ArraySpec& spec ) {
    assert( ds );
    using data_store_t         = typename std::remove_pointer<decltype( ds )>::type;
    ArrayDataStore* data_store = new GridToolsDataStore<data_store_t>( ds );
    return new ArrayT<Value>( data_store, spec );
}

//------------------------------------------------------------------------------

}  // namespace gridtools

template <typename Value>
class ArrayT_impl {
public:
    ArrayT_impl( ArrayT<Value>& array ) : array_( array ) {}

    template <typename... UInts, typename = ::gridtools::is_all_integral<UInts...>>
    void construct( UInts... dims ) {
        auto gt_storage    = create_gt_storage<Value, typename default_layout_t<sizeof...( dims )>::type>( dims... );
        using data_store_t = typename std::remove_pointer<decltype( gt_storage )>::type;
        array_.data_store_ = std::unique_ptr<ArrayDataStore>( new GridToolsDataStore<data_store_t>( gt_storage ) );
        array_.spec_       = make_spec( gt_storage, dims... );
    }

    void construct( const ArrayShape& shape ) {
        assert( shape.size() > 0 );
        switch ( shape.size() ) {
            case 1:
                return construct( shape[0] );
            case 2:
                return construct( shape[0], shape[1] );
            case 3:
                return construct( shape[0], shape[1], shape[2] );
            case 4:
                return construct( shape[0], shape[1], shape[2], shape[3] );
            case 5:
                return construct( shape[0], shape[1], shape[2], shape[3], shape[4] );
            default: {
                std::stringstream err;
                err << "shape not recognized";
                throw eckit::BadParameter( err.str(), Here() );
            }
        }
    }

    void construct( const ArrayShape& shape, const ArrayLayout& layout ) {
        ASSERT( shape.size() > 0 );
        ASSERT( shape.size() == layout.size() );
        switch ( shape.size() ) {
            case 1:
                return construct( shape[0] );
            case 2: {
                if ( layout[0] == 0 && layout[1] == 1 )
                    return construct_with_layout<::gridtools::layout_map<0, 1>>( shape[0], shape[1] );
                if ( layout[0] == 1 && layout[1] == 0 )
                    return construct_with_layout<::gridtools::layout_map<1, 0>>( shape[0], shape[1] );
            }
            case 3: {
                if ( layout[0] == 0 && layout[1] == 1 && layout[2] == 2 )
                    return construct_with_layout<::gridtools::layout_map<0, 1, 2>>( shape[0], shape[1], shape[2] );
                if ( layout[0] == 0 && layout[1] == 2 && layout[2] == 1 )
                    return construct_with_layout<::gridtools::layout_map<0, 2, 1>>( shape[0], shape[1], shape[2] );
                if ( layout[0] == 1 && layout[1] == 0 && layout[2] == 2 )
                    return construct_with_layout<::gridtools::layout_map<1, 0, 2>>( shape[0], shape[1], shape[2] );
                if ( layout[0] == 1 && layout[1] == 2 && layout[2] == 0 )
                    return construct_with_layout<::gridtools::layout_map<1, 2, 0>>( shape[0], shape[1], shape[2] );
                if ( layout[0] == 2 && layout[1] == 0 && layout[2] == 1 )
                    return construct_with_layout<::gridtools::layout_map<2, 0, 1>>( shape[0], shape[1], shape[2] );
                if ( layout[0] == 2 && layout[1] == 1 && layout[2] == 0 )
                    return construct_with_layout<::gridtools::layout_map<2, 1, 0>>( shape[0], shape[1], shape[2] );
            }
            case 4: {
                if ( layout[0] == 0 && layout[1] == 1 && layout[2] == 2 && layout[3] == 3 )
                    return construct_with_layout<::gridtools::layout_map<0, 1, 2, 3>>( shape[0], shape[1], shape[2],
                                                                                       shape[3] );
                if ( layout[0] == 3 && layout[1] == 2 && layout[2] == 1 && layout[3] == 0 )
                    return construct_with_layout<::gridtools::layout_map<3, 2, 1, 0>>( shape[0], shape[1], shape[2],
                                                                                       shape[3] );
            }
            case 5: {
                if ( layout[0] == 0 && layout[1] == 1 && layout[2] == 2 && layout[3] == 3 && layout[4] == 4 )
                    return construct_with_layout<::gridtools::layout_map<0, 1, 2, 3, 4>>( shape[0], shape[1], shape[2],
                                                                                          shape[3], shape[4] );
                if ( layout[0] == 4 && layout[1] == 3 && layout[2] == 2 && layout[3] == 1 && layout[4] == 0 )
                    return construct_with_layout<::gridtools::layout_map<4, 3, 2, 1, 0>>( shape[0], shape[1], shape[2],
                                                                                          shape[3], shape[4] );
            }
            default: {
                std::stringstream err;
                if ( shape.size() > 5 )
                    err << "shape not recognized";
                else {
                    err << "Layout < ";
                    for ( size_t j = 0; j < layout.size(); ++j )
                        err << layout[j] << " ";
                    err << "> not implemented in Atlas.";
                }
                throw eckit::BadParameter( err.str(), Here() );
            }
        }
    }

    template <typename Layout, typename... UInts, typename = ::gridtools::is_all_integral<UInts...>>
    void construct_with_layout( UInts... dims ) {
        auto gt_data_store_ptr = create_gt_storage<Value, Layout>( dims... );
        using data_store_t     = typename std::remove_pointer<decltype( gt_data_store_ptr )>::type;
        array_.data_store_ =
            std::unique_ptr<ArrayDataStore>( new GridToolsDataStore<data_store_t>( gt_data_store_ptr ) );
        array_.spec_ = make_spec( gt_data_store_ptr, dims... );
    }

    template <typename... Ints>
    void resize_variadic( Ints... c ) {
        if ( sizeof...( c ) != array_.rank() ) {
            std::stringstream err;
            err << "Trying to resize an array of Rank " << array_.rank() << " by dimensions with Rank "
                << sizeof...( c ) << std::endl;
            throw eckit::BadParameter( err.str(), Here() );
        }

        check_dimension_lengths( array_.shape(), c... );

        if ( array_.valid() ) array_.syncHostDevice();

        Array* resized = Array::create<Value>( ArrayShape{(unsigned int)c...} );

        array_initializer<sizeof...( c )>::apply( array_, *resized );
        array_.replace( *resized );
        delete resized;
    }

    template <typename UInt, UInt... Indices>
    void apply_resize( const ArrayShape& shape, ::gridtools::gt_integer_sequence<UInt, Indices...> ) {
        return resize_variadic( shape[Indices]... );
    }

private:
    ArrayT<Value>& array_;
};

//------------------------------------------------------------------------------

template <typename Value>
Array* Array::create( size_t dim0 ) {
    return new ArrayT<Value>( dim0 );
}
template <typename Value>
Array* Array::create( size_t dim0, size_t dim1 ) {
    return new ArrayT<Value>( dim0, dim1 );
}
template <typename Value>
Array* Array::create( size_t dim0, size_t dim1, size_t dim2 ) {
    return new ArrayT<Value>( dim0, dim1, dim2 );
}
template <typename Value>
Array* Array::create( size_t dim0, size_t dim1, size_t dim2, size_t dim3 ) {
    return new ArrayT<Value>( dim0, dim1, dim2, dim3 );
}
template <typename Value>
Array* Array::create( size_t dim0, size_t dim1, size_t dim2, size_t dim3, size_t dim4 ) {
    return new ArrayT<Value>( dim0, dim1, dim2, dim3, dim4 );
}
template <typename Value>
Array* Array::create( const ArrayShape& shape ) {
    return new ArrayT<Value>( shape );
}
template <typename Value>
Array* Array::create( const ArrayShape& shape, const ArrayLayout& layout ) {
    return new ArrayT<Value>( shape, layout );
}
template <typename Value>
Array* Array::wrap( Value* data, const ArraySpec& spec ) {
    ArrayShape const& shape     = spec.shape();
    ArrayStrides const& strides = spec.strides();

    assert( shape.size() > 0 );

    switch ( shape.size() ) {
        case 1:
            return wrap_array( wrap_gt_storage<Value, 1>( data, get_array_from_vector<1>( shape ),
                                                          get_array_from_vector<1>( strides ) ),
                               spec );
        case 2:
            return wrap_array( wrap_gt_storage<Value, 2>( data, get_array_from_vector<2>( shape ),
                                                          get_array_from_vector<2>( strides ) ),
                               spec );
        case 3:
            return wrap_array( wrap_gt_storage<Value, 3>( data, get_array_from_vector<3>( shape ),
                                                          get_array_from_vector<3>( strides ) ),
                               spec );
        case 4:
            return wrap_array( wrap_gt_storage<Value, 4>( data, get_array_from_vector<4>( shape ),
                                                          get_array_from_vector<4>( strides ) ),
                               spec );
        case 5:
            return wrap_array( wrap_gt_storage<Value, 5>( data, get_array_from_vector<5>( shape ),
                                                          get_array_from_vector<5>( strides ) ),
                               spec );
        case 6:
            return wrap_array( wrap_gt_storage<Value, 6>( data, get_array_from_vector<6>( shape ),
                                                          get_array_from_vector<6>( strides ) ),
                               spec );
        case 7:
            return wrap_array( wrap_gt_storage<Value, 7>( data, get_array_from_vector<7>( shape ),
                                                          get_array_from_vector<7>( strides ) ),
                               spec );
        case 8:
            return wrap_array( wrap_gt_storage<Value, 8>( data, get_array_from_vector<8>( shape ),
                                                          get_array_from_vector<8>( strides ) ),
                               spec );
        case 9:
            return wrap_array( wrap_gt_storage<Value, 9>( data, get_array_from_vector<9>( shape ),
                                                          get_array_from_vector<9>( strides ) ),
                               spec );
        default: {
            std::stringstream err;
            err << "shape not recognized";
            throw eckit::BadParameter( err.str(), Here() );
        }
    }
}
template <typename Value>
Array* Array::wrap( Value* data, const ArrayShape& shape ) {
    return wrap( data, ArraySpec( shape ) );
}

Array* Array::create( DataType datatype, const ArrayShape& shape ) {
    switch ( datatype.kind() ) {
        case DataType::KIND_REAL64:
            return create<double>( shape );
        case DataType::KIND_REAL32:
            return create<float>( shape );
        case DataType::KIND_INT32:
            return create<int>( shape );
        case DataType::KIND_INT64:
            return create<long>( shape );
        case DataType::KIND_UINT64:
            return create<unsigned long>( shape );
        default: {
            std::stringstream err;
            err << "data kind " << datatype.kind() << " not recognised.";
            throw eckit::BadParameter( err.str(), Here() );
        }
    }
    return 0;
}

Array* Array::create( DataType datatype, const ArrayShape& shape, const ArrayLayout& layout ) {
    switch ( datatype.kind() ) {
        case DataType::KIND_REAL64:
            return create<double>( shape, layout );
        case DataType::KIND_REAL32:
            return create<float>( shape, layout );
        case DataType::KIND_INT32:
            return create<int>( shape, layout );
        case DataType::KIND_INT64:
            return create<long>( shape, layout );
        case DataType::KIND_UINT64:
            return create<unsigned long>( shape, layout );

        default: {
            std::stringstream err;
            err << "data kind " << datatype.kind() << " not recognised.";
            throw eckit::BadParameter( err.str(), Here() );
        }
    }
    return 0;
}

//------------------------------------------------------------------------------

Array::~Array() {}

//------------------------------------------------------------------------------

template <typename Value>
size_t ArrayT<Value>::footprint() const {
    size_t size = sizeof( *this );
    size += bytes();
    return size;
}

//------------------------------------------------------------------------------

template <typename Value>
bool ArrayT<Value>::accMap() const {
    if ( not acc_map_ ) {
#if ATLAS_GRIDTOOLS_STORAGE_BACKEND_CUDA && ATLAS_HAVE_ACC
        atlas_acc_map_data( (void*)host_data<Value>(), (void*)device_data<Value>(),
                            spec_.allocatedSize() * sizeof( Value ) );
        acc_map_ = true;
#endif
    }
    return acc_map_;
}

//------------------------------------------------------------------------------

template <typename Value>
void ArrayT<Value>::insert( size_t idx1, size_t size1 ) {
    // if( hostNeedsUpdate() ) {
    //    cloneFromDevice();
    //}
    if ( not hasDefaultLayout() ) NOTIMP;

    ArrayShape nshape = shape();
    if ( idx1 > nshape[0] ) {
        throw eckit::BadParameter( "can not insert into an array at a position beyond its size", Here() );
    }
    nshape[0] += size1;

    Array* resized = Array::create<Value>( nshape );

    array_initializer_partitioned<0>::apply( *this, *resized, idx1, size1 );

    replace( *resized );
    delete resized;
}

//------------------------------------------------------------------------------

template <typename Value>
void ArrayT<Value>::resize( size_t dim0 ) {
    ArrayT_impl<Value>( *this ).resize_variadic( dim0 );
}

template <typename Value>
void ArrayT<Value>::resize( size_t dim0, size_t dim1 ) {
    ArrayT_impl<Value>( *this ).resize_variadic( dim0, dim1 );
}

template <typename Value>
void ArrayT<Value>::resize( size_t dim0, size_t dim1, size_t dim2 ) {
    ArrayT_impl<Value>( *this ).resize_variadic( dim0, dim1, dim2 );
}

template <typename Value>
void ArrayT<Value>::resize( size_t dim0, size_t dim1, size_t dim2, size_t dim3 ) {
    ArrayT_impl<Value>( *this ).resize_variadic( dim0, dim1, dim2, dim3 );
}

template <typename Value>
void ArrayT<Value>::resize( size_t dim0, size_t dim1, size_t dim2, size_t dim3, size_t dim4 ) {
    ArrayT_impl<Value>( *this ).resize_variadic( dim0, dim1, dim2, dim3, dim4 );
}

template <typename Value>
void ArrayT<Value>::dump( std::ostream& out ) const {
    switch ( rank() ) {
        case 1:
            make_host_view<Value, 1, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 2:
            make_host_view<Value, 2, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 3:
            make_host_view<Value, 3, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 4:
            make_host_view<Value, 4, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 5:
            make_host_view<Value, 5, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 6:
            make_host_view<Value, 6, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 7:
            make_host_view<Value, 7, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 8:
            make_host_view<Value, 8, Intent::ReadOnly>( *this ).dump( out );
            break;
        case 9:
            make_host_view<Value, 9, Intent::ReadOnly>( *this ).dump( out );
            break;
        default:
            NOTIMP;
    }
}

template <typename Value>
void ArrayT<Value>::resize( const ArrayShape& shape ) {
    assert( shape.size() > 0 );
    switch ( shape.size() ) {
        case 1:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<1>() );
        case 2:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<2>() );
        case 3:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<3>() );
        case 4:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<4>() );
        case 5:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<5>() );
        case 6:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<6>() );
        case 7:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<7>() );
        case 8:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<8>() );
        case 9:
            return ArrayT_impl<Value>( *this ).apply_resize( shape, UintSequence<9>() );
        default: {
            std::stringstream err;
            err << "shape not recognized";
            throw eckit::BadParameter( err.str(), Here() );
        }
    }
}

//------------------------------------------------------------------------------

template <typename Value>
ArrayT<Value>::ArrayT( ArrayDataStore* ds, const ArraySpec& spec ) {
    data_store_ = std::unique_ptr<ArrayDataStore>( ds );

    spec_ = spec;
}

template <typename Value>
ArrayT<Value>::ArrayT( size_t dim0 ) {
    ArrayT_impl<Value>( *this ).construct( dim0 );
}
template <typename Value>
ArrayT<Value>::ArrayT( size_t dim0, size_t dim1 ) {
    ArrayT_impl<Value>( *this ).construct( dim0, dim1 );
}
template <typename Value>
ArrayT<Value>::ArrayT( size_t dim0, size_t dim1, size_t dim2 ) {
    ArrayT_impl<Value>( *this ).construct( dim0, dim1, dim2 );
}
template <typename Value>
ArrayT<Value>::ArrayT( size_t dim0, size_t dim1, size_t dim2, size_t dim3 ) {
    ArrayT_impl<Value>( *this ).construct( dim0, dim1, dim2, dim3 );
}
template <typename Value>
ArrayT<Value>::ArrayT( size_t dim0, size_t dim1, size_t dim2, size_t dim3, size_t dim4 ) {
    ArrayT_impl<Value>( *this ).construct( dim0, dim1, dim2, dim3, dim4 );
}

template <typename Value>
ArrayT<Value>::ArrayT( const ArrayShape& shape ) {
    ArrayT_impl<Value>( *this ).construct( shape );
}

template <typename Value>
ArrayT<Value>::ArrayT( const ArrayShape& shape, const ArrayLayout& layout ) {
    ArrayT_impl<Value>( *this ).construct( shape, layout );
}

template <typename Value>
ArrayT<Value>::ArrayT( const ArraySpec& spec ) {
    if ( not spec.contiguous() ) NOTIMP;
    ArrayT_impl<Value>( *this ).construct( spec.shape(), spec.layout() );
}

//------------------------------------------------------------------------------

}  // namespace array
}  // namespace atlas

//------------------------------------------------------------------------------
// Explicit template instantiations
namespace atlas {
namespace array {

template class ArrayT<int>;
template class ArrayT<long>;
template class ArrayT<float>;
template class ArrayT<double>;
template class ArrayT<unsigned long>;

template Array* Array::create<int>( size_t );
template Array* Array::create<long>( size_t );
template Array* Array::create<float>( size_t );
template Array* Array::create<double>( size_t );
template Array* Array::create<long unsigned>( size_t );

template Array* Array::create<int>( size_t, size_t );
template Array* Array::create<long>( size_t, size_t );
template Array* Array::create<float>( size_t, size_t );
template Array* Array::create<double>( size_t, size_t );
template Array* Array::create<long unsigned>( size_t, size_t );

template Array* Array::create<int>( size_t, size_t, size_t );
template Array* Array::create<long>( size_t, size_t, size_t );
template Array* Array::create<float>( size_t, size_t, size_t );
template Array* Array::create<double>( size_t, size_t, size_t );
template Array* Array::create<long unsigned>( size_t, size_t, size_t );

template Array* Array::create<int>( size_t, size_t, size_t, size_t );
template Array* Array::create<long>( size_t, size_t, size_t, size_t );
template Array* Array::create<float>( size_t, size_t, size_t, size_t );
template Array* Array::create<double>( size_t, size_t, size_t, size_t );
template Array* Array::create<long unsigned>( size_t, size_t, size_t, size_t );

template Array* Array::create<int>( size_t, size_t, size_t, size_t, size_t );
template Array* Array::create<long>( size_t, size_t, size_t, size_t, size_t );
template Array* Array::create<float>( size_t, size_t, size_t, size_t, size_t );
template Array* Array::create<double>( size_t, size_t, size_t, size_t, size_t );
template Array* Array::create<long unsigned>( size_t, size_t, size_t, size_t, size_t );

template Array* Array::create<int>( const ArrayShape& );
template Array* Array::create<long>( const ArrayShape& );
template Array* Array::create<float>( const ArrayShape& );
template Array* Array::create<double>( const ArrayShape& );
template Array* Array::create<long unsigned>( const ArrayShape& );

template Array* Array::create<int>( const ArrayShape&, const ArrayLayout& );
template Array* Array::create<long>( const ArrayShape&, const ArrayLayout& );
template Array* Array::create<float>( const ArrayShape&, const ArrayLayout& );
template Array* Array::create<double>( const ArrayShape&, const ArrayLayout& );
template Array* Array::create<long unsigned>( const ArrayShape&, const ArrayLayout& );

template Array* Array::wrap<int>( int*, const ArrayShape& );
template Array* Array::wrap<long>( long*, const ArrayShape& );
template Array* Array::wrap<float>( float*, const ArrayShape& );
template Array* Array::wrap<double>( double*, const ArrayShape& );
template Array* Array::wrap<long unsigned>( long unsigned*, const ArrayShape& );

template Array* Array::wrap<int>( int*, const ArraySpec& );
template Array* Array::wrap<long>( long*, const ArraySpec& );
template Array* Array::wrap<float>( float*, const ArraySpec& );
template Array* Array::wrap<double>( double*, const ArraySpec& );
template Array* Array::wrap<long unsigned>( long unsigned*, const ArraySpec& );

}  // namespace array
}  // namespace atlas
