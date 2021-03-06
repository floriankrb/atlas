/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "eckit/config/Parametrisation.h"
#include "eckit/exception/Exceptions.h"

#include "atlas/array/ArrayView.h"
#include "atlas/array/MakeView.h"
#include "atlas/field/Field.h"
#include "atlas/mesh/HybridElements.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/mesh/Nodes.h"
#include "atlas/numerics/fvm/Method.h"
#include "atlas/numerics/fvm/Nabla.h"
#include "atlas/parallel/omp/omp.h"
#include "atlas/runtime/Log.h"
#include "atlas/util/CoordinateEnums.h"

// =======================================================

using Topology = atlas::mesh::Nodes::Topology;
using Range    = atlas::array::Range;

namespace atlas {
namespace numerics {
namespace fvm {

namespace {
static NablaBuilder<Nabla> __fvm_nabla( "fvm" );
}

Nabla::Nabla( const numerics::Method& method, const eckit::Parametrisation& p ) :
    atlas::numerics::Nabla::nabla_t( method, p ) {
    fvm_ = dynamic_cast<const fvm::Method*>( &method );
    if ( !fvm_ ) throw eckit::BadCast( "atlas::numerics::fvm::Nabla needs a atlas::numerics::fvm::Method", Here() );
    Log::debug() << "Nabla constructed for method " << fvm_->name() << " with "
                 << fvm_->node_columns().nb_nodes_global() << " nodes total" << std::endl;

    setup();
}

Nabla::~Nabla() {}

void Nabla::setup() {
    const mesh::Edges& edges = fvm_->mesh().edges();

    const size_t nedges = edges.size();

    const array::ArrayView<int, 1> edge_is_pole = array::make_view<int, 1>( edges.field( "is_pole_edge" ) );

    // Filter pole_edges out of all edges
    std::vector<size_t> tmp( nedges );
    size_t c( 0 );
    for ( size_t jedge = 0; jedge < nedges; ++jedge ) {
        if ( edge_is_pole( jedge ) ) tmp[c++] = jedge;
    }
    pole_edges_.clear();
    pole_edges_.reserve( c );
    for ( size_t jedge = 0; jedge < c; ++jedge )
        pole_edges_.push_back( tmp[jedge] );
}

void Nabla::gradient( const Field& field, Field& grad_field ) const {
    if ( field.variables() > 1 ) { return gradient_of_vector( field, grad_field ); }
    else {
        return gradient_of_scalar( field, grad_field );
    }
    throw eckit::SeriousBug( "Cannot figure out if field is a scalar or vector field", Here() );
}

void Nabla::gradient_of_scalar( const Field& scalar_field, Field& grad_field ) const {
    Log::debug() << "Compute gradient of scalar field " << scalar_field.name() << " with fvm method" << std::endl;
    const double radius  = fvm_->radius();
    const double deg2rad = M_PI / 180.;

    const mesh::Edges& edges = fvm_->mesh().edges();
    const mesh::Nodes& nodes = fvm_->mesh().nodes();

    const size_t nnodes = nodes.size();
    const size_t nedges = edges.size();
    const size_t nlev   = scalar_field.levels() ? scalar_field.levels() : 1;
    if ( ( grad_field.levels() ? grad_field.levels() : 1 ) != nlev )
        throw eckit::AssertionFailed( "gradient field should have same number of levels", Here() );

    const auto scalar = scalar_field.levels()
                            ? array::make_view<double, 2>( scalar_field ).slice( Range::all(), Range::all() )
                            : array::make_view<double, 1>( scalar_field ).slice( Range::all(), Range::dummy() );
    auto grad = grad_field.levels()
                    ? array::make_view<double, 3>( grad_field ).slice( Range::all(), Range::all(), Range::all() )
                    : array::make_view<double, 2>( grad_field ).slice( Range::all(), Range::dummy(), Range::all() );

    const auto lonlat_deg     = array::make_view<double, 2>( nodes.lonlat() );
    const auto dual_volumes   = array::make_view<double, 1>( nodes.field( "dual_volumes" ) );
    const auto dual_normals   = array::make_view<double, 2>( edges.field( "dual_normals" ) );
    const auto node2edge_sign = array::make_view<double, 2>( nodes.field( "node2edge_sign" ) );

    const mesh::Connectivity& node2edge           = nodes.edge_connectivity();
    const mesh::MultiBlockConnectivity& edge2node = edges.node_connectivity();

    array::ArrayT<double> avgS_arr( nedges, nlev, 2ul );
    auto avgS = array::make_view<double, 3>( avgS_arr );

    const double scale = deg2rad * deg2rad * radius;

    atlas_omp_parallel {
        atlas_omp_for( size_t jedge = 0; jedge < nedges; ++jedge ) {
            int ip1 = edge2node( jedge, 0 );
            int ip2 = edge2node( jedge, 1 );

            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                double avg               = ( scalar( ip1, jlev ) + scalar( ip2, jlev ) ) * 0.5;
                avgS( jedge, jlev, LON ) = dual_normals( jedge, LON ) * deg2rad * avg;
                avgS( jedge, jlev, LAT ) = dual_normals( jedge, LAT ) * deg2rad * avg;
            }
        }

        atlas_omp_for( size_t jnode = 0; jnode < nnodes; ++jnode ) {
            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                grad( jnode, jlev, LON ) = 0.;
                grad( jnode, jlev, LAT ) = 0.;
            }
            for ( size_t jedge = 0; jedge < node2edge.cols( jnode ); ++jedge ) {
                const int iedge  = node2edge( jnode, jedge );
                const double add = node2edge_sign( jnode, jedge );
                for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                    grad( jnode, jlev, LON ) += add * avgS( iedge, jlev, LON );
                    grad( jnode, jlev, LAT ) += add * avgS( iedge, jlev, LAT );
                }
            }
            const double y        = lonlat_deg( jnode, LAT ) * deg2rad;
            const double metric_y = 1. / ( dual_volumes( jnode ) * scale );
            const double metric_x = metric_y / std::cos( y );
            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                grad( jnode, jlev, LON ) *= metric_x;
                grad( jnode, jlev, LAT ) *= metric_y;
            }
        }
    }
}

