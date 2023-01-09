#ifndef AUTOMATA__HH_
#define AUTOMATA__HH_
//=====================================================================
#include <cassert>
#include <spot/twa/twa.hh>
#include <spot/twa/bdddict.hh>
#include <spot/tl/defaultenv.hh>
#include <map>
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
#include "types.c"
#include <cmath>
#include <bitset>
#include <cstdint> 
//=====================================================================
class Automata {

private:

//=====================================================================
//=====================================================================
    int                     max_height = 0;
    int                     max_node = 0;
    //=================================================================
    int****                 h_change;
    //=================================================================
    Vec<Int_SET>            heights;
    Map<int,int>            h_map;
    Map<int,int>            n_map;
    //=================================================================
    Vec<bdd>                propositions;
    //=================================================================
    Map<Int_pair,bdd>       bdd_encoding_global;
    Map<Int_pair,bdd>       bdd_encoding_ipath;
    Map<Int_pair,bdd>       bdd_encoding_trace;
    //=================================================================
    spot::bdd_dict_ptr      dict;
    spot::twa_graph_ptr     aut_ipath;
    spot::twa_graph_ptr     aut_trace;
    //=================================================================
    int                     s_init_ip = 0;
    int                     s_init_tr = 0;
    int64_t                 ap_size = 0;
    //=================================================================
    void                    register_AP(void);
    void                    generate_atomic_BDD(void);
    void                    add_transitions(void);
    void                    init_automata(void);
    void                    generate_BDD_combinations(void);
    int64_t                 encode_relation(int** R);
    
//=====================================================================
//=====================================================================

public:

//=====================================================================
//=====================================================================
    Automata(int n);
    ~Automata();
    //=================================================================
    void                    add_node(int n);
    void                    add_height(int n,int h);
    void                    add_edge(int src_, int tgt_);
    void                    add_decr(int src_, int h_src, int tgt_, int h_tgt);
    void                    add_stay(int src_, int h_src, int tgt_, int h_tgt);
    //=================================================================
    bool                    check_soundness(void);
//=====================================================================
//=====================================================================
};
#endif 