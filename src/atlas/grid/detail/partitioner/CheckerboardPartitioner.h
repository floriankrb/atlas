#pragma once

#include <vector>

#include "atlas/grid/detail/partitioner/Partitioner.h"

namespace atlas {
namespace grid {
namespace detail {
namespace partitioner {

class CheckerboardPartitioner : public Partitioner {
public:
    CheckerboardPartitioner();

    CheckerboardPartitioner( int N );  // N is the number of parts (aka MPI tasks)

    CheckerboardPartitioner( int N, int nbands );
    CheckerboardPartitioner( int N, int nbands, bool checkerboard );

    // Node struct that holds the x and y indices (for global, it's longitude and
    // latitude in millidegrees (integers))
    // This structure is used in sorting algorithms, and uses less memory than
    // if x and y were in double precision.
    struct NodeInt {
        int x, y;
        int n;
    };

    virtual std::string type() const { return "checkerboard"; }

private:
    struct Checkerboard {
        size_t nbands;  // number of bands
        size_t nx, ny;  // grid dimensions
    };

    Checkerboard checkerboard( const Grid& ) const;

    // Doesn't matter if nodes[] is in degrees or radians, as a sorting
    // algorithm is used internally
    void partition( const Checkerboard& cb, int nb_nodes, NodeInt nodes[], int part[] ) const;

    virtual void partition( const Grid&, int part[] ) const;

    void check() const;

private:
    size_t nbands_;      // number of bands from configuration
    bool checkerboard_;  // exact (true) or approximate (false) checkerboard
};

}  // namespace partitioner
}  // namespace detail
}  // namespace grid
}  // namespace atlas
