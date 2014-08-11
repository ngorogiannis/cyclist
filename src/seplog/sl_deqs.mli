(** Sets of disequalities over terms. *)
(** it is guaranteed that for any pair (x,y) in the set, x<=y re [Sl_term.compare].*)

include Util.OrderedContainer with type elt = Sl_tpair.t

val parse : (Sl_tpair.t, 'a) MParser.parser
val subst : Sl_term.substitution -> t -> t

val terms : t -> Sl_term.Set.t
val vars : t -> Sl_term.Set.t

val to_string_list : t -> string list
val to_melt : t -> Latex.t

val unify_partial : ?inverse:bool -> t Sl_term.unifier
(** [unify_partial Option.some (Sl_term.empty_subst, ()) d d'] computes a 
    substitution [theta] such that [d[theta]] is a subset of [d']. 
    If the optional argument [~inverse:false] is set to [true] then a 
    substitution is computed such that [d] is a subset of [d'[theta]]. *)

val subsumed : Sl_uf.t -> t -> t -> bool
(** [subsumed eqs d d'] is true iff [d] can be rewritten using the equalities
    in [eqs] such that it becomes a subset of [d']. *)
