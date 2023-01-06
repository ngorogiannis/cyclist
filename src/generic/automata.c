#include "automata.hpp"
//=================================================================  
Automata::Automata(int max_node_) : max_node(max_node_) {

    heights = Vec<Int_SET>( max_node+1 );
    h_change = (int****)malloc( sizeof(int***) * (max_node+1) );

    for( int i = 0 ; i < (max_node+1) ; i++ ){
        h_change[i] = (int***)malloc(sizeof(int**) * (max_node+1));
        for( int j = 0 ; j < max_node+1 ; j++ ) h_change[i][j] = 0;
    }
};
//=================================================================
 Automata::~Automata(){
        for( int i = 0 ; i < max_node+1 ; i++){
            for( int j = 0 ; j < max_node+1 ; j++){
                if( h_change[i][j] ){
                    for( int k = 0 ; k < max_height+1 ; k++){
                        if( h_change[i][j][k] ) free(h_change[i][j][k]);
                    }
                    free(h_change[i][j]);
                }
            }
            free(h_change[i]);
        }
        free(h_change);
    };
//=================================================================
void Automata::add_node(int n){

    int n_idx = n_map.size();
    auto found2 = n_map.find(n);

    if( found2 == n_map.end() ){
        n_map.insert(Int_pair(n,n_idx));
    }
}
//=================================================================
void Automata::add_height(int n,int h){

    int h_idx = h_map.size();
    int n_idx = n_map.size();

    auto found = h_map.find(h);
    if( found == h_map.end() ){
        h_map.insert(Int_pair(h,h_idx));
        if( h_idx > max_height ) max_height = h_idx;
    }

    auto found2 = n_map.find(n);
    if( found2 == n_map.end() ){
        n_map.insert(Int_pair(n,n_idx));
    }

    heights[n_map.at(n)].insert(h_map.at(h));
}
//=================================================================
 void Automata::add_edge(int src_, int tgt_){
    int src = n_map.at(src_);
    int tgt = n_map.at(tgt_);
    if( h_change[src][tgt] == 0){
        h_change[src][tgt] = (int**)malloc(sizeof(int*) * (max_height+1) );
        for( int i = 0 ; i < max_height+1 ; i++ ){
            h_change[src][tgt][i] = (int*)malloc(sizeof(int) * (max_height+1));
            for( int j = 0 ; j < max_height+1 ; j++ ){
                h_change[src][tgt][i][j] = Undef;
            }
        }
    }
}
//=================================================================
void Automata::add_decr(int src_, int h_src, int tgt_, int h_tgt){
    int src = n_map.at(src_);
    int tgt = n_map.at(tgt_);
    if( h_change[src][tgt] == 0 ) return;
    int h1 = h_map.at(h_src);
    int h2 = h_map.at(h_tgt);
    h_change[src][tgt][h1][h2] = Downward;
}
//=================================================================
void Automata::add_stay(int src_, int h_src, int tgt_, int h_tgt){
    int src = n_map.at(src_);
    int tgt = n_map.at(tgt_);
    if( h_change[src][tgt] == 0 ) return;
    int h1 = h_map.at(h_src);
    int h2 = h_map.at(h_tgt);
    if( h_change[src][tgt][h1][h2] < Stay) h_change[src][tgt][h1][h2] = Stay;
}
//=================================================================
int64_t Automata::encode_relation(int** R){

    size_t code = 0;
    for( int h1 = 0 ; h1 < max_height ; h1++ ){
        for( int h2 = 0 ; h2 < max_height ; h2++ ){
            code = code * 3;
            if( R[h1][h2] == Stay) code = code + 1;
            else if( R[h1][h2] == Downward ) code = code + 2;
        }
    }
    return code;
}
//=================================================================
void Automata::register_AP(){
    for(int64_t i=0; i < ap_size; ++i) {
        std::stringstream ss;
        ss << "p_" << i;
        propositions.push_back( bdd_ithvar(aut_ipath->register_ap(ss.str())) );
        aut_trace->register_ap(ss.str());
    }
}
//=================================================================
void Automata::generate_atomic_BDD(){

    for( int src = 0 ; src < s_init_ip ; src++ ){
        for( int sink = 0 ; sink < s_init_ip ; sink++ ){

            int64_t code = 0;
            bdd curr_bdd = bddfalse;

            if( h_change[src][sink] != 0 ) code = encode_relation(h_change[src][sink]);
            
            for( int64_t i = 0 ; i < ap_size ; ++i ){
                bdd b = propositions[i];
                curr_bdd |= ((code % 2) ? bdd_not(b) : b );
                code >>=1;
            }

            bdd_encoding_global.insert( Pair<Int_pair,bdd>(Int_pair(src,sink) , curr_bdd ));

        }
    }
}
//=================================================================
void Automata::generate_BDD_combinations(){
    // The idea is to have a BDD for each pair of nodes and each pair of heights to utilize 
    // the fact that we can combine BDDs, for now I am focused on soundes rather than optimizing.
    // for this I have 2 maps one for a pair of nodes where I plan to | all BDDs encoutered 
    // another for a pair of heights, same concept.

}
//=================================================================
void Automata::add_transitions(void){


    // for now I am not utilizing the full potential of BDD. Just want to make sure this is sound.
    //      check -void Automata::generate_BDD_combinations()- for "mor"e details.

    for( int src = 0 ; src < s_init_ip ; src++ ){
        for( int sink = 0 ; sink < s_init_ip ; sink++ ){

            bdd_encoding_ipath.insert( Pair<Int_pair,bdd>( Int_pair(src,sink) , bddfalse ) );

            if( h_change[src][sink] != 0 ){
                bdd curr_bdd = bdd_encoding_global.at(Int_pair(src,sink));
                aut_ipath->new_edge(src, sink, curr_bdd, {0});
                aut_ipath->new_edge(s_init_ip, s_init_ip, curr_bdd );
                aut_trace->new_edge(s_init_tr, s_init_tr, curr_bdd );
                for( int i = 0 ; i < s_init_ip ; i++ ){
                    aut_ipath->new_edge(s_init_ip, i, curr_bdd );
                }  
                for( int h1 = 0 ; h1 <= max_height ; h1++ ){
                    aut_trace->new_edge(s_init_tr, h1, curr_bdd );
                }  
            }
        }
    }


    for( int h1 = 0 ; h1 <= max_height ; h1++ ){
        for( int h2 = 0 ; h2 <= max_height ; h2++ ){

            bool has_edge = false;
            bool accepting = false;
            bdd letter_acc = bddfalse;
            bdd letter_not_acc = bddfalse;


            for( int src = 0 ; src < s_init_ip ; src++ ){
                for( int sink = 0 ; sink < s_init_ip ; sink++ ){
                    if( h_change[src][sink] == 0 ){
                        continue;
                    }
                    bdd curr_bdd = bdd_encoding_global.at(Int_pair(src,sink));
                    int s = h_change[src][sink][h1][h2];
                    if( s == Stay ){
                        has_edge = true;
                        letter_not_acc |= curr_bdd;
                    } 
                    else if( s == Downward ){
                        accepting = true;
                        letter_acc |= curr_bdd;
                    }  
                }
            }

            if( has_edge ) aut_trace->new_edge(h1, h2, letter_not_acc);
            if( accepting ) aut_trace->new_edge(h1, h2, letter_acc,{0});

        }
    }
}
//=================================================================
void Automata::init_automata(){
    dict = spot::make_bdd_dict();
    aut_ipath = make_twa_graph(dict);
    aut_trace = make_twa_graph(dict);

    s_init_ip = max_node;
    aut_ipath->set_buchi();
    aut_ipath->new_states(max_node+1);
    aut_ipath->set_init_state(s_init_ip);

    s_init_tr = max_height+1;
    aut_trace->set_buchi();
    aut_trace->new_states(max_height+2);
    aut_trace->set_init_state(s_init_tr);

    ap_size = ceil(log2(pow(3,pow(max_height,2))));

}
//=================================================================
bool Automata::check_soundness(){
    init_automata();
    register_AP();
    generate_atomic_BDD();
    generate_BDD_combinations();
    add_transitions();

    print_dot(std::cout,aut_trace);
    print_dot(std::cout,aut_ipath);

    return spot::contains(aut_trace,aut_ipath);

}
//=================================================================