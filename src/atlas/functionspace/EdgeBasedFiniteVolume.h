/*
 * (C) Copyright 1996-2015 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#ifndef atlas_functionspace_EdgeBasedFiniteVolume_h
#define atlas_functionspace_EdgeBasedFiniteVolume_h

#include <string>
#include "atlas/functionspace/NodesFunctionSpace.h"

namespace atlas { class Mesh; }
namespace atlas { class FunctionSpace; }

namespace atlas {
namespace functionspace {

class EdgeBasedFiniteVolume : public NodesFunctionSpace {

public:

  EdgeBasedFiniteVolume(Mesh &, const Halo & = Halo(1) );

  virtual std::string name() const { return "EdgeBasedFiniteVolume"; }

  const NodesFunctionSpace& nodes_fs() const { return *this; }
        NodesFunctionSpace& nodes_fs()       { return *this; }

private: // data

    atlas::FunctionSpace* edges_; // non-const because functionspace may modify mesh
};

} // namespace functionspace
} // namespace atlas

#endif // atlas_numerics_nabla_EdgeBasedFiniteVolume_h