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

#include <functional>
#include <string>

#include "eckit/memory/Builder.h"
#include "eckit/memory/Owned.h"

#include "atlas/domain/Domain.h"
#include "atlas/projection/Projection.h"
#include "atlas/util/Config.h"
#include "atlas/util/Point.h"

namespace eckit {
class Hash;
}
namespace atlas {
namespace grid {
namespace detail {
namespace grid {

class Grid : public eckit::Owned {
public:  // types
    using Projection = atlas::Projection;
    using Domain     = atlas::Domain;
    using Config     = atlas::util::Config;
    using Spec       = atlas::util::Config;
    using builder_t  = eckit::BuilderT1<Grid>;
    using ARG1       = const Config&;
    using uid_t      = std::string;
    using hash_t     = std::string;

    class IteratorXY {
    public:
        using Predicate                                          = std::function<bool( long )>;
        virtual bool next( PointXY& )                            = 0;
        virtual const PointXY operator*() const                  = 0;
        virtual const IteratorXY& operator++()                   = 0;
        virtual bool operator==( const IteratorXY& other ) const = 0;
        virtual bool operator!=( const IteratorXY& other ) const = 0;
    };

    class IteratorLonLat {
    public:
        virtual bool next( PointLonLat& )                            = 0;
        virtual const PointLonLat operator*() const                  = 0;
        virtual const IteratorLonLat& operator++()                   = 0;
        virtual bool operator==( const IteratorLonLat& other ) const = 0;
        virtual bool operator!=( const IteratorLonLat& other ) const = 0;
    };

public:  // methods
    static std::string className();

    static const Grid* create( const Config& );

    static const Grid* create( const std::string& name, const Config& = Config() );

    /// ctor (default)
    Grid();

    /// dtor
    virtual ~Grid();

    /// Human readable name (may not be unique)
    virtual std::string name() const = 0;
    virtual std::string type() const = 0;

    /// Unique grid id
    /// Computed from the hash. Can be used to compare 2 grids.
    uid_t uid() const;

    /// Adds to the hash the information that makes this Grid unique
    virtual void hash( eckit::Hash& ) const = 0;

    /// @returns the hash of the information that makes this Grid unique
    std::string hash() const;

    /// @return area represented by the grid
    const Domain& domain() const { return domain_; }

    /// @return projection (mapping between geographic coordinates and grid
    /// coordinates)
    const Projection& projection() const { return projection_; }

    /// @return number of grid points
    /// @note This methods should have constant access time, if necessary derived
    //        classes should compute it at construction
    virtual size_t size() const = 0;

    virtual Spec spec() const = 0;

    virtual IteratorXY* xy_begin() const                          = 0;
    virtual IteratorXY* xy_end() const                            = 0;
    virtual IteratorXY* xy_begin( IteratorXY::Predicate p ) const = 0;
    virtual IteratorXY* xy_end( IteratorXY::Predicate p ) const   = 0;
    virtual IteratorLonLat* lonlat_begin() const                  = 0;
    virtual IteratorLonLat* lonlat_end() const                    = 0;

protected:  // methods
    /// Fill provided me
    virtual void print( std::ostream& ) const = 0;

private:  // methods
    friend std::ostream& operator<<( std::ostream& s, const Grid& p ) {
        p.print( s );
        return s;
    }

private:  // members
    /// Cache the unique ID
    mutable uid_t uid_;

    /// Cache the hash
    mutable hash_t hash_;

protected:  // members
    Projection projection_;
    Domain domain_;
};

}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