// ================================================================================

void Nabla::gradient_of_vector( const Field& vector_field, Field& grad_field ) const {
    Log::debug() << "Compute gradient of vector field " << vector_field.name() << " with fvm method" << std::endl;
    const double radius  = fvm_->radius();
    const double deg2rad = M_PI / 180.;

    const mesh::Edges& edges = fvm_->mesh().edges();
    const mesh::Nodes& nodes = fvm_->mesh().nodes();

    const size_t nnodes = nodes.size();
    const size_t nedges = edges.size();
    const size_t nlev   = vector_field.levels();
    if ( vector_field.levels() != nlev )
        throw eckit::AssertionFailed( "gradient field should have same number of levels", Here() );

    const auto vector =
        vector_field.levels()
            ? array::make_view<double, 3>( vector_field ).slice( Range::all(), Range::all(), Range::all() )
            : array::make_view<double, 2>( vector_field ).slice( Range::all(), Range::dummy(), Range::all() );
    auto grad = grad_field.levels()
                    ? array::make_view<double, 3>( grad_field ).slice( Range::all(), Range::all(), Range::all() )
                    : array::make_view<double, 2>( grad_field ).slice( Range::all(), Range::dummy(), Range::all() );

    const array::ArrayView<double, 2> lonlat_deg     = array::make_view<double, 2>( nodes.lonlat() );
    const array::ArrayView<double, 1> dual_volumes   = array::make_view<double, 1>( nodes.field( "dual_volumes" ) );
    const array::ArrayView<double, 2> dual_normals   = array::make_view<double, 2>( edges.field( "dual_normals" ) );
    const array::ArrayView<int, 1> edge_is_pole      = array::make_view<int, 1>( edges.field( "is_pole_edge" ) );
    const array::ArrayView<double, 2> node2edge_sign = array::make_view<double, 2>( nodes.field( "node2edge_sign" ) );

    const mesh::Connectivity& node2edge           = nodes.edge_connectivity();
    const mesh::MultiBlockConnectivity& edge2node = edges.node_connectivity();

    array::ArrayT<double> avgS_arr( nedges, nlev, 4ul );
    array::ArrayView<double, 3> avgS = array::make_view<double, 3>( avgS_arr );

    const double scale = deg2rad * deg2rad * radius;

    enum
    {
        LONdLON = 0,
        LONdLAT = 1,
        LATdLON = 2,
        LATdLAT = 3
    };

    atlas_omp_parallel {
        atlas_omp_for( size_t jedge = 0; jedge < nedges; ++jedge ) {
            int ip1    = edge2node( jedge, 0 );
            int ip2    = edge2node( jedge, 1 );
            double pbc = 1. - 2. * edge_is_pole( jedge );

            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                double avg[2]                = {( vector( ip1, jlev, LON ) + pbc * vector( ip2, jlev, LON ) ) * 0.5,
                                 ( vector( ip1, jlev, LAT ) + pbc * vector( ip2, jlev, LAT ) ) * 0.5};
                avgS( jedge, jlev, LONdLON ) = dual_normals( jedge, LON ) * deg2rad * avg[LON];
                // above = 0 at pole because of dual_normals
                avgS( jedge, jlev, LONdLAT ) = dual_normals( jedge, LAT ) * deg2rad * avg[LON];
                avgS( jedge, jlev, LATdLON ) = dual_normals( jedge, LON ) * deg2rad * avg[LAT];
                // above = 0 at pole because of dual_normals
                avgS( jedge, jlev, LATdLAT ) = dual_normals( jedge, LAT ) * deg2rad * avg[LAT];
            }
        }

        atlas_omp_for( size_t jnode = 0; jnode < nnodes; ++jnode ) {
            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                grad( jnode, jlev, LONdLON ) = 0.;
                grad( jnode, jlev, LONdLAT ) = 0.;
                grad( jnode, jlev, LATdLON ) = 0.;
                grad( jnode, jlev, LATdLAT ) = 0.;
            }
            for ( size_t jedge = 0; jedge < node2edge.cols( jnode ); ++jedge ) {
                const int iedge = node2edge( jnode, jedge );
                double add      = node2edge_sign( jnode, jedge );
                for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                    grad( jnode, jlev, LONdLON ) += add * avgS( iedge, jlev, LONdLON );
                    grad( jnode, jlev, LONdLAT ) += add * avgS( iedge, jlev, LONdLAT );
                    grad( jnode, jlev, LATdLON ) += add * avgS( iedge, jlev, LATdLON );
                    grad( jnode, jlev, LATdLAT ) += add * avgS( iedge, jlev, LATdLAT );
                }
            }
            const double y        = lonlat_deg( jnode, LAT ) * deg2rad;
            const double metric_y = 1. / ( dual_volumes( jnode ) * scale );
            const double metric_x = metric_y / std::cos( y );
            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                grad( jnode, jlev, LONdLON ) *= metric_x;
                grad( jnode, jlev, LATdLON ) *= metric_x;
                grad( jnode, jlev, LONdLAT ) *= metric_y;
                grad( jnode, jlev, LATdLAT ) *= metric_y;
            }
        }
    }
    // Fix wrong node2edge_sign for vector quantities
    for ( size_t jedge = 0; jedge < pole_edges_.size(); ++jedge ) {
        const int iedge       = pole_edges_[jedge];
        const int jnode       = edge2node( iedge, 1 );
        const double metric_y = 1. / ( dual_volumes( jnode ) * scale );
        for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
            grad( jnode, jlev, LONdLAT ) -= 2. * avgS( iedge, jlev, LONdLAT ) * metric_y;
            grad( jnode, jlev, LATdLAT ) -= 2. * avgS( iedge, jlev, LATdLAT ) * metric_y;
        }
    }
}

