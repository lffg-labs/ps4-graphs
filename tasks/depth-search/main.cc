#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stack>
#include <string>

// XX: Make a library.
#include "../representation-star/lib.cc"

using NodeId = uint32_t;

enum class digraph_edge_classification : uint8_t {
    tree,
    back,
    forward,
    cross,
};

auto operator<<(std::ostream &sink, const digraph_edge_classification &ec)
    -> std::ostream & {
    switch (ec) {
        case digraph_edge_classification::tree:
            sink << "tree";
            break;
        case digraph_edge_classification::back:
            sink << "back";
            break;
        case digraph_edge_classification::forward:
            sink << "forward";
            break;
        case digraph_edge_classification::cross:
            sink << "cross";
            break;
    }
    return sink;
}

class dfs_entry {
   public:
    size_t discovery_t = 0;
    size_t term_t      = 0;
    NodeId parent      = 0;
};

class dfs_result {
   private:
    std::vector<dfs_entry> ctl;

   public:
    dfs_result(size_t size_hint) : ctl(size_hint) {
    }

    auto begin() -> std::vector<dfs_entry>::iterator {
        return ctl.begin();
    }

    auto end() -> std::vector<dfs_entry>::iterator {
        return ctl.end();
    }

    auto at_v(NodeId i) -> dfs_entry & {
        // Since we're using the forward star representation to implement this
        // algorithm, there is a guarantee that indexes always start at 1.
        return ctl.at(i - 1);
    }

    auto classify_edge(NodeId orig, NodeId dest)
        -> digraph_edge_classification {
        auto &orig_e = at_v(orig);
        auto &dest_e = at_v(dest);

        if (orig_e.discovery_t < dest_e.discovery_t) {
            if (dest_e.parent == orig) {
                return digraph_edge_classification::tree;
            }
            return digraph_edge_classification::forward;
        }

        if (dest_e.term_t < orig_e.discovery_t) {
            return digraph_edge_classification::forward;
        }
        return digraph_edge_classification::back;
    }
};

using EdgeVisitor   = std::function<void(NodeId, NodeId)>;
using VertexVisitor = std::function<void(NodeId)>;

VertexVisitor NOOP_VERTEX_VISITOR = [](NodeId v) { (void)v; };
EdgeVisitor NOOP_EDGE_VISITOR     = [](NodeId o, NodeId d) {
    (void)o;
    (void)d;
};

class dfs {
   public:
    EdgeVisitor tree_edge_visitor    = NOOP_EDGE_VISITOR;
    EdgeVisitor back_edge_visitor    = NOOP_EDGE_VISITOR;
    EdgeVisitor forward_edge_visitor = NOOP_EDGE_VISITOR;
    EdgeVisitor cross_edge_visitor   = NOOP_EDGE_VISITOR;
    VertexVisitor vertex_visitor     = NOOP_VERTEX_VISITOR;

    dfs() = default;

    auto execute(ForwardStarDigraph &g) -> dfs_result {
        dfs_result res(g.vertexes_count());
        uint64_t time = 0;

        // Stack we use for each call to `dfs_v`.
        std::stack<NodeId> st;

        auto it = g.vertexes();
        for (const NodeId v : it) {
            // Skip if we find a vertex which is not yet discovered.
            if (res.at_v(v).discovery_t != 0U) continue;

#ifdef SANITY_CHECK
            // Sanity check: assert that the stack is empty.
            if (!st.empty()) throw std::logic_error("stack not empty");
#endif

            dfs_v(g, st, time, res, v);
        }

        return res;
    }

