#include "heighted_graph.hpp"
#include "sloped_relation.hpp"
#include "types.c"

#include <cassert>
#include <chrono>
#include <iostream>
#include <fstream>
#include <set>
#include <thread>
#include <utility>

// N.B. These MUST match the corresponding constants in the OCaml code
//      Look in soundcheck.ml
const int Heighted_graph::FAIL_FAST       = 0b0001;
const int Heighted_graph::USE_SCC_CHECK   = 0b0010;
const int Heighted_graph::USE_IDEMPOTENCE = 0b0100;
const int Heighted_graph::USE_MINIMALITY  = 0b1000;

int Heighted_graph::parse_flags(const std::string flags_s) {
    int flags = 0;
    for (char c : flags_s) {
        switch (c) {
            case 'f': flags |= FAIL_FAST; break;
            case 's': flags |= USE_SCC_CHECK; break;
            case 'i': flags |= USE_IDEMPOTENCE; break;
            case 'm': flags |= USE_MINIMALITY; break;
        }
    }
    return flags;
}

// Construcor
Heighted_graph::Heighted_graph(int max_nodes) {

    assert(max_nodes >= 0);

    this->max_nodes = max_nodes;
    number_of_edges = 0;

    ccl_initial_size = 0;
    ccl_size = 0;
    ccl_replacements = 0;
    ccl_rejections = 0;
    compositions = 0;
    comparisons = 0;
    loop_checks = 0;
    checked_size_sum = 0;

    compose_time = DURATION::zero();
    compare_time = DURATION::zero();
    loop_check_time = DURATION::zero();

    rejected = new Relation_LIST();

    h_change_ =
        (Sloped_relation***) malloc(sizeof(Sloped_relation**) * max_nodes);
    Ccl =
        (Relation_LIST_LIST***) malloc(sizeof(Relation_LIST_LIST**) * max_nodes);

    for( int i = 0 ; i < max_nodes ; i++ ){
        Ccl[i] = 
            (Relation_LIST_LIST**) malloc(sizeof(Relation_LIST_LIST*) * max_nodes);
        h_change_[i] =
            (Sloped_relation**) malloc(sizeof(Sloped_relation*) * max_nodes);
        for( int j = 0 ; j < max_nodes ; j++ ){
            h_change_[i][j] = 0;
            Ccl[i][j] = new Relation_LIST_LIST();
            Ccl[i][j]->push_front(new Relation_LIST());
        }
    }

}

void clean_up(Relation_LIST_LIST*** ccl,
              Sloped_relation*** h_change_,
              int num_nodes,
              Relation_LIST* rejected) {

    for (int source = 0; source < num_nodes; source++) {
        for (int sink = 0; sink < num_nodes; sink++) {
            for (Relation_LIST* rels : *ccl[source][sink]) {
                for (Sloped_relation* r : *rels) {
                    delete r;
                }
                delete rels;
            }
            delete ccl[source][sink];
        }
        delete h_change_[source];
        delete ccl[source];
    }
    delete ccl;
    delete h_change_;

    for (Sloped_relation* R : *rejected){
        delete R;
    }
    delete rejected;

}

// Destructor
Heighted_graph::~Heighted_graph(void) {
    for(Int_SET* heights : HeightsOf){
        delete heights;
    }
    std::thread t(clean_up, Ccl, h_change_, max_nodes, rejected);
    t.detach();
}

// Methods for constructing the height graph
void Heighted_graph::add_node(int n) {
    if (node_idxs.find(n) == node_idxs.end()) {
        int next_idx = node_idxs.size();
        assert(next_idx < max_nodes);
        node_idxs[n] = next_idx;
        HeightsOf.push_back(new Int_SET());
    }
}

void Heighted_graph::add_height(int n, int h) {
    add_node(n);
    int idx = node_idxs.at(n);
    HeightsOf[idx]->insert(h);
}

void Heighted_graph::add_edge(int source, int sink) {
    add_node(source);
    add_node(sink);
    int src_idx = node_idxs[source];
    int sink_idx = node_idxs[sink];
    if ( h_change_[src_idx][sink_idx] == 0 ) {
        number_of_edges++;
        Sloped_relation* R = new Sloped_relation(new Map<int,Int_pair_SET*>(),new Map<int,Int_pair_SET*>(),new Map<Int_pair,int>());
        h_change_[src_idx][sink_idx] = R;
        (Ccl[src_idx][sink_idx]->front())->push_front(R);
        ccl_initial_size++;
        ccl_size++;
    }
}