// ================================================================================

void Nabla::divergence( const Field& vector_field, Field& div_field ) const {
    const double radius  = fvm_->radius();
    const double deg2rad = M_PI / 180.;

    const mesh::Edges& edges = fvm_->mesh().edges();
    const mesh::Nodes& nodes = fvm_->mesh().nodes();

    const size_t nnodes = nodes.size();
    const size_t nedges = edges.size();
    const size_t nlev   = vector_field.levels();
    if ( div_field.levels() != nlev )
        throw eckit::AssertionFailed( "divergence field should have same number of levels", Here() );

    const auto vector =
        vector_field.levels()
            ? array::make_view<double, 3>( vector_field ).slice( Range::all(), Range::all(), Range::all() )
            : array::make_view<double, 2>( vector_field ).slice( Range::all(), Range::dummy(), Range::all() );
    auto div = div_field.levels() ? array::make_view<double, 2>( div_field ).slice( Range::all(), Range::all() )
                                  : array::make_view<double, 1>( div_field ).slice( Range::all(), Range::dummy() );

    const auto lonlat_deg                         = array::make_view<double, 2>( nodes.lonlat() );
    const auto dual_volumes                       = array::make_view<double, 1>( nodes.field( "dual_volumes" ) );
    const auto dual_normals                       = array::make_view<double, 2>( edges.field( "dual_normals" ) );
    const auto edge_is_pole                       = array::make_view<int, 1>( edges.field( "is_pole_edge" ) );
    const auto node2edge_sign                     = array::make_view<double, 2>( nodes.field( "node2edge_sign" ) );
    const mesh::Connectivity& node2edge           = nodes.edge_connectivity();
    const mesh::MultiBlockConnectivity& edge2node = edges.node_connectivity();

    array::ArrayT<double> avgS_arr( nedges, nlev, 2ul );
    array::ArrayView<double, 3> avgS = array::make_view<double, 3>( avgS_arr );

    const double scale = deg2rad * deg2rad * radius;

    atlas_omp_parallel {
        atlas_omp_for( size_t jedge = 0; jedge < nedges; ++jedge ) {
            size_t ip1   = edge2node( jedge, 0 );
            size_t ip2   = edge2node( jedge, 1 );
            double y1    = lonlat_deg( ip1, LAT ) * deg2rad;
            double y2    = lonlat_deg( ip2, LAT ) * deg2rad;
            double cosy1 = std::cos( y1 );
            double cosy2 = std::cos( y2 );

            double pbc = 1. - edge_is_pole( jedge );

            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                double avg[2] = {
                    ( vector( ip1, jlev, LON ) + vector( ip2, jlev, LON ) ) * 0.5,
                    ( cosy1 * vector( ip1, jlev, LAT ) + cosy2 * vector( ip2, jlev, LAT ) ) * 0.5 *
                        pbc  // (force cos(y)=0 at pole)
                };
                avgS( jedge, jlev, LON ) = dual_normals( jedge, LON ) * deg2rad * avg[LON];
                // above = 0 at pole by construction of S
                avgS( jedge, jlev, LAT ) = dual_normals( jedge, LAT ) * deg2rad * avg[LAT];
                // above = 0 at pole by construction of pbc
                // We don't need the cross terms for divergence,
                //    i.e.      dual_normals(jedge,LON)*deg2rad*avg[LAT]
                //        and   dual_normals(jedge,LAT)*deg2rad*avg[LON]
            }
        }

        atlas_omp_for( size_t jnode = 0; jnode < nnodes; ++jnode ) {
            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                div( jnode, jlev ) = 0.;
            }
            for ( size_t jedge = 0; jedge < node2edge.cols( jnode ); ++jedge ) {
                int iedge  = node2edge( jnode, jedge );
                double add = node2edge_sign( jnode, jedge );
                for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                    div( jnode, jlev ) += add * ( avgS( iedge, jlev, LON ) + avgS( iedge, jlev, LAT ) );
                }
            }
            const double y = lonlat_deg( jnode, LAT ) * deg2rad;
            double metric  = 1. / ( dual_volumes( jnode ) * scale * std::cos( y ) );
            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                div( jnode, jlev ) *= metric;
            }
        }
    }
}

