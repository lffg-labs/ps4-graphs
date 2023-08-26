#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

class AdjacencyMatrixDigraph {
   private:
    std::vector<bool> _m;
    size_t _node_count;
    size_t _edge_count;

    inline auto set(size_t i, size_t j, bool val) -> void {
        _m.at(i * _node_count + j) = val;
    }

    inline auto get(size_t i, size_t j) const -> bool {
        return _m.at(i * _node_count + j);
    }

   public:
    AdjacencyMatrixDigraph(size_t node_count)
        : _m(node_count * node_count, false),
          _node_count(node_count),
          _edge_count(0) {
    }

    auto add(size_t orig, size_t dest) -> void {
        _edge_count++;
        set(orig, dest, true);
    }

    auto edge_count() const -> size_t {
        return _edge_count;
    }

    auto greatest_outdegree() const -> size_t {
        size_t max   = 0;
        size_t max_i = 0;
        for (size_t i = 0; i < _node_count; i++) {
            auto lo          = _m.begin() + i * _node_count;
            auto hi          = lo + _node_count;
            size_t out_count = 0;
            for (auto p = lo; p < hi; p++) {
                if (*p) out_count++;
            }
            if (out_count > max) {
                max   = out_count;
                max_i = i;
            }
        }
        return max_i;
    }

    auto successors(size_t node_i) const -> std::vector<size_t> {
        std::vector<size_t> v;
        size_t lo = node_i * _node_count;
        size_t hi = lo + _node_count;
        for (auto x = lo; x < hi; x++) {
            if (_m.at(x)) v.push_back(x % _node_count);
        }
        return v;
    }

    auto greatest_indegree() const -> size_t {
        size_t max   = 0;
        size_t max_i = 0;
        for (size_t i = 0; i < _node_count; i++) {
            auto lo          = _m.begin() + i * _node_count;
            auto hi          = lo + _node_count;
            size_t out_count = 0;
            for (auto p = lo; p < hi; p++) {
                if (*p) out_count++;
            }
            if (out_count > max) {
                max   = out_count;
                max_i = i;
            }
        }
        return max_i;
    }

    auto dbg() const -> void {
        std::cerr << "\t";
        for (size_t i = 0; i < _node_count; i++) {
            std::cerr << i << ":\t";
        }
        std::cerr << "\n";
        for (size_t i = 0; i < _node_count; i++) {
            auto lo = _m.begin() + i * _node_count;
            auto hi = lo + _node_count;
            std::cerr << i << ":\t";
            for (auto p = lo; p < hi; p++) {
                std::cerr << (*p ? 1 : 0) << "\t";
            }
            std::cerr << "\n";
        }
    }
};

auto main(int argc, char **argv) -> int {
    if (argc < 2) {
        std::cerr << "error: missing file name argument\n";
        return 1;
    }
    std::string_view file_name(argv[1]);

    std::ifstream input(file_name);
    if (!input.is_open()) {
        std::cerr << "error: file `" << file_name << "` couldn't be opened\n";
        return 1;
    }

    size_t node_count = 0;
    size_t edge_count = 0;
    input >> node_count >> edge_count;
    std::cerr << "dbg: got (node_count " << node_count << ") and (edge_count "
              << edge_count << ")\n";

    // +1 since we don't start in zero.
    AdjacencyMatrixDigraph graph(node_count + 1);

    size_t orig = 0;
    size_t dest = 0;
    while (input >> orig >> dest) {
        // std::cerr << "dbg: got (orig " << orig << ") (dest " << dest << ")"
        //           << std::endl;
        graph.add(orig, dest);
    }
    // sanity check
    auto actual_ec = graph.edge_count();
    if (edge_count != actual_ec) {
        std::cerr << "error: expected " << edge_count << " edges but found "
                  << actual_ec << "\n";
        return 1;
    }

    // graph.dbg();

    size_t go = graph.greatest_outdegree();
    std::cout << "node with greatest outdegree: " << go << ".\n";
    auto succ = graph.successors(go);
    std::cout << "  " << succ.size() << " successors are:\n  ";
    for (auto s : succ) {
        std::cout << s << ", ";
    }
    std::cout << "\n";

    return 0;
}
