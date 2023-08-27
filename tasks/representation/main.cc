#include <fstream>
#include <iostream>
#include <iterator>
#include <ostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef SANITY_CHECK
#define SANITY_CHECK_VECTOR_GROWTH(ident, description)                 \
    if (ident.size() == ident.capacity()) {                            \
        std::cerr << "sanity: will realloc (" << description << ")\n"; \
    }
#else
#define SANITY_CHECK_VECTOR_GROWTH(ident, description)
#endif

const uint32_t VERTEX_TO_EDGE_FACTOR = 10;

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
    EdgeBag(uint32_t edge_size_hint) {
        edges.reserve(edge_size_hint);
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

class ForwardStarDigraph {
    friend class NeighborsIterable<ForwardStarDigraph>;

   private:
    std::vector<uint32_t> ptrs;
    std::vector<uint32_t> edges;

   public:
    ForwardStarDigraph(uint32_t vertex_size_hint, EdgeBag &edge_bag) {
        size_t ptrs_size = vertex_size_hint + 2;
        // first element is unused; last element is used as sentinel
        ptrs.reserve(ptrs_size);
        ptrs.push_back(0);
        // first element is unused
        edges.reserve(edge_bag.edges.size() + 1);
        edges.push_back(0);

        edge_bag.sort_by_orig();  // <------------ each ptr is a orig
        uint32_t last_orig = 0;
        for (const Edge e : edge_bag) {
            if (last_orig != e.orig) {
                last_orig = e.orig;
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

    ForwardStarDigraph(EdgeBag &edge_bag)
        // Probably a high guess for the vertex size hint, but should avoid many
        // reallocations, which is better for performance.
        : ForwardStarDigraph(edge_bag.edges.size() / VERTEX_TO_EDGE_FACTOR,
                             edge_bag) {
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

    void dbg(std::ostream &sink) {
        sink << "orig_ptrs: ";
        for (auto v : ptrs) sink << v << " ";
        sink << "\n";
        sink << " arc_dest: ";
        for (auto v : edges) sink << v << " ";
        sink << "\n";
    }
};

class ReverseStarDigraph {
    friend class NeighborsIterable<ReverseStarDigraph>;

   private:
    std::vector<uint32_t> ptrs;
    std::vector<uint32_t> edges;

   public:
    ReverseStarDigraph(uint32_t vertex_size_hint, EdgeBag &edge_bag) {
        // first element is unused; last element is used as sentinel
        ptrs.reserve(vertex_size_hint + 2);
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
        SANITY_CHECK_VECTOR_GROWTH(ptrs, "ReverseStarDigraph::ptrs");
        ptrs.push_back(edges.size());
    }

    ReverseStarDigraph(EdgeBag &edge_bag)
        // Probably a high guess for the vertex size hint, but should avoid many
        // reallocations, which is better for performance.
        : ReverseStarDigraph(edge_bag.edges.size() / VERTEX_TO_EDGE_FACTOR,
                             edge_bag) {
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

    void dbg(std::ostream &sink) {
        sink << "dest_ptrs: ";
        for (auto v : ptrs) sink << v << " ";
        sink << "\n";
        sink << " arc_orig: ";
        for (auto v : edges) sink << v << " ";
        sink << "\n";
    }
};

struct vertex_degree {
    uint32_t vertex;
    uint32_t degree;
};

auto max_degree(ForwardStarDigraph &g) -> vertex_degree {
    uint32_t max_outdeg = 0;
    uint32_t max_v      = 0;
    for (const uint32_t v : g.vertexes()) {
        const uint32_t outdeg = g.outdegree(v);
        if (outdeg > max_outdeg) {
            max_v      = v;
            max_outdeg = outdeg;
        }
    }
    return {.vertex = max_v, .degree = max_outdeg};
}

auto max_degree(ReverseStarDigraph &g) -> vertex_degree {
    uint32_t max_indeg = 0;
    uint32_t max_v     = 0;
    for (const uint32_t v : g.vertexes()) {
        const uint32_t indeg = g.indegree(v);
        if (indeg > max_indeg) {
            max_v     = v;
            max_indeg = indeg;
        }
    }
    return {.vertex = max_v, .degree = max_indeg};
}

auto dot(ForwardStarDigraph &g, std::ostream &sink) {
    sink << "digraph G {\n";
    for (const uint32_t orig : g.vertexes()) {
        for (const uint32_t dest : g.successors(orig)) {
            sink << "    " << orig << " -> " << dest << "\n";
        }
        sink << "\n";
    }
    sink << "}\n";
}

auto dot(ReverseStarDigraph &g, std::ostream &sink) {
    sink << "digraph G {\n";
    for (const uint32_t dest : g.vertexes()) {
        for (const uint32_t orig : g.predecessors(dest)) {
            sink << "    " << orig << " -> " << dest << "\n";
        }
        sink << "\n";
    }
    sink << "}\n";
}

auto main(int argc, char **argv) -> int {
    if (argc < 2) {
        std::cerr << "error: missing file name argument\n";
        return 1;
    }
    const std::string_view file_name(argv[1]);

    bool debug_mode = false;
    bool dot_mode   = false;

    int curr_arg_i = 2;
    while (curr_arg_i < argc) {
        const std::string_view arg(argv[curr_arg_i++]);
        if (arg == "--debug") {
            debug_mode = true;
            std::cerr << "(debug mode is on)\n";
        } else if (arg == "--dot") {
            dot_mode = true;
            std::cerr << "(dot mode is on)\n";
            continue;
        }
    }

#ifdef SANITY_CHECK
    std::cerr << "(sanity check mode is on)\n";
#endif

    std::ifstream input(file_name);
    if (!input.is_open()) {
        std::cerr << "error: failed to open file `" << file_name << "`\n";
        return 1;
    }

    uint32_t vertex_count = 0;
    uint32_t edge_count   = 0;
    input >> vertex_count >> edge_count;
    if (debug_mode)
        std::cerr << "got (vertex_count " << vertex_count
                  << ") and (edge_count " << edge_count << ")\n";

    EdgeBag edge_bag(edge_count);
    uint32_t e_orig = 0;
    uint32_t e_dest = 0;
    while (input >> e_orig >> e_dest) {
        edge_bag.add(Edge(e_orig, e_dest));
    }
    // sanity check
    if (edge_bag.size() != edge_count) {
        std::cerr << "invalid edge count, expected " << edge_count << ", got "
                  << edge_bag.size() << "\n";
        return 1;
    }

    std::cout << "----------------\n";
    // outdegree
    {
        ForwardStarDigraph g(vertex_count, edge_bag);
        if (debug_mode) g.dbg(std::cerr);
        if (dot_mode) dot(g, std::cerr);

        // get first vertex with greatest outdegree
        auto max_out = max_degree(g);
        std::cout << "maximum outdegree is (" << max_out.degree
                  << "), first for vertex (" << max_out.vertex << ")\n";
        std::cout << "its successors are:\n";
        for (const uint32_t v : g.successors(max_out.vertex)) {
            std::cout << v << ", ";
        }
        std::cout << "\n";
    }
    std::cout << "----------------\n";
    // indegree
    {
        ReverseStarDigraph g(vertex_count, edge_bag);
        if (debug_mode) g.dbg(std::cerr);
        if (dot_mode) dot(g, std::cerr);

        // get first vertex with greatest outdegree
        auto max_in = max_degree(g);
        std::cout << "maximum indegree is (" << max_in.degree
                  << "), first for vertex (" << max_in.vertex << ")\n";
        std::cout << "its predecessors are:\n";
        for (const uint32_t v : g.predecessors(max_in.vertex)) {
            std::cout << v << ", ";
        }
        std::cout << "\n";
    }
    std::cout << "----------------\n";

    return 0;
}
