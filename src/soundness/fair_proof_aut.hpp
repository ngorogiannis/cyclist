#ifndef FAIR_PROOF_AUTOMATON_HH_
#define FAIR_PROOF_AUTOMATON_HH_

#include <string>

#include <spot/twa/twa.hh>

#include "fair_proof.hpp"

//==================================================================
class FairProofState: public spot::state {
public:
  const Vertex vertex;
  const TagVector & tags;
  
  FairProofState(const Vertex & v, const TagVector & ts) : vertex(v), tags(ts) {}
  
  virtual int compare(const spot::state* other) const;
  virtual size_t hash() const { return vertex.id(); }
  virtual spot::state* clone() const { return new FairProofState(vertex, tags); }
};
//==================================================================
class FairProofGhostState: public spot::state {
public:
  virtual int compare(const spot::state* other) const;
  virtual size_t hash() const { return 0; }
  virtual spot::state* clone() const { return new FairProofGhostState(); }
};
//==================================================================
class FairProofAutomaton: public spot::twa, public FairProof {
private:
  std::string acceptance_condition_builder() const;
public:
  FairProofAutomaton(size_t max_vertices_log2) : FairProof(max_vertices_log2), spot::twa(spot::make_bdd_dict()) {
    this->dict_ = FairProof::get_dict();
    register_aps_from_dict();
  }

  void set_acceptance_condition(); 
  
  virtual ~FairProofAutomaton() {};
  virtual spot::state* get_init_state() const { return new FairProofGhostState(); }
  virtual spot::bdd_dict_ptr get_dict() const { return FairProof::get_dict(); }
  virtual spot::twa_succ_iterator* succ_iter(const spot::state* local_state) const;
  virtual std::string format_state(const spot::state* state) const;
  //	virtual std::string transition_annotation(const spot::tgba_succ_iterator* t) const;
  //	virtual spot::state* project_state(const spot::state* s, const spot::tgba* t) const;
};
//==================================================================
class FairProofGhostSuccIterator: public spot::twa_succ_iterator {
private:
  const FairProofAutomaton & proof;
  bool finished;
  
public:
  FairProofGhostSuccIterator(const FairProofAutomaton & p) : proof(p), finished(false) {}
  
  virtual bool first() { finished = false; return !done(); }
  virtual bool next() { finished = true; return !done(); }
  virtual bool done() const { return finished; }
  virtual spot::state* dst() const {
    Vertex v = proof.get_initial_vertex();
    return new FairProofState(v, proof.get_tags_of_vertex(v));
  }
  virtual bdd cond() const { return proof.get_initial_vertex(); }
  virtual spot::acc_cond::mark_t acc() const { return spot::acc_cond::mark_t(); } // TODO: make ghost state not have accepting sets
};
//==================================================================
class FairProofSuccIterator: public spot::twa_succ_iterator {
private:
  const FairProofAutomaton & proof;
  Vertex vertex;
  VertexSet::const_iterator successor;

public:
  FairProofSuccIterator(const FairProofAutomaton & p, const Vertex & v) : proof(p), vertex(v) {}

  virtual bool first() { successor = proof.get_successors(vertex).begin(); return !done(); }
  virtual bool next() { ++successor; return !done(); }
  virtual bool done() const { return successor == proof.get_successors(vertex).end(); }
  virtual spot::state* dst() const {
    return new FairProofState(*successor, proof.get_tags_of_vertex(*successor));
  }
  virtual bdd cond() const { return *successor; }
  virtual spot::acc_cond::mark_t acc() const; 
};
//==================================================================
#endif /* FAIR_PROOF_AUTOMATON_HH_ */
