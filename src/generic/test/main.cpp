#include <iostream>
#include "json.hpp"
#include "../types.c"
#include "../heighted_graph.hpp"
#include <spot/twaalgos/hoa.hh>
#include <spot/twa/twagraph.hh>

#include <iostream>

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
    if(argc > 1){
        std::string path = std::string("./data/");
        path = path + argv[1] + ".json";
        int opts = 0;
        if(const char* flags = std::getenv("FLAGS")) {
            opts = Heighted_graph::parse_flags(flags);
        }
        // Get JSON data
        std::ifstream graph_data(path.c_str());
        json graph;
        graph_data >> graph;

        // Create heighted graph object
        Heighted_graph hg = Heighted_graph(graph["Node"].size());
        parse_from_json(graph, hg);
        bool result;
        if ((opts & Heighted_graph::USE_SD) != 0) {
            result = hg.sd_check();
        } else if ((opts & Heighted_graph::USE_XSD) != 0) {
            result = hg.xsd_check();
        } else {
            result = hg.relational_check(opts);
            // hg.print_Ccl();
            hg.print_statistics();
        }
        if( result ){
            std::cout << "SOUND" << std::endl;
        }
        else {
            std::cout << "UNSOUND" << std::endl;
        }
        return !result;
    }

    std::cout << "Provide test file name!\n";
    return -1;

}