void Nabla::curl( const Field& vector_field, Field& curl_field ) const {
    const double radius  = fvm_->radius();
    const double deg2rad = M_PI / 180.;

    const mesh::Edges& edges = fvm_->mesh().edges();
    const mesh::Nodes& nodes = fvm_->mesh().nodes();

    const size_t nnodes = nodes.size();
    const size_t nedges = edges.size();
    const size_t nlev   = vector_field.levels();
    if ( curl_field.levels() != nlev )
        throw eckit::AssertionFailed( "curl field should have same number of levels", Here() );

    const auto vector =
        vector_field.levels()
            ? array::make_view<double, 3>( vector_field ).slice( Range::all(), Range::all(), Range::all() )
            : array::make_view<double, 2>( vector_field ).slice( Range::all(), Range::dummy(), Range::all() );
    auto curl = curl_field.levels() ? array::make_view<double, 2>( curl_field ).slice( Range::all(), Range::all() )
                                    : array::make_view<double, 1>( curl_field ).slice( Range::all(), Range::dummy() );

    const auto lonlat_deg     = array::make_view<double, 2>( nodes.lonlat() );
    const auto dual_volumes   = array::make_view<double, 1>( nodes.field( "dual_volumes" ) );
    const auto dual_normals   = array::make_view<double, 2>( edges.field( "dual_normals" ) );
    const auto edge_is_pole   = array::make_view<int, 1>( edges.field( "is_pole_edge" ) );
    const auto node2edge_sign = array::make_view<double, 2>( nodes.field( "node2edge_sign" ) );

    const mesh::Connectivity& node2edge           = nodes.edge_connectivity();
    const mesh::MultiBlockConnectivity& edge2node = edges.node_connectivity();

    array::ArrayT<double> avgS_arr( nedges, nlev, 2ul );
    array::ArrayView<double, 3> avgS = array::make_view<double, 3>( avgS_arr );

    const double scale = deg2rad * deg2rad * radius * radius;

    atlas_omp_parallel {
        atlas_omp_for( size_t jedge = 0; jedge < nedges; ++jedge ) {
            size_t ip1    = edge2node( jedge, 0 );
            size_t ip2    = edge2node( jedge, 1 );
            double y1     = lonlat_deg( ip1, LAT ) * deg2rad;
            double y2     = lonlat_deg( ip2, LAT ) * deg2rad;
            double rcosy1 = radius * std::cos( y1 );
            double rcosy2 = radius * std::cos( y2 );

            double pbc = 1 - edge_is_pole( jedge );

            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                double avg[2] = {( rcosy1 * vector( ip1, jlev, LON ) + rcosy2 * vector( ip2, jlev, LON ) ) * 0.5 *
                                     pbc,  // (force R*cos(y)=0 at pole)
                                 ( radius * vector( ip1, jlev, LAT ) + radius * vector( ip2, jlev, LAT ) ) * 0.5};
                avgS( jedge, jlev, LON ) = dual_normals( jedge, LAT ) * deg2rad * avg[LON];
                // above = 0 at pole by construction of pbc
                avgS( jedge, jlev, LAT ) = dual_normals( jedge, LON ) * deg2rad * avg[LAT];
                // above = 0 at pole by construction of S
                // We don't need the non-cross terms for curl, i.e.
                //          dual_normals(jedge,LON)*deg2rad*avg[LON]
                //   and    dual_normals(jedge,LAT)*deg2rad*avg[LAT]
            }
        }

        atlas_omp_for( size_t jnode = 0; jnode < nnodes; ++jnode ) {
            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                curl( jnode, jlev ) = 0.;
            }
            for ( size_t jedge = 0; jedge < node2edge.cols( jnode ); ++jedge ) {
                size_t iedge = node2edge( jnode, jedge );
                double add   = node2edge_sign( jnode, jedge );
                for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                    curl( jnode, jlev ) += add * ( avgS( iedge, jlev, LAT ) - avgS( iedge, jlev, LON ) );
                }
            }
            double y      = lonlat_deg( jnode, LAT ) * deg2rad;
            double metric = 1. / ( dual_volumes( jnode ) * scale * std::cos( y ) );
            for ( size_t jlev = 0; jlev < nlev; ++jlev ) {
                curl( jnode, jlev ) *= metric;
            }
        }
    }
}

void Nabla::laplacian( const Field& scalar, Field& lapl ) const {
    Field grad( fvm_->node_columns().createField<double>( option::name( "grad" ) | option::levels( scalar.levels() ) |
                                                          option::variables( 2 ) ) );
    gradient( scalar, grad );
    if ( fvm_->node_columns().halo().size() < 2 ) fvm_->node_columns().haloExchange( grad );
    divergence( grad, lapl );
}

}  // namespace fvm
}  // namespace numerics
}  // namespace atlas
