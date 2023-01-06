#include <cassert>
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
#include <iostream>
#include <sstream>

extern "C" {
#include <memory.h>
#include <mlvalues.h>
#include <alloc.h>
}

#include "proof_aut.hpp"
#include "trace.hpp"

#include <spot/twaalgos/hoa.hh>
#include <spot/twa/twagraph.hh>



static std::shared_ptr<ProofAutomaton> proof = 0;
static std::map< int, Vertex > bdd_map;

extern "C" void create_aut(value max_log2_states) {
	CAMLparam1(max_log2_states);
//	std::cerr << "create_aut " << Int_val(max_log2_states) << '\n';
	assert(proof==0);
	proof = std::make_shared<ProofAutomaton>(Int_val(max_log2_states));
	CAMLreturn0;
}

extern "C" void destroy_aut() {
	CAMLparam0();
//	std::cerr << "destroy_aut\n";
	assert(proof);
	proof = 0;
	bdd_map.clear();
	CAMLreturn0;
}

//extern "C" value create_tag() {
//	CAMLparam0();
//	CAMLlocal1(v_res);
//	assert(proof);
//	v_res = Val_int(proof->create_tag());
//	CAMLreturn(v_res);
//}

extern "C" void create_vertex(value v_) {
	CAMLparam1(v_);
	assert(proof);
	Vertex v = proof->create_vertex();
	int id = Int_val(v_); //v.id();
	assert( bdd_map.find(id) == bdd_map.end() );
	bdd_map[id] = v;
//	std:: cerr << "create_vertex " << id << '\n';
	CAMLreturn0;
}

extern "C" void tag_vertex(value v_, value t_) {
	CAMLparam2(v_, t_);
	int v = Int_val(v_);
	Tag t = Int_val(t_);
	assert(proof);
	assert( bdd_map.find(v) != bdd_map.end() );
	proof->tag_vertex(bdd_map[v], t);
	CAMLreturn0;
}

extern "C" void set_successor(value v1_, value v2_) {
	CAMLparam2(v1_, v2_);
	int v1 = Int_val(v1_);
	int v2 = Int_val(v2_);
	assert(proof);
	assert( bdd_map.find(v1) != bdd_map.end() );
	assert( bdd_map.find(v2) != bdd_map.end() );
	proof->set_successor(bdd_map[v1], bdd_map[v2]);
	CAMLreturn0;
}

extern "C" void set_trace_pair(value v1_, value v2_, value t1_, value t2_) {
	CAMLparam4(v1_, v2_, t1_, t2_);
	int v1 = Int_val(v1_);
	int v2 = Int_val(v2_);
	Tag t1 = Int_val(t1_);
	Tag t2 = Int_val(t2_);
	assert(proof);
	assert( bdd_map.find(v1) != bdd_map.end() );
	assert( bdd_map.find(v2) != bdd_map.end() );
	proof->set_trace_pair(bdd_map[v1], bdd_map[v2], t1, t2);
	CAMLreturn0;
}

extern "C" void set_progress_pair(value v1_, value v2_, value t1_, value t2_) {
	CAMLparam4(v1_, v2_, t1_, t2_);
	int v1 = Int_val(v1_);
	int v2 = Int_val(v2_);
	Tag t1 = Int_val(t1_);
	Tag t2 = Int_val(t2_);
	assert(proof);
	assert( bdd_map.find(v1) != bdd_map.end() );
	assert( bdd_map.find(v2) != bdd_map.end() );
	proof->set_progress_pair(bdd_map[v1], bdd_map[v2], t1, t2);
	CAMLreturn0;
}

extern "C" value get_proof_aut_hoa() {
	CAMLparam0();

	assert(proof);
	std::stringstream res;
	spot::print_hoa(res, proof);

	return caml_copy_string(res.str().c_str());
}

extern "C" value get_trace_aut_hoa() {
	CAMLparam0();

	assert(proof);
	spot::const_twa_ptr ta = std::make_shared<TraceAutomaton>(*proof);
	std::stringstream res;
	spot::print_hoa(res, ta);

	return caml_copy_string(res.str().c_str());
}

extern "C" value check_soundness_buchi() {
	CAMLparam0();
	CAMLlocal1(v_res);

	spot::const_twa_ptr ta = std::make_shared<TraceAutomaton>(*proof);
	spot::twa_graph_ptr graph = make_twa_graph(ta, spot::twa::prop_set::all());
	spot::twa_graph_ptr prf = make_twa_graph(proof, spot::twa::prop_set::all());
	bool retval = spot::contains(graph, prf);

	//std:: cout << "retval " << retval << '\n';

	v_res = Val_bool(retval);
	CAMLreturn(v_res);
}

extern "C" void set_initial_vertex(value v_) {
	CAMLparam1(v_);
	int v = Int_val(v_);
//	std::cerr << "set_initial_vertex " << v << '\n';
	assert(proof);
	assert( bdd_map.find(v) != bdd_map.end() );
	proof->set_initial_vertex( bdd_map[v] );
	CAMLreturn0;
}


