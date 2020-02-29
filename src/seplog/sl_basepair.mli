open Lib

module AllocatedT : BasicType with type t = Sl_term.t * int

module Allocated : OrderedContainer with type elt = AllocatedT.t

include BasicType with type t = Allocated.t * Sl_heap.t

module Set : OrderedContainer with type elt = t

module Hashset : Hashset.S with type elt = t

val vars : t -> Sl_term.Set.t

val satisfiable : ?only_first:bool -> ?output:bool -> Sl_defs.t -> bool
(** See [gen_all_pairs] regarding [~only_first]. 

    When the optional argument [~output=false] is set to [true] then the
    result is reported on stdout.  FIXME this should be moved to the caller. *)

val form_sat : Sl_defs.t -> Sl_form.t -> bool
(** Decide the satisfiability of a formula with predicates out of the definitions
    provided, using the base pairs algorithm. *)

val pairs_of_form : Sl_defs.t -> Sl_form.t -> Set.t
(** Convert the formula into a inductive rules, append those to the given
    definitions, generate all base pairs up to the fixpoint and then return
    all base pairs generated by the (inductive rules corresponding to the)
    formula. *)

val leq : t -> t -> bool
(** Ordering relation on base pairs. [leq (v,h) (v',h')] is true iff  [h' |- h]
    and [h'(v)] is a subset of [h'(v')] where [h(.)] represents rewriting to
    fixed representatives of equivalence classes induced by [h]. *)

val minimise : Set.t -> Set.t
