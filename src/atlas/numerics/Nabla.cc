/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <map>
#include <string>

#include "eckit/exception/Exceptions.h"
#include "eckit/thread/AutoLock.h"
#include "eckit/thread/Mutex.h"

#include "atlas/library/config.h"
#include "atlas/numerics/Method.h"
#include "atlas/numerics/Nabla.h"
#include "atlas/numerics/fvm/Method.h"
#include "atlas/numerics/fvm/Nabla.h"
#include "atlas/runtime/ErrorHandling.h"
#include "atlas/runtime/Log.h"
#include "atlas/util/Config.h"

namespace {

static eckit::Mutex* local_mutex                                = 0;
static std::map<std::string, atlas::numerics::NablaFactory*>* m = 0;
static pthread_once_t once                                      = PTHREAD_ONCE_INIT;

static void init() {
    local_mutex = new eckit::Mutex();
    m           = new std::map<std::string, atlas::numerics::NablaFactory*>();
}
}  // namespace

namespace atlas {
namespace numerics {

NablaImpl::NablaImpl( const Method& method, const eckit::Parametrisation& p ) {}

NablaImpl::~NablaImpl() {}

Nabla::Nabla() : nabla_( nullptr ) {}

Nabla::Nabla( const Nabla::nabla_t* nabla ) : nabla_( nabla ) {}

Nabla::Nabla( const Nabla& nabla ) : nabla_( nabla.nabla_ ) {}

Nabla::Nabla( const Method& method, const eckit::Parametrisation& p ) : nabla_( NablaFactory::build( method, p ) ) {}

Nabla::Nabla( const Method& method ) : Nabla( method, util::NoConfig() ) {}

void Nabla::gradient( const Field& scalar, Field& grad ) const {
    nabla_->gradient( scalar, grad );
}

void Nabla::divergence( const Field& vector, Field& div ) const {
    nabla_->divergence( vector, div );
}

void Nabla::curl( const Field& vector, Field& curl ) const {
    nabla_->curl( vector, curl );
}

void Nabla::laplacian( const Field& scalar, Field& laplacian ) const {
    nabla_->laplacian( scalar, laplacian );
}

namespace {

template <typename T>
void load_builder() {
    NablaBuilder<T>( "tmp" );
}

struct force_link {
    force_link() { load_builder<fvm::Nabla>(); }
};

}  // namespace

NablaFactory::NablaFactory( const std::string& name ) : name_( name ) {
    pthread_once( &once, init );

    eckit::AutoLock<eckit::Mutex> lock( local_mutex );

    ASSERT( m->find( name ) == m->end() );
    ( *m )[name] = this;
}

NablaFactory::~NablaFactory() {
    eckit::AutoLock<eckit::Mutex> lock( local_mutex );
    m->erase( name_ );
}

void NablaFactory::list( std::ostream& out ) {
    pthread_once( &once, init );

    eckit::AutoLock<eckit::Mutex> lock( local_mutex );

    static force_link static_linking;

    const char* sep = "";
    for ( std::map<std::string, NablaFactory*>::const_iterator j = m->begin(); j != m->end(); ++j ) {
        out << sep << ( *j ).first;
        sep = ", ";
    }
}

bool NablaFactory::has( const std::string& name ) {
    pthread_once( &once, init );

    eckit::AutoLock<eckit::Mutex> lock( local_mutex );

    static force_link static_linking;

    return ( m->find( name ) != m->end() );
}

const NablaImpl* NablaFactory::build( const Method& method, const eckit::Parametrisation& p ) {
    pthread_once( &once, init );

    eckit::AutoLock<eckit::Mutex> lock( local_mutex );

    static force_link static_linking;

    std::map<std::string, NablaFactory*>::const_iterator j = m->find( method.name() );

    Log::debug() << "Looking for NablaFactory [" << method.name() << "]" << '\n';

    if ( j == m->end() ) {
        Log::error() << "No NablaFactory for [" << method.name() << "]" << '\n';
        Log::error() << "NablaFactories are:" << '\n';
        for ( j = m->begin(); j != m->end(); ++j )
            Log::error() << "   " << ( *j ).first << '\n';
        throw eckit::SeriousBug( std::string( "No NablaFactory called " ) + method.name() );
    }

    return ( *j ).second->make( method, p );
}

extern "C" {

void atlas__Nabla__delete( Nabla::nabla_t* This ) {
    ATLAS_ERROR_HANDLING( ASSERT( This ); delete This; );
}

const Nabla::nabla_t* atlas__Nabla__create( const Method* method, const eckit::Parametrisation* params ) {
    const Nabla::nabla_t* nabla( 0 );
    ATLAS_ERROR_HANDLING( ASSERT( method ); ASSERT( params ); {
        Nabla n( *method, *params );
        nabla = n.get();
        nabla->attach();
    } nabla->detach(); );
    return nabla;
}

void atlas__Nabla__gradient( const Nabla::nabla_t* This, const field::FieldImpl* scalar, field::FieldImpl* grad ) {
    ATLAS_ERROR_HANDLING( ASSERT( This ); ASSERT( scalar ); ASSERT( grad ); Field fgrad( grad );
                          This->gradient( scalar, fgrad ); );
}

void atlas__Nabla__divergence( const Nabla::nabla_t* This, const field::FieldImpl* vector, field::FieldImpl* div ) {
    ATLAS_ERROR_HANDLING( ASSERT( This ); ASSERT( vector ); ASSERT( div ); Field fdiv( div );
                          This->divergence( vector, fdiv ); );
}

void atlas__Nabla__curl( const Nabla::nabla_t* This, const field::FieldImpl* vector, field::FieldImpl* curl ) {
    ATLAS_ERROR_HANDLING( ASSERT( This ); ASSERT( vector ); ASSERT( curl ); Field fcurl( curl );
                          This->curl( vector, fcurl ); );
}

void atlas__Nabla__laplacian( const Nabla::nabla_t* This, const field::FieldImpl* scalar,
                              field::FieldImpl* laplacian ) {
    ATLAS_ERROR_HANDLING( ASSERT( This ); ASSERT( scalar ); ASSERT( laplacian ); Field flaplacian( laplacian );
                          This->laplacian( scalar, flaplacian ); );
}
}

}  // namespace numerics
}  // namespace atlas
