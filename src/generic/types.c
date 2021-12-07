
#ifndef TYPES_HH_
#define TYPES_HH_

#include <set>
#include <map>
#include <list>

#define Map                std::map
#define Pair               std::pair
#define Int_SET            std::set<int>
#define Int_pair           std::pair<int,int>
#define Int_pair_SET       std::set<std::pair<int,int>>
#define Relation_LIST      std::list<Sloped_relation*>
#define Relation_LIST_LIST std::list<Relation_LIST*>
#define CCL_ITER           Relation_LIST_LIST::iterator

enum slope {
  Undef = -1,
  Stay = 0,
  Downward = 1
};

/* Enumeration for comparing sloped relations */
enum comparison { 
  lt,     // Less
  eq,     // Equal
  gt,     // Greater 
  noncomp // Incomparable
};

#endif