void Heighted_graph::add_hchange(int source, int source_h, int sink, int sink_h, slope s) {
    add_edge(source, sink);
    int src_idx = node_idxs[source];
    int sink_idx = node_idxs[sink];
    h_change_[src_idx][sink_idx]->add(source_h, sink_h, s);
}

void Heighted_graph::add_stay(int source_node, int source_h, int sink_node, int sink_h) {
    add_hchange(source_node, source_h, sink_node, sink_h, Stay);
}

void Heighted_graph::add_decrease(int source_node, int source_h, int sink_node, int sink_h) {
    add_hchange(source_node, source_h, sink_node, sink_h, Downward);
}

int Heighted_graph::num_nodes(void) {
    return node_idxs.size();
}

int Heighted_graph::num_edges(void) {
    return number_of_edges;
}

struct ccl_data {
    int additions = 0;
    int compositions = 0;
    int comparisons = 0;
    int rejections = 0;
    int replacements = 0;
    int loop_checks = 0;
    int checked_size_sum = 0;
    DURATION compose_time = DURATION::zero();
    DURATION compare_time = DURATION::zero();
    DURATION loop_check_time = DURATION::zero();
};

bool check_self_loop(Sloped_relation& R, int node, Int_SET& node_heights, 
                     ccl_data& data, int opts) {

    auto start = std::chrono::system_clock::now();

    data.loop_checks++;
    data.checked_size_sum += R.size();

    bool result = false;

    if ((opts & Heighted_graph::USE_SCC_CHECK) != 0) {
        result = R.has_downward_SCC();
    } else {
        // Compute R composed with R if using the idempotent method
        // or the transitive closure otherwise (i.e. using the standard method)
        Sloped_relation* R2 = 
            ((opts & Heighted_graph::USE_IDEMPOTENCE) != 0)
                ? R.compose(R)
                : R.compute_transitive_closure();

        // If we're using the idempotent method and the relation is not
        // idempotent then trivially return true
        if (((opts & Heighted_graph::USE_IDEMPOTENCE) != 0) && !(*R2 == R)) {
            result = true;
            data.loop_checks--;
            data.checked_size_sum -= R.size();
        } else {
            // Otherwise, check we have a self-loop in the relevant relation
            Map<Int_pair,int>* slopes = R2->get_slopes();

            if (node_heights.size() < slopes->size()) {
                // Iterate over all heights h in the node, to see if (h, h) is
                // mapped to the Downward slope.
                for (int h : node_heights) {
                    auto exists = slopes->find(Int_pair(h,h));
                    if (exists != slopes->end() && exists->second == Downward) {
                        result = true;
                        break;
                    }
                }
            } else {
                // Only iterate over all entries in the transitive closure of R
                // if this will be quicker then going over the heights
                for (auto it = slopes->begin(); it != slopes->end(); it++) {
                    Int_pair heights = it->first;
                    if (heights.first == heights.second && it->second == Downward) {
                        result = true;
                        break;
                    }
                }
            }
        }
    }

    auto end = std::chrono::system_clock::now();
    data.loop_check_time += end - start;

    return result;
}

enum CompareResult {
    CONTINUE,
    REJECT,
    REPLACE
};

CompareResult compare_rels(Sloped_relation& R, Sloped_relation& S,
                           ccl_data& data, int opts) {

    data.comparisons++;

    if ((opts & Heighted_graph::USE_MINIMALITY) != 0) {
        auto start = std::chrono::system_clock::now();
        comparison result = R.compare(S);
        auto end = std::chrono::system_clock::now();
        data.compare_time += (end - start);
        switch (result) {
            case lt: return REPLACE;
            case eq:
            case gt: return REJECT;
        }
        return CONTINUE;
    } else {
        auto start = std::chrono::system_clock::now();
        bool equal = (R == S);
        auto end = std::chrono::system_clock::now();
        data.compare_time += (end - start);
        if (equal) {
            return REJECT;
        } else {
            return CONTINUE;
        }
    }

}

/*
 * Method to compose two lists of sloped relations that come from the CCL.
 * Used by Heighted_graph::check_soundness().
 * 
 * Returns false if FAST_FAIL optimisation is on and a new sloped relation to be
 * added does not pass the self-loop test. Otherwise returns true.
 * 
 * Records data, including the number of new sloped relations added to the CCL
 * in the ccl_data& argument.
 */
