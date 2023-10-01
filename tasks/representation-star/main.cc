#include <stdint.h>

#include <fstream>
#include <iostream>
#include <string>

#include "./lib.cc"

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
        if (dot_mode) g.dot(std::cerr);

        // get first vertex with greatest outdegree
        auto max_out = g.max_outdegree();
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
        if (dot_mode) g.dot(std::cerr);

        // get first vertex with greatest indegree
        auto max_in = g.max_indegree();
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
