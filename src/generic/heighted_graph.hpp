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
#include <mutex>
#include <condition_variable>

//======================================================
#include <cassert>
#include <memory>
#include <spot/twa/twa.hh>
#include <spot/twa/bdddict.hh>
#include <spot/tl/defaultenv.hh>

#include <spot/twaalgos/contains.hh>
#include <spot/twaalgos/determinize.hh>
#include <spot/twaalgos/dualize.hh>
#include <spot/twaalgos/totgba.hh>
#include <spot/twaalgos/copy.hh>
#include <spot/twaalgos/stutter.hh>
#include <spot/twa/twaproduct.hh>
#include <spot/twaalgos/gtec/gtec.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/remfin.hh>

#include <spot/twaalgos/hoa.hh>
#include <spot/twa/twagraph.hh>

#include <iostream>
#include <sstream>
//======================================================



#define DURATION std::milli

class Heighted_graph {
private:
    int                         max_nodes;
    int                         number_of_edges;
    Map<int, int>               node_idxs;
    std::vector<Map<int, int>*> height_idxs;
    std::vector<Int_SET*>       HeightsOf;
    Sloped_relation***          h_change_;
    Relation_LIST***            Ccl;
    Relation_LIST*              rejected;
    std::vector<Int_pair>*      edges;

    int max_height = -1;
    int max_height_set_size = 0;


    int flags;



#ifdef LOG_STATS
    int                     ccl_initial_size;
    int                     ccl_size;
    int                     ccl_iterations;
    int                     ccl_replacements;
    int                     ccl_rejections;
    int                     compositions;
    int                     comparisons;
    int                     loop_checks;
    int                     checked_size_sum;
    int                     total_size_sum;
    std::chrono::duration<double, DURATION> compose_time;
    std::chrono::duration<double, DURATION> compare_time;
    std::chrono::duration<double, DURATION> need_to_add_compute_time;
    std::chrono::duration<double, DURATION> insertion_time;
    std::chrono::duration<double, DURATION> loop_check_time;
#endif

    bool check_self_loop(Sloped_relation *R, int node, int opts);
    bool set_based_check(SD_decrease_type SD_DEC_TYPE);
    bool check_Ccl(int opts);

public:

    
    std::mutex* m;
    std::condition_variable* cv;
    bool* mainReady;
    bool* SD_done;
    bool* XSD_done;
    bool* REL_done;
    std::vector<int>* res;
    


    // Option flags
    static const int FAIL_FAST;
    static const int USE_SCC_CHECK;
    static const int USE_IDEMPOTENCE;
    static const int USE_MINIMALITY;
    static const int COMPUTE_FULL_CCL;
    static const int USE_SD;
    static const int USE_XSD;

    static int parse_flags(const std::string flags);
    static void print_flags(int flags);

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
    bool relational_check(int opts);
    bool sd_check();
    bool xsd_check();
    void print_Ccl(void);
    void print_statistics(void);



    //=========== THOR ==========


    

    bool thors_hammer(int opts,Heighted_graph*,Heighted_graph*);
    // void thor_sd(void);
    // void thor_xsd(void);
    // void thor_rel(int opts);
    

};

#endif