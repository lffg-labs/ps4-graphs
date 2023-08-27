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
        // first element is unused; last element is used as sentinel
        ptrs.reserve(vertex_size_hint + 2);
        ptrs.push_back(0);
        // first element is unused
        edges.reserve(edge_bag.edges.size() + 1);
        edges.push_back(0);

        edge_bag.sort_by_orig();
        uint32_t last_orig = 0;
        for (const Edge e : edge_bag) {
            if (last_orig != e.orig) {
                last_orig = e.orig;
                ptrs.push_back(edges.size());
            }
            edges.push_back(e.dest);
        }
        // last `ptrs`, the sentinel
        ptrs.push_back(edges.size());
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

    // Returns an iterable over the sucessor vertex for the given vertex.
    auto successors(uint32_t vertex) -> NeighborsIterable<ForwardStarDigraph> {
        return NeighborsIterable(*this, ptrs.at(vertex), ptrs.at(vertex + 1));
    }

    // Returns the outgoing degree for the given vertex.
    auto outgoing_deg(uint32_t vertex) -> uint32_t {
        NeighborsIterable<ForwardStarDigraph> it = successors(vertex);
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

struct vertex_degree {
    uint32_t vertex;
    uint32_t degree;
};

auto max_vertex_degree(ForwardStarDigraph &g) -> vertex_degree {
    uint32_t max_out_deg   = 0;
    uint32_t max_out_deg_v = 0;
    for (const uint32_t v : g.vertexes()) {
        const uint32_t out_deg = g.outgoing_deg(v);
        if (out_deg > max_out_deg) {
            max_out_deg_v = v;
            max_out_deg   = out_deg;
        }
    }
    return {.vertex = max_out_deg_v, .degree = max_out_deg};
}

auto main(int argc, char **argv) -> int {
    if (argc < 2) {
        std::cerr << "error: missing file name argument\n";
        return 1;
    }
    const std::string_view file_name(argv[1]);

    const bool debug = argc == 3 && std::string_view(argv[2]) == "--debug";
    if (debug) std::cerr << "(debug mode is on)\n";

    std::ifstream input(file_name);
    if (!input.is_open()) {
        std::cerr << "error: failed to open file `" << file_name << "`\n";
        return 1;
    }

    uint32_t vertex_count = 0;
    uint32_t edge_count   = 0;
    input >> vertex_count >> edge_count;
    if (debug)
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

    ForwardStarDigraph g(edge_count, edge_bag);
    if (debug) g.dbg(std::cerr);

    // get first vertex with greatest outgoing degree
    auto max_outgoing = max_vertex_degree(g);
    std::cout << "maximum outgoing degree is (" << max_outgoing.degree
              << "), first for vertex (" << max_outgoing.vertex << ")\n";
    std::cout << "its successors are:\n";
    for (const uint32_t v : g.successors(max_outgoing.vertex)) {
        std::cout << v << ", ";
    }
    std::cout << "\n";

    return 0;
}
