#include "heighted_graph.hpp"
#include "sloped_relation.hpp"
#include "types.c"
#include "generic_graph.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <fstream>
#include <set>
#include <unordered_set>
#include <thread>
#include <utility>
#include <mutex>
#include <condition_variable>
//=============================================================
// Flag init
//=============================================================
// N.B. These MUST match the corresponding constants in the OCaml code
//      Look in soundcheck.ml
const int Heighted_graph::FAIL_FAST        = 0b0000001;
const int Heighted_graph::USE_SCC_CHECK    = 0b0000010;
const int Heighted_graph::USE_IDEMPOTENCE  = 0b0000100;
const int Heighted_graph::USE_MINIMALITY   = 0b0001000;
const int Heighted_graph::COMPUTE_FULL_CCL = 0b0010000;
const int Heighted_graph::USE_SD           = 0b0100000;
const int Heighted_graph::USE_XSD          = 0b1000000;
//=============================================================
// Essential class methods
//=============================================================
Heighted_graph::Heighted_graph(int max_nodes) {

    assert(max_nodes >= 0);

    this->max_nodes = max_nodes;
    number_of_edges = 0;

#ifdef LOG_STATS
    ccl_initial_size = 0;
    ccl_size = 0;
    ccl_iterations = 0;
    ccl_replacements = 0;
    ccl_rejections = 0;
    compositions = 0;
    comparisons = 0;
    loop_checks = 0;
    checked_size_sum = 0;
    total_size_sum = 0;

    compose_time = std::chrono::duration<double, DURATION>::zero();
    compare_time = std::chrono::duration<double, DURATION>::zero();
    need_to_add_compute_time = std::chrono::duration<double, DURATION>::zero();
    insertion_time = std::chrono::duration<double, DURATION>::zero();
    loop_check_time = std::chrono::duration<double, DURATION>::zero();
#endif

    rejected = new Relation_LIST();

    h_change_ =
        (Sloped_relation***) malloc(sizeof(Sloped_relation**) * max_nodes);
    Ccl = (Relation_LIST***) malloc(sizeof(Relation_LIST**) * max_nodes);

    for(int i = 0; i < max_nodes; i++) {
        Ccl[i] = (Relation_LIST**) malloc(sizeof(Relation_LIST*) * max_nodes);
        h_change_[i] =
            (Sloped_relation**) malloc(sizeof(Sloped_relation*) * max_nodes);
        for (int j = 0; j < max_nodes; j++) {
            h_change_[i][j] = 0;
            Ccl[i][j] = new Relation_LIST();
        }
    }
    edges = new std::vector<Int_pair>();

}

void clean_up(Relation_LIST*** ccl,
              Sloped_relation*** h_change_,
              int num_nodes,
              Relation_LIST* rejected) {

    for (int source = 0; source < num_nodes; source++) {
        for (int sink = 0; sink < num_nodes; sink++) {
            for (Sloped_relation* s : *(ccl[source][sink])) {
                delete s;
            }
            delete ccl[source][sink];
        }
        free(h_change_[source]);
        free(ccl[source]);
    }
    free(ccl);
    free(h_change_);
    for (Sloped_relation* R : *rejected){
        delete R;
    }
    delete rejected;
}

Heighted_graph::~Heighted_graph(void) {
    for (Map<int, int>* idx_map : height_idxs) {
        if (idx_map) delete idx_map;
    }
    for (Int_SET* heights : HeightsOf) {
        if (heights) delete heights;
    }
    if (edges) delete edges;
    clean_up(Ccl, h_change_, max_nodes, rejected);
    // std::thread t(clean_up, Ccl, h_change_, max_nodes, rejected);
    // t.detach();
}

