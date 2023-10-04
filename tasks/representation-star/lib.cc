#pragma once

#include <stdint.h>

#include <iterator>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

// XX: I should probably just use an array (perhaps wrapped by an owned_ptr) to
// avoid any kind of size checks. It won't be a small change since I'd have to
// also ditch vector's iterators in favor of my own implementation.
#ifdef SANITY_CHECK
#define SANITY_CHECK_VECTOR_GROWTH(ident, description)                 \
    if (ident.size() == ident.capacity()) {                            \
        std::cerr << "sanity: will realloc (" << description << ")\n"; \
    }
#else
#define SANITY_CHECK_VECTOR_GROWTH(ident, description)
#endif

class Edge {
   public:
    uint32_t orig;
    uint32_t dest;

    Edge(uint32_t orig, uint32_t dest) : orig(orig), dest(dest) {
        if (orig == 0U || dest == 0U) {
            throw std::invalid_argument("vertex 0 is not valid");
        }
    }

    [[nodiscard]] auto lt_by_orig(const Edge &other) const -> bool {
        return (orig != other.orig) ? orig < other.orig : dest < other.dest;
    }

    [[nodiscard]] auto lt_by_dest(const Edge &other) const -> bool {
        return (dest != other.dest) ? dest < other.dest : orig < other.orig;
    }
};

class EdgeBag {
    friend class ForwardStarDigraph;
    friend class ReverseStarDigraph;

   private:
    std::vector<Edge> edges;

    void sort_by_orig() {
        std::sort(edges.begin(), edges.end(), [](auto &left, auto &right) {
            return left.lt_by_orig(right);
        });
    }

    void sort_by_dest() {
        std::sort(edges.begin(), edges.end(), [](auto &left, auto &right) {
            return left.lt_by_dest(right);
        });
    }

   public:
    EdgeBag(uint32_t size) {
        edges.reserve(size);
    }

    void add(Edge e) {
        SANITY_CHECK_VECTOR_GROWTH(edges, "edges");
        edges.push_back(e);
    }

    [[nodiscard]] auto size() const -> size_t {
        return edges.size();
    }

    [[nodiscard]] auto begin() const {
        return edges.begin();
    }

    [[nodiscard]] auto end() const {
        return edges.end();
    }
};

template <typename G>
class NeighborsIterable {
    friend class ForwardStarDigraph;
    friend class ReverseStarDigraph;

   private:
    const G &g;
    uint32_t _start;
    uint32_t _end;

    NeighborsIterable(G &g, uint32_t start, uint32_t end)
        : g(g), _start(start), _end(end) {
    }

   public:
    auto begin() {
        return g.edges.begin() + _start;
    }

    auto end() {
        return g.edges.begin() + _end;
    }
};

struct vertex_degree {
    uint32_t vertex;
    uint32_t degree;
};

class ForwardStarDigraph {
    friend class NeighborsIterable<ForwardStarDigraph>;

   private:
    std::vector<uint32_t> ptrs;
    std::vector<uint32_t> edges;

   public:
    ForwardStarDigraph(uint32_t vertex_count, EdgeBag &edge_bag) {
        const size_t ptrs_size = vertex_count + 2;
        // first element is unused; last element is used as sentinel
        ptrs.reserve(ptrs_size);
        ptrs.push_back(0);
        // first element is unused
        edges.reserve(edge_bag.edges.size() + 1);
        edges.push_back(0);

        edge_bag.sort_by_orig();  // <------------ each ptr is a orig
        uint32_t last_orig = 0;
        for (const Edge e : edge_bag) {
            // insert the new orig ptr while also avoiding holes due to vertexes
            // without any successors
            while (last_orig < e.orig) {
                last_orig++;
                ptrs.push_back(edges.size());
            }
            SANITY_CHECK_VECTOR_GROWTH(edges, "ForwardStarDigraph::edges");
            edges.push_back(e.dest);
        }
        while (ptrs.size() < ptrs_size) {
            SANITY_CHECK_VECTOR_GROWTH(ptrs, "ForwardStarDigraph::ptrs");
            ptrs.push_back(edges.size());
        }
    }

    // Returns the number of vertexes in the graph.
    auto vertexes_count() {
        // There are two extra elements (0, the first element and a sentinel at
        // the end).
        return ptrs.size() - 2;
    }

    // Returns an iterable over all the vertexes.
    auto vertexes() {
        return std::views::iota(1U, ptrs.size() - 1);
    }

