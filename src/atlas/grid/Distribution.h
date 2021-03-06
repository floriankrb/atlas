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
#include "eckit/memory/SharedPtr.h"

#include "atlas/library/config.h"

namespace atlas {
class Grid;
namespace grid {
class Partitioner;
}
}  // namespace atlas

namespace atlas {
namespace grid {

class Distribution {
    friend class Partitioner;

public:
    class impl_t : public eckit::Owned {
    public:
        impl_t( const Grid& );

        impl_t( const Grid&, const Partitioner& );

        impl_t( size_t npts, int partition[], int part0 = 0 );

        virtual ~impl_t() {}

        int partition( const gidx_t gidx ) const { return part_[gidx]; }

        const std::vector<int>& partition() const { return part_; }

        size_t nb_partitions() const { return nb_partitions_; }

        operator const std::vector<int>&() const { return part_; }

        const int* data() const { return part_.data(); }

        const std::vector<int>& nb_pts() const { return nb_pts_; }

        size_t max_pts() const { return max_pts_; }
        size_t min_pts() const { return min_pts_; }

        const std::string& type() const { return type_; }

        void print( std::ostream& ) const;

    private:
        size_t nb_partitions_;
        std::vector<int> part_;
        std::vector<int> nb_pts_;
        size_t max_pts_;
        size_t min_pts_;
        std::string type_;
    };

public:
    Distribution();
    Distribution( const impl_t* );
    Distribution( const Distribution& );

    Distribution( const Grid& );

    Distribution( const Grid&, const Partitioner& );

    Distribution( size_t npts, int partition[], int part0 = 0 );

    ~Distribution() {}

    int partition( const gidx_t gidx ) const { return impl_->partition( gidx ); }

    const std::vector<int>& partition() const { return impl_->partition(); }

    size_t nb_partitions() const { return impl_->nb_partitions(); }

    operator const std::vector<int>&() const { return *impl_; }

    const int* data() const { return impl_->data(); }

    const std::vector<int>& nb_pts() const { return impl_->nb_pts(); }

    size_t max_pts() const { return impl_->max_pts(); }
    size_t min_pts() const { return impl_->min_pts(); }

    const std::string& type() const { return impl_->type(); }

    friend std::ostream& operator<<( std::ostream& os, const Distribution& distribution ) {
        distribution.impl_->print( os );
        return os;
    }

    const impl_t* get() const { return impl_.get(); }

private:
    eckit::SharedPtr<const impl_t> impl_;
};

extern "C" {
Distribution::impl_t* atlas__GridDistribution__new( int npts, int part[], int part0 );
void atlas__GridDistribution__delete( Distribution::impl_t* This );
}

}  // namespace grid
}  // namespace atlas
