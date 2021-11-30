(** Provides an abstract view of a proof as a graph and allows checking its 
    soundness. *)

open Lib

(** Abstract proof node type. The only 
    information stored is a set of tags (integers) and a list of
    tuples of: successor, set of valid tag transitions and set of
    progressing tag transitions. *)
type abstract_node

val use_spot : bool ref
(** Flag to indicate whether the Spot model checker should be used to verify the
    trace condition for proofs. *)

val use_external : bool ref
(** Flag to indicate whether trace condition check should be done by external
    C++ code. *)

val fail_fast : unit -> unit
(** Set fail_fast flag for external code checking trace condition *)
val use_scc_check : unit -> unit
(** Set use_scc_check flag for external code checking trace condition *)
val use_idempotence : unit -> unit
(** Set use_idempotence flag for external code checking trace condition *)
val use_minimality : unit -> unit
(** Set use_minimality flag for external code checking trace condition *)

val mk_abs_node :
  Tags.t -> int list -> (Tagpairs.t * Tagpairs.t) list -> abstract_node
(** Constructor for nodes. *)

(** The type of abstracted proof as a map from ints to nodes. 
    NB the root is always at 0. *)
type t = abstract_node Int.Map.t

val build_proof :
  (int * int list * (int * (int * int) list * (int * int) list) list) list -> t
(* Make an abstract proof from a representation using integers for node and tag
   IDs. The representation consists of a list of tuples, each representing one
   node of the proof and consisting of:
   
     - a node ID
     - a list of the tag IDs belonging to that node
     - a list of tuples representing the successors of the node, each consisting
       of:
         # the ID of the successor
         # a list of valid tag transitions
         # a list of the progressing tag transitions
 *)

val check_proof : ?init:int -> ?minimize:bool -> t -> bool
(** Validate, minimise, check soundness of proof/graph and memoise. *)

val pp : Format.formatter -> t -> unit
(** Pretty print abstract proof. *)