bool compose_lists(Relation_LIST_LIST* composed, 
                   Relation_LIST* left, Relation_LIST* right,
                   int source, int sink, Int_SET& src_heights,
                   Relation_LIST* rejected, ccl_data& data, int opts) {

    // std::cout << "Beginning list composition" << std::endl;
    // if (left == NULL) std::cout << "left is NULL!" << std::endl;
    // if (right == NULL) std::cout << "right is NULL!" << std::endl;

    // std::cout << "Size of left list: " << left->size() << std::endl;
    for (Sloped_relation* P : *left) {

        // if (P == NULL) std::cout << "P is NULL!" << std::endl;
        if (P->size() == 0) continue;

        for (Sloped_relation* Q : *right) {

            // if (Q == NULL) std::cout << "Q is NULL!" << std::endl;
            if (Q->size() == 0) continue;

            // Generate new sloped relation
            auto start = std::chrono::system_clock::now();
            // std::cout << "About to compose relations" << std::endl;
            Sloped_relation* R = P->compose(*Q);
            auto end = std::chrono::system_clock::now();
            data.compose_time += (end - start);

            data.compositions++;

            if (R->size() == 0) {
                rejected->push_back(R);
                continue;
            }

            bool need_to_add = true;
            bool cont = true;

            // std::cout << "Computed new composition for " << source << ", " << sink << std::endl << *R;
            
            // Now compare with the existing relations to see if we need to add
            // the new one to the CCL
            auto outer = composed->begin();
            while (outer != composed->end()) {

                for (auto inner = (*outer)->begin(); inner != (*outer)->end(); inner++) {
                    Sloped_relation* S = *inner;
                    // std::cout << "Comparing with existing relation" << std::endl << *S;
                    CompareResult res = compare_rels(*R, *S, data, opts);
                    // std::cout << "Result is " << res << std::endl;
                    if (res == REJECT) {
                        data.rejections++;
                        rejected->push_back(R);
                        need_to_add = false;
                    }
                    if (res == REPLACE) {
                        // std::cout << "Replacing" << std::endl;
                        data.replacements++;
                        rejected->push_back(S);
                        (*outer)->erase(inner);
                    }
                    if (res != CONTINUE) {
                        cont = false;
                        break;
                    }
                }

                /*** SOMETHING WRONG WITH THIS - DON'T KNOW WHAT YET ***/
                // If we have replaced the last sloped relation in a collection
                // of sloped relationss generated in a previous iteration of the
                // fixed point computation, then remove this now empty collection
                // if (outer != composed->begin() 
                //       && (*outer)->size() == 0 && composed->size() > 2) {
                //     std::cout << "Removing an empty collection "
                //               << "from CCL bucket of size "
                //               << composed->size()
                //               << std::endl;
                //     auto to_erase = outer;
                //     outer++;
                //     composed->erase(to_erase);
                //     continue;
                // }

                if (!cont) {
                    break;
                }

                outer++;

            }

            // If fail-fast, then check for self-loop if necessary
            bool fail_now = false;
            if (need_to_add 
                    && ((opts & Heighted_graph::FAIL_FAST) != 0)
                    && source == sink
                    && !(check_self_loop(*R, source, src_heights, data, opts))) {
                fail_now = true;
                need_to_add = false;
            }

            if (need_to_add) {
                (composed->front())->push_back(R);
                // std::cout << "Adding relation, source " << source 
                //           << ", sink " << sink << std::endl
                //           << *R;
                data.additions++;
            } else {
                rejected->push_back(R);                        
            }

            if (fail_now) { return false; }
        }
    }

    // std::cout << "Finished composing relation lists" << std::endl;

    return true;

}

