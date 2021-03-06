/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#pragma once

#include <vector>

#include "eckit/memory/Owned.h"

#include "atlas/array/ArrayUtil.h"
#include "atlas/array/DataType.h"
#include "atlas/array_fwd.h"
#include "atlas/library/config.h"

namespace atlas {
namespace array {

// --------------------------------------------------------------------------------------------
// Forward declarations

template <typename Value>
class ArrayT;
template <typename Value>
class ArrayT_impl;

// --------------------------------------------------------------------------------------------

class Array : public eckit::Owned {
public:
    virtual ~Array();

    static Array* create( array::DataType, const ArrayShape& );

    static Array* create( array::DataType, const ArrayShape&, const ArrayLayout& );

    virtual size_t footprint() const = 0;

    template <typename Value>
    static Array* create( size_t size0 );
    template <typename Value>
    static Array* create( size_t size0, size_t size1 );
    template <typename Value>
    static Array* create( size_t size0, size_t size1, size_t size2 );
    template <typename Value>
    static Array* create( size_t size0, size_t size1, size_t size2, size_t size3 );
    template <typename Value>
    static Array* create( size_t size0, size_t size1, size_t size2, size_t size3, size_t size4 );

    template <typename Value>
    static Array* create( const ArrayShape& shape );

    template <typename Value>
    static Array* create( const ArrayShape& shape, const ArrayLayout& layout );

    template <typename Value>
    static Array* wrap( Value* data, const ArrayShape& shape );

    template <typename Value>
    static Array* wrap( Value* data, const ArraySpec& spec );

    size_t bytes() const { return sizeof_data() * spec().allocatedSize(); }

    size_t size() const { return spec_.size(); }

    size_t rank() const { return spec_.rank(); }

    size_t stride( size_t i ) const { return spec_.strides()[i]; }

    size_t shape( size_t i ) const { return spec_.shape()[i]; }

    const ArrayStrides& strides() const { return spec_.strides(); }

    const ArrayShape& shape() const { return spec_.shape(); }

    const std::vector<int>& shapef() const { return spec_.shapef(); }

    const std::vector<int>& stridesf() const { return spec_.stridesf(); }

    bool contiguous() const { return spec_.contiguous(); }

    bool hasDefaultLayout() const { return spec_.hasDefaultLayout(); }

    virtual array::DataType datatype() const = 0;

    virtual size_t sizeof_data() const = 0;

    virtual void resize( const ArrayShape& shape ) = 0;

    virtual void resize( size_t size0 )                                                         = 0;
    virtual void resize( size_t size0, size_t size1 )                                           = 0;
    virtual void resize( size_t size0, size_t size1, size_t size2 )                             = 0;
    virtual void resize( size_t size0, size_t size1, size_t size2, size_t size3 )               = 0;
    virtual void resize( size_t size0, size_t size1, size_t size2, size_t size3, size_t size4 ) = 0;

    virtual void insert( size_t idx1, size_t size1 ) = 0;

    virtual void dump( std::ostream& os ) const = 0;

    virtual bool accMap() const = 0;

    virtual void* storage() { return data_store_->voidDataStore(); }

    virtual const void* storage() const { return data_store_->voidDataStore(); }

    void cloneToDevice() const { data_store_->cloneToDevice(); }

    void cloneFromDevice() const { data_store_->cloneFromDevice(); }

    bool valid() const { return data_store_->valid(); }

    void syncHostDevice() const { data_store_->syncHostDevice(); }

    bool hostNeedsUpdate() const { return data_store_->hostNeedsUpdate(); }

    bool deviceNeedsUpdate() const { return data_store_->deviceNeedsUpdate(); }

    void reactivateDeviceWriteViews() const { data_store_->reactivateDeviceWriteViews(); }

    void reactivateHostWriteViews() const { data_store_->reactivateHostWriteViews(); }

    const ArraySpec& spec() const { return spec_; }

    // -- dangerous methods... You're on your own interpreting the raw data
    template <typename DATATYPE>
    DATATYPE const* host_data() const {
        return data_store_->hostData<DATATYPE>();
    }
    template <typename DATATYPE>
    DATATYPE* host_data() {
        return data_store_->hostData<DATATYPE>();
    }
    template <typename DATATYPE>
    DATATYPE const* device_data() const {
        return data_store_->deviceData<DATATYPE>();
    }
    template <typename DATATYPE>
    DATATYPE* device_data() {
        return data_store_->deviceData<DATATYPE>();
    }
    template <typename DATATYPE>
    DATATYPE const* data() const {
        return data_store_->hostData<DATATYPE>();
    }
    template <typename DATATYPE>
    DATATYPE* data() {
        return data_store_->hostData<DATATYPE>();
    }

    ArrayDataStore const* data_store() const { return data_store_.get(); }

protected:
    ArraySpec spec_;
    std::unique_ptr<ArrayDataStore> data_store_;

    void replace( Array& array ) {
        data_store_.swap( array.data_store_ );
        spec_ = array.spec_;
    }
};

// --------------------------------------------------------------------------------------------

template <typename Value>
class ArrayT : public Array {
public:
    ArrayT( size_t size0 );
    ArrayT( size_t size0, size_t size1 );
    ArrayT( size_t size0, size_t size1, size_t size2 );
    ArrayT( size_t size0, size_t size1, size_t size2, size_t size3 );
    ArrayT( size_t size0, size_t size1, size_t size2, size_t size3, size_t size4 );

    ArrayT( const ArraySpec& );

    ArrayT( const ArrayShape& );

    ArrayT( const ArrayShape&, const ArrayLayout& );

    virtual void insert( size_t idx1, size_t size1 );

    virtual void resize( const ArrayShape& );

    virtual void resize( size_t size0 );
    virtual void resize( size_t size0, size_t size1 );
    virtual void resize( size_t size0, size_t size1, size_t size2 );
    virtual void resize( size_t size0, size_t size1, size_t size2, size_t size3 );
    virtual void resize( size_t size0, size_t size1, size_t size2, size_t size3, size_t size4 );

    virtual array::DataType datatype() const { return array::DataType::create<Value>(); }

    virtual size_t sizeof_data() const { return sizeof( Value ); }

    virtual void dump( std::ostream& os ) const;

    // This constructor is used through the Array::create() or the Array::wrap()
    // methods
    ArrayT( ArrayDataStore*, const ArraySpec& );

    virtual size_t footprint() const;

    virtual bool accMap() const;

private:
    template <typename T>
    friend class ArrayT_impl;
    mutable bool acc_map_{false};
};

}  // namespace array
}  // namespace atlas
