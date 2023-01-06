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
#include <iostream>
#include <sstream>

extern "C" {
#include <memory.h>
#include <mlvalues.h>
}

#include "heighted_graph.hpp"

#include <spot/twaalgos/hoa.hh>
#include <spot/twa/twagraph.hh>



static std::shared_ptr<Heighted_graph> hg_0 = 0;
static std::shared_ptr<Heighted_graph> hg_1 = 0;
static std::shared_ptr<Heighted_graph> hg_2 = 0;

extern "C" void create_hgraph(value max_nodes_) {
  CAMLparam1(max_nodes_);
  assert(hg_0 == 0);
  assert(hg_1 == 0);
  assert(hg_2 == 0);
  int max_nodes = Int_val(max_nodes_);
  hg_0 = std::make_shared<Heighted_graph>(max_nodes);
  hg_1 = std::make_shared<Heighted_graph>(max_nodes);
  hg_2 = std::make_shared<Heighted_graph>(max_nodes);
  CAMLreturn0;
}

extern "C" void destroy_hgraph() {
  CAMLparam0();
  assert(hg_0);
  assert(hg_1);
  assert(hg_2);
  hg_0 = 0;
  hg_1 = 0;
  hg_2 = 0;
  CAMLreturn0;
}

extern "C" void add_node(value n_) {
  CAMLparam1(n_);
  assert(hg_0);
  assert(hg_1);
  assert(hg_2);
  int n = Int_val(n_);
  hg_0->add_node(n);
  hg_1->add_node(n);
  hg_2->add_node(n);
  CAMLreturn0;
}

extern "C" void add_height(value n_, value h_) {
  CAMLparam2(n_, h_);
  assert(hg_0);
  assert(hg_1);
  assert(hg_2);
  int n = Int_val(n_);
  int h = Int_val(h_);
  hg_0->add_height(n, h);
  hg_1->add_height(n, h);
  hg_2->add_height(n, h);
  CAMLreturn0;
}

extern "C" void add_edge(value n1_, value n2_) {
  CAMLparam2(n1_, n2_);
  assert(hg_0);
  assert(hg_1);
  assert(hg_2);
  int n1 = Int_val(n1_);
  int n2 = Int_val(n2_);
  hg_0->add_edge(n1, n2);
  hg_1->add_edge(n1, n2);
  hg_2->add_edge(n1, n2);
  CAMLreturn0;
}

extern "C" void add_stay(value src_, value src_h_, value snk_, value snk_h_) {
  CAMLparam4(src_, src_h_, snk_, snk_h_);
  assert(hg_0);
  assert(hg_1);
  assert(hg_2);
  int src = Int_val(src_);
  int src_h = Int_val(src_h_);
  int snk = Int_val(snk_);
  int snk_h = Int_val(snk_h_);
  hg_0->add_stay(src, src_h, snk, snk_h);
  hg_1->add_stay(src, src_h, snk, snk_h);
  hg_2->add_stay(src, src_h, snk, snk_h);
  CAMLreturn0;
}

extern "C" void add_decr(value src_, value src_h_, value snk_, value snk_h_) {
  CAMLparam4(src_, src_h_, snk_, snk_h_);
  assert(hg_0);
  assert(hg_1);
  assert(hg_2);
  int src = Int_val(src_);
  int src_h = Int_val(src_h_);
  int snk = Int_val(snk_);
  int snk_h = Int_val(snk_h_);
  hg_0->add_decrease(src, src_h, snk, snk_h);
  hg_1->add_decrease(src, src_h, snk, snk_h);
  hg_2->add_decrease(src, src_h, snk, snk_h);
  CAMLreturn0;
}

extern "C" void print_ccl() {
  CAMLparam0();
  assert(hg_0);
  hg_0->print_Ccl();
  CAMLreturn0;
}

extern "C" void print_statistics() {
  CAMLparam0();
  assert(hg_0);
  hg_0->print_statistics();
  CAMLreturn0;
}

extern "C" value relational_check(value opts_) {
  CAMLparam1(opts_);
  CAMLlocal1(v_res);

  assert(hg_0);
  assert(hg_1);
  assert(hg_2);
  int opts = Int_val(opts_);

  bool retval = (hg_0->thors_hammer(opts,hg_1.get(),hg_2.get()));


  // bool retval = (hg->relational_check(opts));

  v_res = Val_bool(retval);
  CAMLreturn(v_res);
}

extern "C" value sd_check() {
  CAMLparam0();
  CAMLlocal1(v_res);
  assert(hg_0);
  assert(hg_1);
  assert(hg_2);
  bool retval = (hg_0->sd_check());
  v_res = Val_bool(retval);
  CAMLreturn(v_res);
}

extern "C" value xsd_check() {
  CAMLparam0();
  CAMLlocal1(v_res);
  assert(hg_0);
  bool retval = (hg_0->xsd_check());
  v_res = Val_bool(retval);
  CAMLreturn(v_res);
}


extern "C" value test(){
	CAMLparam0();

	
  spot::bdd_dict_ptr dict = spot::make_bdd_dict();
  spot::twa_graph_ptr aut = make_twa_graph(dict);
  aut->set_buchi();

  


  bdd p1 = bdd_ithvar(aut->register_ap("a"));
  bdd p2 = bdd_ithvar(aut->register_ap("b"));

  

  

  aut->new_states(2);
  aut->set_init_state(0U);

  aut->new_edge(0, 1, p1);
  aut->new_edge(1, 1, p1 & p2, {0});
  aut->new_edge(1, 2, p2, {0});
  aut->new_edge(2, 1, p1 | p2, {0});

  // Print the resulting automaton.
  print_dot(std::cout, aut);
  
	
	CAMLlocal1(v_res);
	v_res = Val_bool(true);
	CAMLreturn(v_res);

}
