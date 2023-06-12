#include <iostream>
#include "json.hpp"
#include "../types.c"
#include "../heighted_graph.hpp"
#include <spot/twaalgos/hoa.hh>
#include <spot/twa/twagraph.hh>


#include <iostream>
#include "automata.hpp"

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

using json = nlohmann::json;

void parse_from_json(json &graph, Heighted_graph &hg){
    //====================== parse nodes
    std::cout << "Parsing Nodes: ";
    for( auto& element : graph["Node"] ){
        int id = element[0];
        for( int h : element[1] ){
            hg.add_height(id, h);
        }
    }
    std::cout << "Done! , Node size: " << hg.num_nodes() << std::endl;

    //====================== parse edges
    std::cout << "Parsing Edges: ";
    for( auto& element : graph["Edge"] ){
        int source = element[0][0];
        int sink = element[0][1];
        hg.add_edge(source, sink);
        for( auto& truple : element[1]){
            int source_h = truple[0];
            int sink_h = truple[1];
            slope s = static_cast<slope>(truple[2]);
            hg.add_hchange(source, source_h, sink, sink_h, s);
        }
    }
    std::cout << "Done! , Edge size: " << hg.num_edges() << std::endl;
}

int main(int argc, char** argv) {
    // if(argc > 1){
    //     std::string path = std::string("./data/");
    //     path = path + argv[1] + ".json";
    //     int opts = 0;
    //     if(const char* flags = std::getenv("FLAGS")) {
    //         opts = Heighted_graph::parse_flags(flags);
    //     }
    //     // Get JSON data
    //     std::ifstream graph_data(path.c_str());
    //     json graph;
    //     graph_data >> graph;

    //     // Create heighted graph object
    //     Heighted_graph hg = Heighted_graph(graph["Node"].size());
    //     parse_from_json(graph, hg);
    //     bool result;
    //     if ((opts & Heighted_graph::USE_SD) != 0) {
    //         result = hg.sd_check();
    //     } else if ((opts & Heighted_graph::USE_XSD) != 0) {
    //         result = hg.xsd_check();
    //     } else {
    //         result = hg.relational_check(opts);
    //         // hg.print_Ccl();
    //         hg.print_statistics();
    //     }
    //     if( result ){
    //         std::cout << "SOUND" << std::endl;
    //     }
    //     else {
    //         std::cout << "UNSOUND" << std::endl;
    //     }
    //     return !result;
    // }

    // std::cout << "Provide test file name!\n";

     // The bdd_dict is used to maintain the correspondence between the
  // atomic propositions and the BDD variables that label the edges of
  // the automaton.
  spot::bdd_dict_ptr dict = spot::make_bdd_dict();
  // This creates an empty automaton that we have yet to fill.
  spot::twa_graph_ptr aut = make_twa_graph(dict);

//   // Since a BDD is associated to every atomic proposition, the
//   // register_ap() function returns a BDD variable number
//   // that can be converted into a BDD using bdd_ithvar().
//   bdd p1 = bdd_ithvar(aut->register_ap("p1"));
//   bdd p2 = bdd_ithvar(aut->register_ap("p2"));

//   // Set the acceptance condition of the automaton to Inf(0)&Inf(1)
//   aut->set_generalized_buchi(2);

//   // States are numbered from 0.
//   aut->new_states(3);
//   // The default initial state is 0, but it is always better to
//   // specify it explicitly.
//   aut->set_init_state(0U);

//   // new_edge() takes 3 mandatory parameters: source state,
//   // destination state, and label.  A last optional parameter can be
//   // used to specify membership to acceptance sets.
//   aut->new_edge(0, 1, p1);
//   aut->new_edge(1, 1, p1 & p2, {0});
//   aut->new_edge(1, 2, p2, {1});
//   aut->new_edge(2, 1, p1 | p2, {0, 1});

//   // Print the resulting automaton.
//   print_hoa(std::cout, aut);
    return -1;

}