//=============================================================
// Infinite descent main algorithms
//=============================================================
bool Heighted_graph::relational_check(int opts){

    this->flags = opts;

    // if ((opts & FAIL_FAST) != 0) std::cout << "Fail Fast\n";
    // if ((opts & USE_SCC_CHECK) != 0) std::cout << "Use SCC Check\n";
    // if ((opts & USE_IDEMPOTENCE) != 0) std::cout << "Use Idempotence\n";
    // if ((opts & USE_MINIMALITY) != 0) std::cout << "Use Minimality\n";

    // We cannot combine the idempotence and minimality optimisations.
    assert(((opts & USE_IDEMPOTENCE) == 0) || ((opts & USE_MINIMALITY) == 0));
    // It doesn't make sense to combine the idempotence and the SCC-based loop check
    assert(((opts & USE_IDEMPOTENCE) == 0) || ((opts & USE_SCC_CHECK) == 0));

    /* N.B. Initially, we though that it is useless to combine the fast-fail and
            minimality optimisations, since the point of the minimality
            optimisation is to not have to check for self-loops in all sloped
            relations. So when applying the minimality optimisation, it can be
            that we replace a sloped relation with a self-loop by one without a
            self-loop. Therefore, if checking for self-loops on-the-fly, we
            would still end up having to check every sloped relation anyway.

            However, experimentally, we did not see sloped relations being
            replaced in the CCL when using the minimality optimisation. Instead,
            we only observed that newly computed relations were rejected,
            leading to the CCL being significantly smaller in some cases.
            So, this being the case, it **does** make sense to combine it with
            fast-fail: if we detect that we need to add a newly computed sloped
            relation to the CCL, at that point we check the self-loop, and
            otherwise we simply reject the relation and fail.
     */

    // If fail-fast, then need to check initial sloped relations for self-loops
    if ((opts & FAIL_FAST) != 0) {
        if (!check_Ccl(opts)) {
            return false;
        }
    }

#ifdef LOG_STATS
    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::time_point<std::chrono::system_clock> end;
#endif

    int num_nodes = this->num_nodes();

    // Now compute the CCL
    bool done;
    do {
#ifdef LOG_STATS
        ccl_iterations++;
#endif
        // reset loop flag
        done = true;
        for (int source = 0; source < num_nodes; source++) {
        for (int sink = 0; sink < num_nodes; sink++) {
        for (int middle = 0; middle < num_nodes; middle++) {


            for (
                auto left = Ccl[source][middle]->begin();
                left != Ccl[source][middle]->end();
                left++
            ) {
                bool continue_left = false;
                Sloped_relation* P = *left;
                P->initialize();
                for (
                    auto right = Ccl[middle][sink]->begin();
                    right != Ccl[middle][sink]->end();
                    right++
                ) {
                    Sloped_relation* Q = *right;
                    Q->initialize();
#ifdef LOG_STATS
                    start = std::chrono::system_clock::now();
#endif
                    Sloped_relation* R = P->compose(*Q);
#ifdef LOG_STATS
                    end = std::chrono::system_clock::now();
                    compose_time += (end - start);
                    compositions++;
                    total_size_sum += R->size();
#endif

                    bool need_to_add = true;
#ifdef LOG_STATS
                    auto loop_start = std::chrono::system_clock::now();
#endif
                    for (
                        auto outer = Ccl[source][sink]->begin();
                        outer != Ccl[source][sink]->end();
                        outer++
                    ) {
                        Sloped_relation* S = *outer;
                        S->initialize();
#ifdef LOG_STATS
                        comparisons++;
#endif
                        if ((opts & USE_MINIMALITY) != 0) {
#ifdef LOG_STATS
                            start = std::chrono::system_clock::now();
#endif
                            comparison result = R->compare(*S);
#ifdef LOG_STATS
                            end = std::chrono::system_clock::now();
                            compare_time += (end - start);
#endif
                            if (result == lt) {
#ifdef LOG_STATS
                                ccl_replacements++;
                                ccl_size--;
#endif
                                auto to_delete = outer--;
                                if (to_delete == left){
                                    continue_left = true;
                                    left--;
                                }
                                if (to_delete == right) {
                                    right--;
                                }
                                (Ccl[source][sink])->erase(to_delete);
                                delete S;
                                break;
                            }
                            else if (result == eq || result == gt) {
#ifdef LOG_STATS
                                ccl_rejections++;
#endif
                                need_to_add = false;
                                break;
                            }
                        } else {
#ifdef LOG_STATS
                            start = std::chrono::system_clock::now();
#endif
                            bool equal = (*S == *R);
#ifdef LOG_STATS
                            end = std::chrono::system_clock::now();
                            compare_time += (end - start);
#endif
                            if (equal) {
#ifdef LOG_STATS
                                ccl_rejections++;
#endif
                                need_to_add = false;
                                break;
                            }
                        }
                    }
#ifdef LOG_STATS
                    auto loop_end = std::chrono::system_clock::now();
                    need_to_add_compute_time += (loop_end - loop_start);
#endif

                    // If fail-fast, then check for self-loop if necessary
                    bool fail_now = false;
                    if (need_to_add 
                            && ((opts & FAIL_FAST) != 0)
                            && source == sink
                            && !(check_self_loop(R, source, opts))) {
                        delete R;
                        return false;
                    }

                    if (need_to_add) {
                        // Otherwise, not done with the fixed point computation
                        done = false;
                        // So add the newly computed sloped relation
#ifdef LOG_STATS
                        start = std::chrono::system_clock::now();
#endif
                        (Ccl[source][sink])->push_back(R);
#ifdef LOG_STATS
                        end = std::chrono::system_clock::now();
                        insertion_time += (end - start);
                        ccl_size++;
#endif
                        if (continue_left) {
                            break;
                        }
                    } else {
                        delete R;
                    }
                }
            }

        }
        }
        }
    } while (((opts & COMPUTE_FULL_CCL) != 0) && !done);

    // If not using fast-fail, then check for self-loops in the computed CCL
    if ((opts & FAIL_FAST) == 0) {
        if (!check_Ccl(opts)) {
            return false;
        }
    }

    return true;
}