bool Heighted_graph::check_soundness(int opts){

    // if ((opts & FAIL_FAST) != 0) std::cout << "Fail Fast\n";
    // if ((opts & USE_SCC_CHECK) != 0) std::cout << "Use SCC Check\n";
    // if ((opts & USE_IDEMPOTENCE) != 0) std::cout << "Use Idempotence\n";
    // if ((opts & USE_MINIMALITY) != 0) std::cout << "Use Minimality\n";

    // We cannot combine the idempotence and minimality optimisations.
    assert(((opts & USE_IDEMPOTENCE) == 0) || ((opts & USE_MINIMALITY) == 0));
    // It doesn't make sense to combine the idempotence and the SCC-based loop check
    assert(((opts & USE_IDEMPOTENCE) == 0) || ((opts & USE_SCC_CHECK) == 0));

    /* N.B. It is useless to combine the fast-fail and minimality optimisations.
            The point of the minimality optimisation is to not have to check for
            self-loops in all sloped relations. But when applying the minimality
            optimisation, it can be that we replace a sloped relation with a
            self-loop by one without a self-loop. Therefore, if checking for
            self-loops on-the-fly, we would still end up having to check every
            sloped relation anyway.

            If both flags are set, then we ignore (unset) fast-fail
     */
    if ((opts & USE_MINIMALITY) != 0) {
        opts &= ~FAIL_FAST;
    }

    // If fail-fast, then need to check initial sloped relations for self-loops
    if ((opts & FAIL_FAST) != 0) {
        if (!check_Ccl(opts)) {
            return false;
        }
    }

    Relation_LIST* rejected = new Relation_LIST();

    // Now compute the CCL
    bool done = false;
    int num_nodes = this->num_nodes();
    while( !done ){

        // std::cout << "New iteration" << std::endl;
        // print_Ccl();

        // reset loop flag
        done = true;
        
        // Push new lists onto the CCL to store new relations
        for (int n = 0; n < num_nodes; n++) {
            for (int m = 0; m < num_nodes; m++) {
                Relation_LIST_LIST* rel_list = Ccl[m][n];
                // Avoid building up more empty lists than necessary
                if (rel_list->size() > 1) {
                    auto start = rel_list->begin();
                    if ((*start)->size() == 0 && (*(std::next(start)))->size() == 0) {
                        continue;
                    }
                    if ((*start)->size() != 0 && (*(std::next(start)))->size() == 0) {
                        rel_list->erase(std::next(start));
                    }
                }
                // Push new empty list
                Ccl[n][m]->push_front(new Relation_LIST());
            }
        }

        /* Assumption: the method has not been called before
         * Invariants, for all m,n:
         *   - Ccl[m][n]->size() >= 1
         *   - CCl[m][n] contains no more than 2 empty lists, and these will
         *       always be the first lists in the collection
         *
         * First list in Ccl[m][n] is where we collect new relations produced
         *   in this iteration.
         * Second list in Ccl[m][n] contains the new relations produced in the
         *   previous iteration
         * Any lists after this contain relations generated before that.
         * Note that we don't have to combine these older relations now, as we
         * will already have done that in a previous iteration.
         */

        for (int source = 0; source < num_nodes; source++) {
        for (int sink = 0; sink < num_nodes; sink++) {

            // The contents of this nested for loop can be parallelised

            // Record for storing performance metrics
            ccl_data data;
            // std::cout << "Additions: " << data.additions << std::endl;
            bool fail_now = false;

            for (int middle = 0; middle < num_nodes; middle++) {

                // std::cout << "Beginning new intermediate iteration" << std::endl;

                // No relations to compose
                if (Ccl[source][middle]->size() == 1) continue;
                if (Ccl[middle][sink]->size() == 1) continue;

                auto left_start = std::next(Ccl[source][middle]->begin());
                auto right_start = std::next(Ccl[middle][sink]->begin());

                // Get new relations computed from last iteration
                Relation_LIST* left_new = *left_start;
                Relation_LIST* right_new = *right_start;

                // If no new relations computed last iteration, nothing to do
                if (left_new->size() == 0 && right_new->size() == 0) {
                    continue;
                }

                // Otherwise there are potentially new relations to compute
                auto left_rest = std::next(left_start);
                auto right_rest = std::next(right_start);

                /*
                 * 1. Compose R in left_new with R' in right_new
                 * 2. Compose R in left_rest with R' in right_new
                 * 3. Compose R in left_new with R' in right_rest
                 * 
                 * Don't need to compose R in left_rest with R' in right_rest
                 * since this will already have been done in a previously
                 */
                bool result;

                // std::cout << "1. Compose R in left_new with R' in right_new" << std::endl;
                // std::cout << "" << std::endl;
                // 1. Compose R in left_new with R' in right_new
                result = compose_lists(Ccl[source][sink],
                                       left_new, right_new,
                                       source, sink, *(HeightsOf.at(source)),
                                       rejected, data, opts);
                if (((opts & FAIL_FAST) != 0) && !result) {
                    fail_now = true;
                    break;
                }
                
                // std::cout << "2. Compose R in left_rest with R' in right_new" << std::endl;
                // 2. Compose R in left_rest with R' in right_new
                for (auto it = left_rest; it != Ccl[source][middle]->end(); it++) {
                    Relation_LIST* left_old = *it;
                    result = compose_lists(Ccl[source][sink],
                                           left_old, right_new, 
                                           source, sink, *(HeightsOf.at(source)),
                                           rejected, data, opts);
                    if (((opts & FAIL_FAST) != 0) && !result) {
                        fail_now = true;
                        break;
                    }
                }
                if (fail_now) { break; }

                // std::cout << "3. Compose R in left_new with R' in right_rest" << std::endl;
                // 3. Compose R in left_new with R' in right_rest
                for (auto it = right_rest; it != Ccl[middle][sink]->end(); it++) {
                    Relation_LIST* right_old = *it;
                    result = compose_lists(Ccl[source][sink],
                                           left_new, right_old, 
                                           source, sink, *(HeightsOf.at(source)),
                                           rejected, data, opts);
                    if (((opts & FAIL_FAST) != 0) && !result) {
                        fail_now = true;
                        break;
                    }
                }
                if (fail_now) { break; }

            }

            // Extract performance metrics and store in fields
            ccl_size += data.additions;
            compositions += data.compositions;
            comparisons += data.comparisons;
            ccl_rejections += data.rejections;
            ccl_replacements += data.replacements;
            loop_checks = data.loop_checks;
            checked_size_sum += data.checked_size_sum;
            compose_time += data.compose_time;
            compare_time += data.compare_time;
            loop_check_time += data.loop_check_time;

            if (fail_now) { return false; }

            if (data.additions > 0) { done = false; }
        }
        }
    }

    // If not using fast-fail, then check for self-loops in the computed CCL
    if ((opts & FAIL_FAST) == 0) {
        if (!check_Ccl(opts)) {
            return false;
        }
    }

    return true;
}