   private:
    void dfs_v(ForwardStarDigraph &g, std::stack<NodeId> &st, uint64_t &time,
               dfs_result &res, NodeId starting_vertex) const {
        st.push(starting_vertex);

    st_loop:
        while (!st.empty()) {
            // Notice that we don't yet remove the vertex from the stack; we do
            // so only after all of its children are processed.
            const NodeId v     = st.top();
            dfs_entry &v_entry = res.at_v(v);

            // Registers the current vertex as discovered.
            auto &v_dt = res.at_v(v).discovery_t;
            if (v_dt == 0) {
                res.at_v(v).discovery_t = ++time;
                vertex_visitor(v);
            }

            for (const NodeId succ_v : g.successors(v)) {
                dfs_entry &succ_entry = res.at_v(succ_v);

                // We have just discovered `succ_v`.
                if (succ_entry.discovery_t == 0) {
                    tree_edge_visitor(v, succ_v);
                    succ_entry.parent = v;
                    st.push(succ_v);

                    // XX: This is sub-optimal since, contrary to recursive
                    // calls, which would resume AFTER the call, a naive
                    // iterative implementation, such as this one, would have to
                    // re-scan ALL of the **already** visited vertexes to resume
                    // where it had stopped to recurse.
                    //
                    // Maybe a way to fix this problem is to save in the stack,
                    // next to the NodeId, the index of the current iterator
                    // pointer to resume (instead of always starting from the
                    // beginning, as `g.successors(v)` does).
                    goto st_loop;
                } else {
                    // The dest `succ_v` is ancestral and isn't yet finished.
                    if (succ_entry.term_t == 0) {
                        back_edge_visitor(v, succ_v);
                    }
                    // The origin `v` is discovered before the dest `succ_v`.
                    else if (v_entry.discovery_t < succ_entry.discovery_t) {
                        forward_edge_visitor(v, succ_v);
                    }
                    // The origin `v` is discovered after the dest `succ_v`.
                    else {
                        cross_edge_visitor(v, succ_v);
                    }
                }
            }

            st.pop();
            v_entry.term_t = ++time;
        }
    }
};

auto classify_outgoing_edges(std::ostream &sink, ForwardStarDigraph &g,
                             dfs_result &res, NodeId v) {
    sink << "classification of the outgoing edges of vertex (" << v << ")\n";
    for (const NodeId dest : g.successors(v)) {
        sink << "  (" << v << " -> " << dest << ") is a "
             << res.classify_edge(v, dest) << " edge\n";
    }
}

auto main(int argc, char **argv) -> int {
    const int POSITIONAL_ARG_LEN = 3;
    if (argc < POSITIONAL_ARG_LEN) {
        std::cerr << "error: missing file name argument and vertex number to "
                     "classify edges from\n";
        std::cerr << "usage is: ./prog [file_name] [vertex]\n";
        std::cerr << "  (pass \"ALL\" in the [vertex] argument to classify all "
                     "vertexes' outgoing edges.)\n";
        return 1;
    }
    const std::string_view file_name(argv[1]);

    const std::string vertex_to_classify_opt = std::string(argv[2]);
    const int64_t vertex_to_classify =
        vertex_to_classify_opt == "ALL"
            ? -1L
            : static_cast<NodeId>(std::stoul(vertex_to_classify_opt));

    bool debug_mode = false;
    bool dot_mode   = false;

    int curr_arg_i = POSITIONAL_ARG_LEN;
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

    ForwardStarDigraph g(vertex_count, edge_bag);
    if (debug_mode) g.dbg(std::cerr);
    if (dot_mode) g.dot(std::cerr);

    dfs dfs_executor;
    dfs_executor.tree_edge_visitor = [](NodeId orig, NodeId dest) {
        std::cout << "  (" << orig << " -> " << dest << ")\n";
    };

    std::cout << "tree edges:\n";
    dfs_result dfs_res = dfs_executor.execute(g);
    std::cout << "------------------------------------\n";

    // We could also have used the visitor APIs to implement this
    // classification. However, this seemed more appropriate for this use-case.
    if (vertex_to_classify == -1) /* all */ {
        for (const NodeId v : g.vertexes()) {
            classify_outgoing_edges(std::cout, g, dfs_res, v);
        }
    } else {
        classify_outgoing_edges(std::cout, g, dfs_res,
                                static_cast<NodeId>(vertex_to_classify));
    }

    return 0;
}