bool Heighted_graph::sd_check() {
    this->flags |= USE_SD;
    return set_based_check(STD);
}

bool Heighted_graph::xsd_check() {
    this->flags |= USE_XSD;
    return set_based_check(XTD);
}

//=============================================================
// Methods for constructing the height graph
//=============================================================
void Heighted_graph::add_node(int n) {
    if (node_idxs.find(n) == node_idxs.end()) {
        int next_idx = node_idxs.size();
        assert(next_idx < max_nodes);
        node_idxs[n] = next_idx;
        height_idxs.push_back(new Map<int, int>());
        HeightsOf.push_back(new Int_SET());
    }
}

void Heighted_graph::add_height(int n, int h) {
    // for each node the heights are mapped in an increasing order,
    // i.e the heights are local -- node 1 and 2 might have height 4
    // but for them locally one could have it as 0 and the other as 2 !!!
    add_node(n);
    int n_idx = node_idxs.at(n);
    Map<int, int>* heights = height_idxs[n_idx];
    if (heights->find(h) == heights->end()) {
        int next_idx = heights->size();
        heights->insert(Int_pair(h, next_idx));
    }
    int h_idx = heights->at(h);
    HeightsOf[n_idx]->insert(h_idx);
    if( max_height < h_idx ) max_height = h_idx;
    if( HeightsOf[n_idx]->size() > this->max_height_set_size ){
        this->max_height_set_size =  HeightsOf[n_idx]->size();
    }
}

void Heighted_graph::add_edge(int source, int sink) {
    add_node(source);
    add_node(sink);
    int src_idx = node_idxs.at(source);
    int sink_idx = node_idxs.at(sink);
    if (h_change_[src_idx][sink_idx] == 0) {
        number_of_edges++;
        edges->push_back(Int_pair(src_idx,sink_idx));
        int num_src_heights = height_idxs[src_idx]->size();
        int num_dst_heights = height_idxs[sink_idx]->size();
        Sloped_relation* R =
            new Sloped_relation(num_src_heights, num_dst_heights);
        h_change_[src_idx][sink_idx] = R;
        Ccl[src_idx][sink_idx]->push_back(R);
#ifdef LOG_STATS
        ccl_initial_size++;
        ccl_size++;
#endif
    }
}

void Heighted_graph::add_hchange(int source, int source_h, int sink, int sink_h, slope s) {
    add_edge(source, sink);
    add_height(source, source_h);
    add_height(sink, sink_h);
    int src_idx = node_idxs[source];
    int src_h_idx = height_idxs[src_idx]->at(source_h);
    int sink_idx = node_idxs[sink];
    int sink_h_idx = height_idxs[sink_idx]->at(sink_h);
    h_change_[src_idx][sink_idx]->add(src_h_idx, sink_h_idx, s);
}

void Heighted_graph::add_stay(int source_node, int source_h, int sink_node, int sink_h) {
    add_hchange(source_node, source_h, sink_node, sink_h, Stay);
}