// Could add parallelism/concurrency here
bool Heighted_graph::check_Ccl(int opts) {
    bool result = true;
    int num_nodes = this->num_nodes();
    for (int node = 0; node < num_nodes; node++) {
        Relation_LIST_LIST* Ccl_nd = Ccl[node][node];
        ccl_data data;
        for (Relation_LIST* Rs : *Ccl_nd) {
            for (Sloped_relation* R : *Rs) {
                if(!check_self_loop(*R, node, *(HeightsOf.at(node)), data, opts)) {
                    result = false;
                    break;
                }
            }
            if (!result) break;
        }
        loop_checks += data.loop_checks;
        checked_size_sum += data.checked_size_sum;
        loop_check_time += data.loop_check_time;
        if (!result) break;
    }
    return result;
}

void Heighted_graph::print_Ccl(void){
    int num_nodes = this->num_nodes();
    int num_rels = 0;
    for (int source = 0; source < num_nodes; source++) {
    for (int sink = 0; sink < num_nodes; sink++) {
        for (Relation_LIST* rels : *Ccl[source][sink]) {
        for (Sloped_relation* R : *rels) {
            num_rels++;
            std::cout << "<><><><><><><><><><><><><><><><><><><><><><><><><><><><><>" << std::endl;
            std::cout << source << " " << sink << std::endl;
            std::cout << *R;
            std::cout << "<><><><><><><><><><><><><><><><><><><><><><><><><><><><><>" << std::endl;
        }
        }
    }
    }
    std::cout << "Size of CCL: " << num_rels << std::endl;
}

void Heighted_graph::print_statistics(void) {
    std::cout << "Initial CCL size: " << ccl_initial_size << std::endl;
    std::cout << "Final CCL size: " << ccl_size << std::endl;
    std::cout << "CCL Rejections: " << ccl_rejections << std::endl;
    std::cout << "CCL Replacements: " << ccl_replacements << std::endl;
    std::cout << "Sloped relations computed: " << compositions << std::endl;
    std::cout << "Time spent computing sloped relations (ms): "
              << compose_time.count() << std::endl;
    std::cout << "Sloped relations compared: " << comparisons << std::endl;
    std::cout << "Time spent comparing sloped relations (ms): "
              << compare_time.count() << std::endl;
    std::cout << "Number of self-loop checks: " << loop_checks << std::endl;
    std::cout << "Time spent loop checking (ms): "
              << loop_check_time.count() << std::endl;
    if (loop_checks > 0)
        std::cout << "Average size of loop-checked sloped relations: "
                << (checked_size_sum / loop_checks) << std::endl;
}
