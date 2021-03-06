#include <vector>

#include "atlas/grid.h"

namespace atlas {
namespace grid {
namespace detail {
namespace grid {

StructuredGrid::grid_t* reduced_gaussian( const std::vector<long>& nx );
StructuredGrid::grid_t* reduced_gaussian( const std::vector<long>& nx, const Domain& domain );

}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