void Heighted_graph::add_decrease(int source_node, int source_h, int sink_node, int sink_h) {
    add_hchange(source_node, source_h, sink_node, sink_h, Downward);
}


//=============================================================
// Misc.
//=============================================================
int Heighted_graph::num_nodes(void) {
    return node_idxs.size();
}

int Heighted_graph::num_edges(void) {
    return number_of_edges;
}

void Heighted_graph::print_Ccl(void){
    int num_nodes = this->num_nodes();
    for( int source = 0 ; source < num_nodes ; source++ ){
    for( int sink = 0 ; sink < num_nodes  ; sink++ ){
        for( auto S : *Ccl[source][sink]){
            std::cout << "<><><><><><><><><><><><><><><><><><><><><><><><><><><><><>" << std::endl;
            std::cout << source << " " << sink << std::endl;
            S->print_();
            std::cout << "<><><><><><><><><><><><><><><><><><><><><><><><><><><><><>" << std::endl;
        }
    }
    }
}

void Heighted_graph::print_statistics(void) {
    print_flags(this->flags);
#ifdef LOG_STATS
    std::cout << "Initial CCL size: " << ccl_initial_size << std::endl;
    std::cout << "Final CCL size: " << ccl_size << std::endl;
    std::cout << "Number of iterations to compute CCL: " << ccl_iterations << std::endl;
    std::cout << "CCL Rejections: " << ccl_rejections << std::endl;
    std::cout << "CCL Replacements: " << ccl_replacements << std::endl;
    std::cout << "Sloped relations computed: " << compositions << std::endl;
    std::cout << "Time spent computing sloped relations (ms): "
              << compose_time.count() << std::endl;
    std::cout << "Sloped relations compared: " << comparisons << std::endl;
    std::cout << "Time spent computing \"need to add\" (ms): "
              << need_to_add_compute_time.count() << std::endl;
    std::cout << "(of which) time spent comparing sloped relations (ms): "
              << compare_time.count() << std::endl;
    std::cout << "Time spent inserting sloped relations (ms): "
              << insertion_time.count() << std::endl;
    std::cout << "Number of self-loop checks: " << loop_checks << std::endl;
    std::cout << "Time spent loop checking (ms): "
              << loop_check_time.count() << std::endl;
    std::cout << "Average size of loop-checked sloped relations: "
              << ((loop_checks == 0) ? 0 : (checked_size_sum / loop_checks))
              << std::endl;
    std::cout << "Average size of all computed sloped relations: "
              << ((compositions == 0) ? 0 : (total_size_sum / compositions))
              << std::endl;
#endif
}

int Heighted_graph::parse_flags(const std::string flags_s) {
    int flags = 0;
    for (char c : flags_s) {
        switch (c) {
            case 'f': flags |= FAIL_FAST; break;
            case 's': flags |= USE_SCC_CHECK; break;
            case 'i': flags |= USE_IDEMPOTENCE; break;
            case 'm': flags |= USE_MINIMALITY; break;
            case 'A': flags |= COMPUTE_FULL_CCL; break;
            case 'D': flags |= USE_SD; break;
            case 'X': flags |= USE_XSD; break;
        }
    }
    assert(((flags & USE_SD) == 0) || ((flags & USE_XSD) == 0));
    return flags;
}

void Heighted_graph::print_flags(int flags) {
    if ((flags & FAIL_FAST) != 0) std::cout << "FAIL_FAST" << std::endl;
    if ((flags & USE_SCC_CHECK) != 0) std::cout << "USE_SCC_CHECK" << std::endl;
    if ((flags & USE_IDEMPOTENCE) != 0) std::cout << "USE_IDEMPOTENCE" << std::endl;
    if ((flags & USE_MINIMALITY) != 0) std::cout << "USE_MINIMALITY" << std::endl;
    if ((flags & COMPUTE_FULL_CCL) != 0) std::cout << "COMPUTE_FULL_CCL" << std::endl;
    if ((flags & USE_SD) != 0) std::cout << "USE_SD" << std::endl;
    if ((flags & USE_XSD) != 0) std::cout << "USE_XSD" << std::endl;
}