    // Returns an iterable over the sucessor vertexes for the given vertex.
    auto successors(uint32_t vertex) -> NeighborsIterable<ForwardStarDigraph> {
        return NeighborsIterable(*this, ptrs.at(vertex), ptrs.at(vertex + 1));
    }

    // Returns the outdegree for the given vertex.
    auto outdegree(uint32_t vertex) -> uint32_t {
        auto it = successors(vertex);
        return std::distance(it.begin(), it.end());
    }

    auto max_outdegree() -> vertex_degree {
        uint32_t max_outdeg = 0;
        uint32_t max_v      = 0;
        for (const uint32_t v : vertexes()) {
            const uint32_t outdeg = outdegree(v);
            if (outdeg > max_outdeg) {
                max_v      = v;
                max_outdeg = outdeg;
            }
        }
        return {.vertex = max_v, .degree = max_outdeg};
    }

    void dbg(std::ostream &sink) {
        sink << "orig_ptrs: ";
        for (auto v : ptrs) sink << v << " ";
        sink << "\n";
        sink << " arc_dest: ";
        for (auto v : edges) sink << v << " ";
        sink << "\n";
    }

    auto dot(std::ostream &sink) {
        sink << "digraph G {\n";
        for (const uint32_t orig : vertexes()) {
            for (const uint32_t dest : successors(orig)) {
                sink << "    " << orig << " -> " << dest << "\n";
            }
            sink << "\n";
        }
        sink << "}\n";
    }
};

class ReverseStarDigraph {
    friend class NeighborsIterable<ReverseStarDigraph>;

   private:
    std::vector<uint32_t> ptrs;
    std::vector<uint32_t> edges;

   public:
    ReverseStarDigraph(uint32_t vertex_count, EdgeBag &edge_bag) {
        const size_t ptrs_size = vertex_count + 2;
        // first element is unused; last element is used as sentinel
        ptrs.reserve(ptrs_size);
        ptrs.push_back(0);
        // first element is unused
        edges.reserve(edge_bag.edges.size() + 1);
        edges.push_back(0);

        edge_bag.sort_by_dest();  // <------------ each ptr is a dest
        uint32_t last_dest = 0;
        for (const Edge e : edge_bag) {
            // insert the new dest ptr while also avoiding holes due to vertexes
            // without any predecessors
            while (last_dest < e.dest) {
                last_dest++;
                ptrs.push_back(edges.size());
            }
            SANITY_CHECK_VECTOR_GROWTH(edges, "ReverseStarDigraph::edges");
            edges.push_back(e.orig);
        }
        while (ptrs.size() < ptrs_size) {
            SANITY_CHECK_VECTOR_GROWTH(ptrs, "ReverseStarDigraph::ptrs");
            ptrs.push_back(edges.size());
        }
    }

    // Returns an iterable over all the vertexes.
    auto vertexes() {
        return std::views::iota(1U, ptrs.size() - 1);
    }

    // Returns an iterable over the predecessor vertexes for the given vertex.
    auto predecessors(uint32_t vertex)
        -> NeighborsIterable<ReverseStarDigraph> {
        return NeighborsIterable(*this, ptrs.at(vertex), ptrs.at(vertex + 1));
    }

    // Returns the indegree for the given vertex.
    auto indegree(uint32_t vertex) -> uint32_t {
        auto it = predecessors(vertex);
        return std::distance(it.begin(), it.end());
    }

    auto max_indegree() -> vertex_degree {
        uint32_t max_indeg = 0;
        uint32_t max_v     = 0;
        for (const uint32_t v : vertexes()) {
            const uint32_t indeg = indegree(v);
            if (indeg > max_indeg) {
                max_v     = v;
                max_indeg = indeg;
            }
        }
        return {.vertex = max_v, .degree = max_indeg};
    }

    void dbg(std::ostream &sink) {
        sink << "dest_ptrs: ";
        for (auto v : ptrs) sink << v << " ";
        sink << "\n";
        sink << " arc_orig: ";
        for (auto v : edges) sink << v << " ";
        sink << "\n";
    }

    auto dot(std::ostream &sink) {
        sink << "digraph G {\n";
        for (const uint32_t dest : vertexes()) {
            for (const uint32_t orig : predecessors(dest)) {
                sink << "    " << orig << " -> " << dest << "\n";
            }
            sink << "\n";
        }
        sink << "}\n";
    }
};
