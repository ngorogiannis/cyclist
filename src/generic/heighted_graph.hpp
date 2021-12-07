#ifndef HEIGHTED_GRAPH_HH_
#define HEIGHTED_GRAPH_HH_

#include<chrono>
#include <iostream>
#include <fstream>
#include <set>
#include <utility>
#include "sloped_relation.hpp"
#include "types.c"
#include <vector>
#include <thread>

#define DURATION std::chrono::duration<double, std::milli>

class Heighted_graph {
private:
    int                   max_nodes;
    int                   number_of_edges;
    Map<int, int>         node_idxs;
    std::vector<Int_SET*> HeightsOf;
    Sloped_relation***    h_change_;
    Relation_LIST_LIST*** Ccl;
    Relation_LIST*        rejected;

    int                   ccl_initial_size;
    int                   ccl_size;
    int                   ccl_replacements;
    int                   ccl_rejections;
    int                   compositions;
    int                   comparisons;
    int                   loop_checks;
    int                   checked_size_sum;
    DURATION              compose_time;
    DURATION              compare_time;
    DURATION              loop_check_time;

    bool check_Ccl(int opts);

public:

    // Option flags
    static const int FAIL_FAST;
    static const int USE_SCC_CHECK;
    static const int USE_IDEMPOTENCE;
    static const int USE_MINIMALITY;

    static int parse_flags(const std::string flags);

    Heighted_graph(int max_nodes);
    ~Heighted_graph(void);

    // Methods for constructing the height graph
    void add_node(int n);
    void add_height(int n, int h);
    void add_edge(int source, int sink);
    void add_hchange(int source, int source_h, int sink, int sink_h, slope s);
    void add_stay(int source_node, int source_h, int sink_node, int sink_h);
    void add_decrease(int source_node, int source_h, int sink_node, int sink_h);
    int num_nodes(void);
    int num_edges(void);
    bool check_soundness(int opts);
    void print_Ccl(void);
    void print_statistics(void);
};

#endif