//=============================================================
// Helper functions
//=============================================================
bool Heighted_graph::check_self_loop(Sloped_relation* R, int node, int opts) {

#ifdef LOG_STATS
    auto start = std::chrono::system_clock::now();
    loop_checks++;
    checked_size_sum += R->size();
#endif

    bool result = false;

    if ((opts & USE_SCC_CHECK) != 0) {
        result = R->has_downward_SCC();
    } else {
        // Compute R composed with R if using the idempotent method
        // or the transitive closure otherwise (i.e. using the standard method)
        Sloped_relation* R2 = 
            ((opts & USE_IDEMPOTENCE) != 0)
                ? R->compose(*R)
                : R->compute_transitive_closure();

        // If we're using the idempotent method and the relation is not
        // idempotent then trivially return true
        if (((opts & USE_IDEMPOTENCE) != 0) && !(*R2 == *R)) {
            result = true;
#ifdef LOG_STATS
            loop_checks--;
            checked_size_sum -= R->size();
#endif
        } else {
            // Otherwise, check we have a self-loop in the relevant relation
            Map<Int_pair,int>* slopes = R2->get_slopes();

            if (HeightsOf[node]->size() < slopes->size()) {
                // Iterate over all heights h in the node, to see if (h, h) is
                // mapped to the Downward slope.
                for( int h : *(HeightsOf.at(node)) ){
                    auto exists = slopes->find(Int_pair(h,h));
                    if( exists != slopes->end() && exists->second == Downward ){
                        result = true;
                        break;
                    }
                }
            } else {
                // Only iterate over all entries in the transitive closure of R
                // if this will be quicker then going over the heights
                for(auto it = slopes->begin(); it != slopes->end(); it++) {
                    Int_pair heights = it->first;
                    if (heights.first == heights.second && it->second == Downward) {
                        result = true;
                        break;
                    }
                }
            }
        }
        rejected->push_back(R2);
    }

#ifdef LOG_STATS
    auto end = std::chrono::system_clock::now();
    loop_check_time += end - start;
#endif

    return result;
}

bool Heighted_graph::check_Ccl(int opts) {
    int num_nodes = this->num_nodes();
    for( int node = 0; node < num_nodes; node++ ){
        Relation_LIST* Ccl_nd = Ccl[node][node];
        for( Sloped_relation* R : *Ccl_nd ){
            R->initialize();
            if(!check_self_loop(R, node, opts)) {
                return false;
            }
        }
    }
    return true;
}

bool Heighted_graph::set_based_check(SD_decrease_type SD_DEC_TYPE) {
    generic_graph G;
    G.set_edges(edges,num_nodes());
    switch( SD_DEC_TYPE ){
        case STD: 
            return G.check_SD(&HeightsOf,h_change_);
            break;
        case XTD: 
            return G.check_XSD(&HeightsOf,h_change_,this->max_height_set_size);
            break;
    }
    return false;
}

//=============================================================



void thor_sd(Heighted_graph* hg){
   
    
    // // Wait until main() sends data
    {
        std::unique_lock<std::mutex> lk(*(hg->m));
        // cv.wait(lk, []{return mainReady;});
        hg->cv->wait(lk, [hg]{return *(hg->mainReady);});
    }
        std::cout << "thor_sd"<< std::endl;

 
    // std::cout << "Worker thread 2 is processing data: " << data << std::endl;
 
    // Send data back to main()
    {
        // std::lock_guard<std::mutex> lk(*(hg->m));
        std::cout << "SD START" << std::endl;
        bool res = hg->sd_check();
        std::cout << "SD DONE" << std::endl;

        *(hg->SD_done) = true;
        std::lock_guard<std::mutex> lk(*(hg->m));
        hg->res->at(0) = (res ? 1 : 0);
        // std::cout << "Worker thread 2 signals data processing completed\n";
    }
    hg->cv->notify_all();
    


}

void thor_xsd(Heighted_graph* hg){


    // // Wait until main() sends data
    {
        std::unique_lock<std::mutex> lk(*(hg->m));
        // cv.wait(lk, []{return mainReady;});
        hg->cv->wait(lk, [hg]{return *(hg->mainReady);});
    }
        std::cout << "thor_xsd"<< std::endl;

    // std::cout << "Worker thread 2 is processing data: " << data << std::endl;
 
    // Send data back to main()
    {
        // std::lock_guard<std::mutex> lk(*(hg->m));

        std::cout << "XSD START" << std::endl;
        bool res = hg->xsd_check();
        std::cout << "XSD DONE" << std::endl;

        *(hg->XSD_done) = true;
        std::lock_guard<std::mutex> lk(*(hg->m));
        hg->res->at(1) = (res ? 1 : 0);
        // std::cout << "Worker thread 2 signals data processing completed\n";
    }
    hg->cv->notify_all();

}

