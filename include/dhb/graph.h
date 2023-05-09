#pragma once

#include <limits>
#include <random>
#include <tuple>
#include <vector>

namespace dhb {

using Weight = double;
using Degree = unsigned int;
#ifdef DHB_64BIT_IDS
using Vertex = uint64_t;
using EdgeID = uint64_t;
#else
using Vertex = uint32_t;
using EdgeID = uint32_t;
#endif

struct EdgeData {
    Weight weight;
    EdgeID id;
};

struct Target {
    Vertex vertex;
    EdgeData data;
};

struct Edge {
    Edge() = default;

    Edge(Vertex _source, Target _target) : source(_source), target(_target) {}

    Vertex source;
    Target target;
};

using Edges = std::vector<Edge>;
using Targets = std::vector<Target>;

Vertex invalidVertex();
Weight invalidWeight();
EdgeID invalidEdgeID();
Edge invalidEdge();
Target invalidTarget();

namespace graph {
inline Vertex vertex_count(Edges const& edges) {
    // Determine n as the maximal node ID.
    Vertex n = 0;
    for (Edge const& edge : edges) {
        Vertex const vertex = std::max(edge.source, edge.target.vertex);
        n = std::max(n, vertex);
    }

    n += 1;
    return n;
}
inline Edge into(Vertex source, Target const&);
inline Target into(Edge const&);
} // namespace graph

inline Vertex invalidVertex() { return std::numeric_limits<Vertex>::max(); }
inline Weight invalidWeight() { return std::numeric_limits<float>::infinity(); }
inline EdgeID invalidEdgeID() { return std::numeric_limits<EdgeID>::max(); }

inline Edge invalidEdge() { return Edge(invalidVertex(), invalidTarget()); }

inline Target invalidTarget() { return {invalidVertex(), {invalidWeight(), invalidEdgeID()}}; }

} // namespace dhb
