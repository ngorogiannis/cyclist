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

extern "C" {
#include <memory.h>
#include <mlvalues.h>
#include <alloc.h>
}

#include "proof_aut.hpp"
#include "trace.hpp"
#include "heighted_graph.hpp"
#include "automata.hpp"

static std::shared_ptr<Automata> aut = 0;




//=============================================================================


extern "C" void create_auto_new(value max_nodes_) {
  CAMLparam1(max_nodes_);
  assert(aut == 0);
  int max_nodes = Int_val(max_nodes_);
  aut = std::make_shared<Automata>(max_nodes);
  CAMLreturn0;
}

extern "C" void destroy_auto_new() {
  CAMLparam0();
  assert(aut);
  // assert(hg_1);
  // assert(hg_2);
  aut = 0;
  // hg_1 = 0;
  // hg_2 = 0;
  CAMLreturn0;
}

extern "C" void add_node_auto_new(value n_) {
  CAMLparam1(n_);
//   assert(hg_0);
//   assert(hg_1);
//   assert(hg_2);
//   int n = Int_val(n_);
//   hg_0->add_node(n);
//   hg_1->add_node(n);
//   hg_2->add_node(n);
  CAMLreturn0;
}

extern "C" void add_height_auto_new(value n_, value h_) {
  CAMLparam2(n_, h_);
  assert(aut);
//   assert(hg_1);
//   assert(hg_2);
  int n = Int_val(n_);
  int h = Int_val(h_);
  aut->add_height(n, h);
//   hg_1->add_height(n, h);
//   hg_2->add_height(n, h);
  CAMLreturn0;
}

extern "C" void add_edge_auto_new(value n1_, value n2_) {
  CAMLparam2(n1_, n2_);
  assert(aut);
//   assert(hg_1);
//   assert(hg_2);
  int n1 = Int_val(n1_);
  int n2 = Int_val(n2_);
  aut->add_edge(n1, n2);
//   hg_1->add_edge(n1, n2);
//   hg_2->add_edge(n1, n2);
  CAMLreturn0;
}

extern "C" void add_decr_auto_new(value src_, value src_h_, value snk_, value snk_h_) {
  CAMLparam4(src_, src_h_, snk_, snk_h_);
  assert(aut);
  // assert(hg_1);
  // assert(hg_2);
  int src = Int_val(src_);
  int src_h = Int_val(src_h_);
  int snk = Int_val(snk_);
  int snk_h = Int_val(snk_h_);
  aut->add_decr(src, src_h, snk, snk_h);
  // hg_1->add_stay(src, src_h, snk, snk_h);
  // hg_2->add_stay(src, src_h, snk, snk_h);
  CAMLreturn0;
}


extern "C" void add_stay_auto_new(value src_, value src_h_, value snk_, value snk_h_) {
  CAMLparam4(src_, src_h_, snk_, snk_h_);
  assert(aut);
  // assert(hg_1);
  // assert(hg_2);
  int src = Int_val(src_);
  int src_h = Int_val(src_h_);
  int snk = Int_val(snk_);
  int snk_h = Int_val(snk_h_);
  // hg_0->add_stay(src, src_h, snk, snk_h);
  aut->add_stay(src, src_h, snk, snk_h);
  // hg_2->add_stay(src, src_h, snk, snk_h);
  CAMLreturn0;
}

extern "C" value test_auto_new() {
	CAMLparam0();
	CAMLlocal1(v_res);

	// spot::const_twa_ptr ta = std::make_shared<TraceAutomaton>(*proof);
	// spot::twa_graph_ptr graph = make_twa_graph(ta, spot::twa::prop_set::all());
	// spot::twa_graph_ptr prf = make_twa_graph(proof, spot::twa::prop_set::all());
	// bool retval = spot::contains(graph, prf);

	//std:: cout << "retval " << retval << '\n';

	v_res = Val_bool(aut->check_soundness());
	CAMLreturn(v_res);
}