void thor_rel(Heighted_graph* hg,int opts){
    

    // // Wait until main() sends data
    {
        std::unique_lock<std::mutex> lk(*(hg->m));
        // cv.wait(lk, []{return mainReady;});
        hg->cv->wait(lk, [hg]{return *(hg->mainReady);});
    }
    std::cout << "thor_rel"<< std::endl;
 
    // // std::cout << "Worker thread 2 is processing data: " << data << std::endl;
 
    // Send data back to main()
    {
        

        std::cout << "REL START" << std::endl;
        bool res = hg->relational_check(opts);
        std::cout << "REL DONE" << std::endl;

        *(hg->REL_done) = true;
        std::lock_guard<std::mutex> lk(*(hg->m));
        hg->res->at(2) = (res ? 1 : 0);
        // std::cout << "Worker thread 2 signals data processing completed\n";
    }
    hg->cv->notify_all();

}








bool Heighted_graph::thors_hammer(int opts,Heighted_graph* hg_sd,Heighted_graph* hg_xsd){

    // std::mutex* m;
    // std::condition_variable* cv;
    // bool* mainReady
    // bool* SD_done;
    // bool* XSD_done;
    // bool* REL_done;
    // std::vector<int>* res;

    this->m = new std::mutex();
    this->cv = new std::condition_variable();
    this->mainReady = new bool(false);
    this->SD_done = new bool(false);
    this->XSD_done = new bool(false);
    this->REL_done = new bool(false);
    this->res = new std::vector<int>();

    hg_sd->m =this->m;
    hg_sd->cv =this->cv;
    hg_sd->mainReady =this->mainReady;
    hg_sd->SD_done =this->SD_done;
    hg_sd->XSD_done =this->XSD_done;
    hg_sd->REL_done =this->REL_done;
    hg_sd->res =this->res;

    hg_xsd->m =this->m;
    hg_xsd->cv =this->cv;
    hg_xsd->mainReady =this->mainReady;
    hg_xsd->SD_done =this->SD_done;
    hg_xsd->XSD_done =this->XSD_done;
    hg_xsd->REL_done =this->REL_done;
    hg_xsd->res =this->res;


    res->push_back(-1);
    res->push_back(-1);
    res->push_back(-1);

    // Heighted_graph* hg_sd = new Heighted_graph(max_nodes); 
    // Heighted_graph* hg_xsd = new Heighted_graph(max_nodes);

 
    std::thread t_sd(thor_sd,hg_sd);
    std::thread t_xsd(thor_xsd,hg_xsd);
    std::thread t_rel(thor_rel,this,opts);

    t_sd.detach();
    t_xsd.detach();
    t_rel.detach();
    
    {
        std::lock_guard<std::mutex> lk(*m);
        *mainReady = true;
    }
    cv->notify_all();


    {
        std::unique_lock<std::mutex> lk(*m);
        cv->wait(lk, [this]{
            if( this->res->at(0) != -1){
                std::cout << "SD DONE " << (this->res->at(0)) << std::endl;
                if( this->res->at(0) == 1 ) return true;
                // return true;
                // return (res->at(0) == 1 ? true : false );
            }
            if( this->res->at(1) != -1 ){
                std::cout << "XSD DONE "<< (this->res->at(1) )<< std::endl;
                if( this->res->at(1) == 1 ) return true;
            }
            if( this->res->at(2) != -1 ){
                std::cout << "REL DONE "<< (this->res->at(2) )<< std::endl;
                return true;
            }
            std::cout << "HELLO" << std::endl;
            return false;
            }
        );
    }
    std::cout << "Back in main()"<< '\n';

    


    std::cout << "PAPA"<< '\n';
 
  
    // t_xsd.detach();
    // t_rel.detach();
    if( res->at(0) == 1 ) return true;
    if( res->at(1) == 1 ) return true;
    return (res->at(2) == 1 ? true : false);

}


