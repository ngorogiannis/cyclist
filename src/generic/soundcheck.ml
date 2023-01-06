open Lib
open Parsers

type soundness_method =
  | SPOT
  | RELATIONAL_OCAML
  | RELATIONAL_CPP
  | SD_CPP
  | XSD_CPP
  | SLEDGE_HAMMER
  | SPOT_NEW


let soundness_method = ref RELATIONAL_OCAML
let use_spot () = 
  soundness_method := SPOT
let use_external () = 
  soundness_method := RELATIONAL_CPP
let use_sprengerdam () =
  soundness_method := SD_CPP
let use_xtd_sprengerdam () =
  soundness_method := XSD_CPP

let use_sledge_hammer () = 
  soundness_method := SLEDGE_HAMMER
  
let use_spot_new () = 
  soundness_method := SPOT_NEW
module IntPair = struct
  include Pair.Make(Int)(Int)
  include Containers.Make(Pair.Make(Int)(Int))
end

(* computes the composition of two sets of pairs *)
let compose t1 t2 =
  IntPair.Set.fold
    (fun (x, y) a ->
      IntPair.Set.fold
        (fun (w, z) b -> if Int.equal y w then IntPair.Set.add (x, z) b else b)
        t2 a )
    t1 IntPair.Set.empty

(* An abstract node is a set of tags, and a list of successor node IDs along
   with, for each successor, a list of the tag pairs and strictly progressing
   tag pairs *)
type abstract_node = Int.Set.t * (int * IntPair.Set.t * IntPair.Set.t) list

(* An abstract proof is a map from node IDs to abstract nodes *)
type t = abstract_node Int.Map.t

let get_tags n = fst n

let get_subg n = snd n

let is_leaf n = Blist.is_empty (get_subg n)

let in_children child n =
  Blist.exists (fun (i, _, _) -> Int.equal i child) (get_subg n)

let index_of_child child (_, subg) =
  Blist.find_index (fun (i, _, _) -> Int.equal i child) subg

let mk_abs_node tags succs tps_pair =
  let tags = Tags.to_ints tags in
  let subg =
    List.map2
      (fun i (tps, tps') ->
        let tps, tps' =
          Pair.map
            (Tagpairs.map_to IntPair.Set.add IntPair.Set.empty
               (Pair.map Tags.Elt.to_int))
            (tps, tps')
        in
        (i, tps, tps') )
      succs tps_pair
  in
  (tags, subg)

let build_proof nodes =
  Int.Map.of_list
    (List.rev_map
       (fun (id, tags, premises) ->
         let premises =
           List.rev_map
             (fun (target, allpairs, progpairs) ->
               ( target
               , IntPair.Set.of_list allpairs
               , IntPair.Set.of_list progpairs ) )
             premises
         in
         (id, (Int.Set.of_list tags, List.rev premises)) )
       nodes)

(* has one child and is not a self loop *)
let is_single_node idx n =
  match get_subg n with
  | [(idx', _, _)] -> not (Int.equal idx' idx)
  | _ -> false

let fathers_grandchild prf idx n =
  match get_subg n with
  | [(grandchild, _, _)] ->
      if Int.equal idx grandchild then invalid_arg "fathers_grandchild1"
      else
        Int.Map.exists
          (fun idx' par_node ->
            in_children idx par_node && in_children grandchild par_node )
          prf
  | _ -> invalid_arg "fathers_grandchild2"

let pp_proof_node fmt n =
  let aux fmt (tags, subg) =
    Format.fprintf fmt "tags=%a " Int.Set.pp tags ;
    if Blist.is_empty subg then Format.pp_print_string fmt "leaf"
    else
      Blist.pp pp_semicolonsp
        (fun fmt (i, tv, tp) ->
          Format.fprintf fmt "(goal=%a, valid=%a, prog=%a)" Format.pp_print_int
            i IntPair.Set.pp tv IntPair.Set.pp tp )
        fmt subg
  in
  Format.fprintf fmt "@[%a@]" aux n

let pp fmt prf =
  Format.open_vbox 0 ;
  Int.Map.iter
    (fun idx n -> Format.fprintf fmt "%i: %a@\n" idx pp_proof_node n)
    prf ;
  Format.close_box ()

let remove_dead_nodes prf' =
  let prf = ref prf' in
  let process_node child par_idx n =
    if not (in_children child n) then ()
    else
      let newparent =
        let tags, subg = n in
        match subg with
        | [_] ->
          (tags, [])
        | _ -> 
          (tags, Blist.remove_nth (index_of_child child n) subg)
      in
      prf := Int.Map.add par_idx newparent !prf
  in
  let remove_dead_node idx n =
    let () = prf := Int.Map.remove idx !prf in
    Int.Map.iter (fun p n -> process_node idx p n) !prf
  in
  let cont = ref true in
  while !cont do
    match
      Int.Map.find_map (fun idx n -> (not (Int.equal idx 0)) && is_leaf n) !prf
    with
    | Some (idx, n) -> remove_dead_node idx n
    | None -> cont := false
  done ;
  !prf

let fuse_single_nodes prf' init =
  let prf = ref prf' in
  let process_node child grand_child tv tp par_idx n =
    if not (in_children child n) then ()
    else
      let par_tags, par_subg = n in
      let pos = index_of_child child n in
      let _, par_tv, par_tp = List.nth par_subg pos in
      let newsubg =
        Blist.replace_nth
          ( grand_child
          , compose par_tv tv
          , IntPair.Set.union_of_list
              [compose par_tp tp; compose par_tv tp; compose par_tp tv] )
          pos par_subg
      in
      prf := Int.Map.add par_idx (par_tags, newsubg) !prf
  in
  let fuse_node idx = function
    | tags, [(child, tv, tp)] ->
        Int.Map.iter (fun p n -> process_node idx child tv tp p n) !prf ;
        prf := Int.Map.remove idx !prf
    | _ -> invalid_arg "fuse_node"
  in
  let cont = ref true in
  (* if a parent points to the child of the node to be fused then *)
  (* we would run into difficulties when updating that parent to point *)
  (* directly to the grandchild, so we avoid that altogether *)
  let p idx n =
    (not (Int.equal idx init))
    && is_single_node idx n
    && not (fathers_grandchild !prf idx n)
  in
  while !cont do
    match Int.Map.find_map p !prf with
    | Some (idx, n) -> fuse_node idx n
    | None -> cont := false
  done ;
  !prf

let minimize_abs_proof prf init = fuse_single_nodes (remove_dead_nodes prf) init

let valid prf init =
  let projectl = IntPair.Set.map_to Int.Set.add Int.Set.empty Pair.left in
  let projectr = IntPair.Set.map_to Int.Set.add Int.Set.empty Pair.right in
  (* init is a node in the proof *)
  if (not (Int.Map.mem init prf)) then
    let () =
      prerr_endline
        (Printf.sprintf "Initial node %i not in proof" init) in
    false
  else
  (* For all nodes n in the proof *)
  Int.Map.for_all
    (fun n_idx n ->
      (* For all premises i of n *)
      Blist.for_all
        (fun (i, tv, tp) ->
          (* i is a node in the proof *)
          if (not (Int.Map.mem i prf)) then
            let () =
              prerr_endline
                (Printf.sprintf "Goal %i for node %i not in proof" i n_idx) in
            false
          (* progressing tag pairs of i are a subset of all tagpairs of i *)
          else if (not (IntPair.Set.subset tp tv)) then
            let () =
              prerr_endline
                (Printf.sprintf "Prog pairs for goal %i of node %i not contained in all pairs" i n_idx) in
            false
          (* The left-hand components of all tagpairs are in the tagset of n *)
          else if (not (Int.Set.subset (projectl tv) (get_tags n))) then
            let () =
              prerr_endline
                (Printf.sprintf "Source tags of tagpairs for goal %i of node %i not contained in tags of node" i n_idx) in
            false
          (* The right-hand components of all tagpairs are in the tagset of i *)
          else if (not (Int.Set.subset (projectr tv) (get_tags (Int.Map.find i prf)))) then
            let () =
              prerr_endline
                (Printf.sprintf "Target tags of tagpairs for goal %i of node %i not contained in tags of target" i n_idx) in
            false
          else
            true)
        (get_subg n))
    prf

module BuchiCheck = struct

  external create_aut : int -> unit = "create_aut"
  external destroy_aut : unit -> unit = "destroy_aut"
  external create_vertex : int -> unit = "create_vertex"
  external tag_vertex : int -> int -> unit = "tag_vertex"
  external set_successor : int -> int -> unit = "set_successor"
  external set_trace_pair : int -> int -> int -> int -> unit = "set_trace_pair"
  external set_progress_pair : int -> int -> int -> int -> unit = "set_progress_pair"
  external check_soundness : unit -> bool = "check_soundness_buchi"
  external get_paut_hoa : unit -> string = "get_proof_aut_hoa"
  external get_taut_hoa : unit -> string = "get_trace_aut_hoa"
  external set_initial_vertex : int -> unit = "set_initial_vertex"

  let print_paut = ref false
  let print_taut = ref false
  
    (* check global soundness condition on proof *)
  let check_proof ?(init=0) p =
    Stats.MC.call () ;
    let create_tags i n = Int.Set.iter (tag_vertex i) (get_tags n) in
    let create_succs i (_, l) =
      Blist.iter (fun (j, _, _) -> set_successor i j) l
    in
    let create_trace_pairs i (_, l) =
      let do_tag_transitions (j, tvs, tps) =
        IntPair.Set.iter (fun (k, m) -> set_trace_pair i j k m) tvs ;
        IntPair.Set.iter (fun (k, m) -> set_progress_pair i j k m) tps
      in
      Blist.iter do_tag_transitions l
    in
    let size = Int.Map.cardinal p in
    let log2size =
      1 + int_of_float (ceil (log (float_of_int size) /. log 2.0))
    in
    debug (fun () -> "Checking soundness starts...") ;
    create_aut log2size ;
    Int.Map.iter (fun i _ -> create_vertex i) p ;
    Int.Map.iter create_tags p ;
    Int.Map.iter create_succs p ;
    set_initial_vertex init ;
    Int.Map.iter create_trace_pairs p ;
    if !print_paut then begin
      print_endline "Proof Automaton" ;
      print_endline (get_paut_hoa ()) ;
    end ;
    if !print_taut then begin
      print_endline "Trace Automaton" ;
      print_endline (get_taut_hoa ()) ;
    end ;
    let retval = check_soundness () in
    if retval then Stats.MC.accept () else Stats.MC.reject () ;
    destroy_aut () ;
    debug (fun () ->
        "Checking soundness ends, result=" ^ if retval then "OK" else "NOT OK" ) ;
    retval
end

module RelationalCheck = struct

  module Slope = struct

    type t = 
      (* | Unknown *)
      | Stay
      | Decrease

    let equal h h' =
      match (h, h') with
      | Decrease, Decrease
      | Stay, Stay
      (* | Unknown, Unknown  *)
        ->
        true
      | _ ->
        false
    
    let ( < ) h h' =
      match (h, h') with
      (* | Unknown, Stay
      | Unknown, Decrease *)
      | Stay, Decrease ->
        true
      | _, _ ->
        false

    let compare h h' =
      match (h, h') with
      | Stay, Decrease ->
        -1
      | Decrease, Stay ->
        1
      | _ ->
        0

    let max h h' = 
      match (h, h') with
      | Decrease, _
      | _, Decrease ->
        Decrease
      (* | Stay, _ 
      | _, Stay ->
        Stay *)
      | _, _ ->
        (* Unknown *)
        Stay
    
    let pp fmt s =
      Format.fprintf fmt "%s"
        begin match s with
        (* | Unknown -> "Unknown" *)
        | Stay ->
          "Stay"
        | Decrease ->
          "Decrease"
        end
    
    let hash s = Hashtbl.hash s
    
  end

  module SlopedRel = struct

    module HashedKernel = struct
      let collect eq_key s =
        let rec collect prev acc s =
          match s () with
          | Seq.Nil ->
            begin match acc with
            | [] ->
              []
            | (k, vs)::acc ->
              List.rev ((k, List.rev vs)::acc)
            end
          | Seq.Cons ((k, v), s') ->
            begin match prev, acc with
            | None, _ ->
              collect (Some k) ((k, [v])::acc) s'
            | Some k', (_, vs)::acc' ->
              if (eq_key k k') then
                collect (Some k) ((k, v::vs)::acc') s'
              else
                collect (Some k) ((k, [v])::acc) s'
            | _ ->
              failwith "SlopedRel.HashedKernel.collect"
            end in
        collect None [] s
      let sort_bindings cmp_key s =
        List.stable_sort (fun (k, _) (k', _) -> cmp_key k k') (List.of_seq s)
      let compare cmp_key cmp_val kvs kvs' =
        let rec compare kvs kvs' =
          match kvs, kvs' with
          | [], [] ->
            0
          | [], _ ->
            -1
          | _, [] ->
            1
          | (k, v)::kvs, (k', v')::kvs' ->
            let k_cmp = cmp_key k k' in
            if Int.(k_cmp <> 0) then
              k_cmp
            else 
              let v_cmp = cmp_val v v' in
              if Int.(v_cmp <> 0) then
                v_cmp
              else
                compare kvs kvs' in
        compare kvs kvs'
      type domain_map = Int.Hashset.t Int.Hashmap.t
      type slope_map = Slope.t IntPair.Hashmap.t
      (* domain-codomain mapping, codomain-domain mapping, slopes *)
      type t = domain_map * domain_map * slope_map
      let equal (_, _, p) (_, _, q) =
        if not (Int.equal (IntPair.Hashmap.length p) (IntPair.Hashmap.length p))
          then false
          else
            List.for_all
              (fun (((h, h') as k), ss) ->
                let ss' = IntPair.Hashmap.find_all q k in
                if not (Int.equal (List.length ss) (List.length ss'))
                  then false
                  else
                    List.for_all2 Slope.equal ss ss')
              (collect IntPair.equal (IntPair.Hashmap.to_seq p))
      let compare (_, _, p) (_, _, q) =
        let p_kvs, q_kvs =
          Pair.map
            (fun x -> sort_bindings IntPair.compare (IntPair.Hashmap.to_seq x))
            (p, q) in
        Pair.apply (compare IntPair.compare Slope.compare) (p_kvs, q_kvs)
      let hash (_, _, p) =
        let bindings =
          sort_bindings IntPair.compare (IntPair.Hashmap.to_seq p) in
        List.fold_left
          (fun a (k, v) -> genhash a (genhash (IntPair.hash k) (Slope.hash v)))
          0x9e3779b9
          bindings
      include HashtablePrinter.Make(IntPair.Hashmap)
      let pp fmt (_, _, p) =
        pp IntPair.pp Slope.pp fmt p
      let to_string (_, _, p) =
        to_string IntPair.pp Slope.pp p 
    end

    include (HashedKernel : BasicType with type t = HashedKernel.t)

    let copy (fd, bk, slopes) =
      let fd' = Int.Hashmap.create (Int.Hashmap.length fd) in
      let () = 
        Int.Hashmap.iter
          (fun i hs -> Int.Hashmap.add fd' i (Int.Hashset.copy hs))
          (fd) in
      let bk' = Int.Hashmap.create (Int.Hashmap.length bk) in
      let () = 
        Int.Hashmap.iter
          (fun i hs -> Int.Hashmap.add bk' i (Int.Hashset.copy hs))
          (bk) in
      let slopes' = IntPair.Hashmap.copy slopes in
      (fd', bk', slopes')

    include Containers.Make(HashedKernel)

    let empty () =
      let fd = Int.Hashmap.create 1 in
      let bd = Int.Hashmap.create 1 in
      let slopes = IntPair.Hashmap.create 1 in
      (fd, bd, slopes)

    let add (h, h', s) (fd, bk, slopes) =
      let upd x =
        function
        | None ->
          Int.Hashset.singleton x
        | Some s ->
          let () = Int.Hashset.add s x in
          s in
      Int.Hashmap.replace fd h (upd h' (Int.Hashmap.find_opt fd h)) ;
      Int.Hashmap.replace bk h' (upd h (Int.Hashmap.find_opt bk h')) ;
      IntPair.Hashmap.replace
        slopes
        (h, h')
        (match IntPair.Hashmap.find_opt slopes (h, h')
          with
          | None -> s
          | Some s' -> Slope.max s s')

    let has_decreasing_self_loop (_, _, slopes) =
      List.exists
        (fun ((n, n'), s) ->
          if Int.(n <> n') then
            false
          else
            match s with
            | Slope.Decrease ->
              true
            | _ ->
              false)
        (List.of_seq (IntPair.Hashmap.to_seq slopes))

    let compose (p_fd, _, p_sl) (_, q_bk, q_sl) = 
      let result = empty () in
      let () =
        Int.Hashmap.iter
          (fun h hs ->
            Int.Hashmap.iter
              (fun h' hs' ->
                let s = 
                  Int.Hashset.fold
                  (fun h'' s ->
                    let s' =
                      Slope.max
                        (IntPair.Hashmap.find p_sl (h, h''))
                        (IntPair.Hashmap.find q_sl (h'', h')) in
                    match s with
                    | None   -> Some s'
                    | Some s -> Some (Slope.max s s'))
                  (Int.Hashset.inter hs hs')
                  None in
                match s with
                | None   -> ()
                | Some s -> add (h, h', s) result)
              q_bk)
          p_fd in
      result
    
    (* Repeat code for relational composition, so as to inline check for whether
       we have reached the fixed point - slightly more efficient than comparing
       new relation for equality with the old one at each iteration. *)
    let transitive_closure ((p_fd, p_bk, p_sl) as p) =
      let rec transitive_closure ((q_fd, q_bk, q_sl) as q) =
        let result = copy q in
        let continue =
          Int.Hashmap.fold
            (fun h hs ->
              Int.Hashmap.fold
                (fun h' hs' continue ->
                  let s = 
                    Int.Hashset.fold
                    (fun h'' s ->
                      let s' =
                        Slope.max
                          (IntPair.Hashmap.find p_sl (h, h''))
                          (IntPair.Hashmap.find q_sl (h'', h')) in
                        match s with
                          | None   -> Some s'
                          | Some s -> Some (Slope.max s s'))
                    (Int.Hashset.inter hs hs')
                    None in
                  let () =
                    match s with
                    | None   -> ()
                    | Some s -> add (h, h', s) result in
                  match s with
                  | None ->
                    continue
                  | Some s' ->
                    continue
                      ||
                    not (IntPair.Hashmap.mem q_sl (h, h'))
                      ||
                    Slope.((IntPair.Hashmap.find q_sl (h, h')) < s'))
              q_bk)
            p_fd
            false in
        if continue then transitive_closure result else result in
      transitive_closure p

  end

  (* A height graph is a set of nodes and sloped relation for each edge *)
  type height_graph = Int.Set.t * SlopedRel.t IntPair.Map.t

  let pp_height_graph fmt (nodes, slopes) =
    Format.fprintf fmt "@[Nodes: %a@.Slopes: %a@]"
      Int.Set.pp nodes
      (IntPair.Map.pp SlopedRel.pp) slopes

  (* Compute the composition closure of the given height graph *)
  let comp_closure (ns, slopes) =
    let init = 
      Int.Set.fold
        (fun n ccl ->
          Int.Set.fold
            (fun n' ccl ->
              let edge = (n, n') in
              let s = SlopedRel.Hashset.create 1 in
              let () =
                match (IntPair.Map.find_opt edge slopes) with
                | None ->
                  ()
                | Some p ->
                  SlopedRel.Hashset.add s p in
              IntPair.Map.add edge s ccl)
            ns
            ccl)
        ns
        IntPair.Map.empty in
    let rec closure ccl =
      let ccl', cont =
        Int.Set.fold (fun n ->
        Int.Set.fold (fun n' (acc, cont) ->
          let old_slopes = IntPair.Map.find (n, n') ccl in
          let new_slopes = SlopedRel.Hashset.copy old_slopes in
          let cont =
            Int.Set.fold (fun n'' ->
            SlopedRel.Hashset.fold (fun p ->
            SlopedRel.Hashset.fold (fun q cont ->
              let r = SlopedRel.compose p q in
              let () = SlopedRel.Hashset.add new_slopes r in
              cont || not (SlopedRel.Hashset.mem old_slopes r)
            ) (IntPair.Map.find (n'', n') ccl)
            ) (IntPair.Map.find (n, n'') ccl)
            ) ns cont in
          (IntPair.Map.add (n, n') new_slopes acc, cont)
        ) ns) ns (IntPair.Map.empty, false) in
      if cont then closure ccl' else ccl in
    closure init

  let pp_closure fmt ccl =
    let pp fmt s =
      let () = Format.fprintf fmt "@[[" in
      let first = ref true in
      let () =
        SlopedRel.Hashset.iter
          (fun p ->
            let () = if not !first then Format.fprintf fmt ", " in
            let () = first := false in
            let () = Format.fprintf fmt "%a" SlopedRel.pp p in
            ())
          s in
      let () = Format.fprintf fmt "]@]" in
      () in
    IntPair.Map.pp pp fmt ccl

  let check_proof p =
    Stats.MC.call () ;
    let to_height_graph p =
      Int.Map.fold
        (fun n (_, succs) (nodes, slopes) ->
          let nodes = Int.Set.add n nodes in
          let slopes = 
            List.fold_left
              (fun slopes (n', all_tags, prog_tags) ->
                let hs = SlopedRel.empty () in
                let () =
                  IntPair.Set.iter
                    (fun (h, h') -> SlopedRel.add (h, h', Slope.Stay) hs)
                    all_tags in
                let () =
                  IntPair.Set.iter
                    (fun (h, h') -> SlopedRel.add (h, h', Slope.Decrease) hs)
                    prog_tags in
                IntPair.Map.add (n, n') hs slopes)
              slopes
              succs in
          (nodes, slopes))
        p
        (Int.Set.empty, IntPair.Map.empty) in
    let () = debug (fun () -> "Checking soundness starts...") in
    let ((nodes, _) as g) = to_height_graph p in
    let () = debug (fun () -> "Height Graph:\n" ^ mk_to_string pp_height_graph g) in
    let ccl = comp_closure g in
    let () = debug (fun () -> "Composition Closure:\n" ^ mk_to_string pp_closure ccl) in
    let retval =
      try
        let () = 
          Int.Set.iter (fun n ->
          SlopedRel.Hashset.iter (fun p ->
            let () = debug (fun () -> "Checking " ^ mk_to_string SlopedRel.pp p) in
            let r = SlopedRel.transitive_closure p in
            let () = debug (fun () -> "Transitive closure: " ^ mk_to_string SlopedRel.pp r) in
            if not (SlopedRel.has_decreasing_self_loop r)
              then raise Not_found
          ) (IntPair.Map.find (n, n) ccl)
          ) nodes in
        true
      with Not_found ->
        false in
    if retval then Stats.MC.accept () else Stats.MC.reject () ;
    debug (fun () ->
        "Checking soundness ends, result=" ^ if retval then "OK" else "NOT OK" ) ;
    retval
  
  (* Call out to C++ library for relational check *)
  module Ext = struct

    external create_hgraph : int -> unit = "create_hgraph"
    external destroy_hgraph : unit -> unit = "destroy_hgraph"
    external add_node : int -> unit = "add_node"
    external add_height : int -> int -> unit = "add_height"
    external add_edge : int -> int -> unit = "add_edge"
    external add_stay : int -> int -> int -> int -> unit = "add_stay"
    external add_decrease : int -> int -> int -> int -> unit = "add_decr"
    external relational_check : int -> bool = "relational_check"
    external sd_check : unit -> bool = "sd_check"
    external xsd_check : unit -> bool = "xsd_check"
    external print_ccl : unit -> unit = "print_ccl"
    external print_stats : unit -> unit = "print_statistics"
    external test : unit -> bool = "test"


    (* Flags for applying different optimisations in the C++ code
       These values MUST match the corresponding constants in the C++ code
       Check heighted_graph.c
    *)
    let flag_fail_fast = 0b0001
    let flag_use_scc_check = 0b0010
    let flag_use_idempotence = 0b0100
    let flag_use_minimality = 0b1000
    let flag_compute_full_ccl = 0b10000

    let opts = ref 0
    let do_stats = ref false
  
    let fail_fast () =
      opts := !opts lor flag_fail_fast
    let use_scc_check () =
      if not (Int.equal (!opts land flag_use_idempotence) 0) then
        prerr_endline
          "Cannot use both idempotence and SCC-based loop check - ignoring SCC-based loop check!"
      else
      opts := !opts lor flag_use_scc_check
    let use_idempotence () =
      let use_min = not (Int.equal (!opts land flag_use_minimality) 0) in
      let use_scc = not (Int.equal (!opts land flag_use_scc_check) 0) in
      if use_min then
          prerr_endline
            "Cannot use both idempotence and minimality - ignoring idempotence!" ;
      if use_scc then
        prerr_endline
          "Cannot use both idempotence and SCC-based loop check - ignoring idempotence!" ;
      if (not use_min) && (not use_scc) then
        opts := !opts lor flag_use_idempotence
    let use_minimality () =
      if not (Int.equal (!opts land flag_use_idempotence) 0) then
        prerr_endline
          "Cannot use both minimality and idempotence - ignoring minimality!"
      else
        opts := !opts lor flag_use_minimality
    let compute_full_ccl () =
      opts := !opts lor flag_compute_full_ccl

    let check_proof p =
      let process_succ n (n', tps, prog) =
        add_edge n n' ;
        IntPair.Set.iter
          (fun ((h, h') as p) ->
            if IntPair.Set.mem p prog
              then add_decrease n h n' h'
              else add_stay n h n' h')
          (tps) ;
        in
      let add_heights n (tags, _) =
        Int.Set.iter (add_height n) tags in
      let add_edges n (_, succs) =
        List.iter (process_succ n) succs in
      Stats.MC.call () ;
      debug (fun () -> "Checking soundness starts...") ;
      create_hgraph (Lib.Int.Map.cardinal p) ;
      Int.Map.iter add_heights p ;
      Int.Map.iter add_edges p ;
      (* debug (fun () -> "Built height graph") ; *)
      let retval = 
        match !soundness_method with
        | RELATIONAL_CPP ->
          relational_check !opts 
        | SD_CPP ->
          sd_check ()
        | XSD_CPP ->
          xsd_check ()
        | _ ->
          failwith "Unexpected soundness check method!" in
      if retval then Stats.MC.accept () else Stats.MC.reject () ;
      (* debug (fun () -> "Composition Closure:\n") ; *)
      (* if !do_debug then print_ccl() ; *)
      if !do_stats then print_stats ();
      destroy_hgraph () ;
      debug (fun () ->
          "Checking soundness ends, result=" ^ if retval then "OK" else "NOT OK" ) ;
      retval
  
  end

end



module NewAutomata = struct

  external create_hgraph : int -> unit = "create_auto_new"
  external destroy_hgraph : unit -> unit = "destroy_auto_new"
  external add_node : int -> unit = "add_node_auto_new"
  external add_height : int -> int -> unit = "add_height_auto_new"
  external add_edge : int -> int -> unit = "add_edge_auto_new"
  external add_decrease : int -> int -> int -> int -> unit = "add_decr_auto_new"
  external add_stay : int -> int -> int -> int -> unit = "add_stay_auto_new"
  external test : unit -> bool = "test_auto_new"


  let build_HG p =
    let process_succ n (n', tps, prog) =
      add_edge n n' ;
      IntPair.Set.iter
        (fun ((h, h') as p) ->
          if IntPair.Set.mem p prog
            then add_decrease n h n' h'
            else add_stay n h n' h')
        (tps) ;
      in
    let add_heights n (tags, _) =
      Int.Set.iter (add_height n) tags in
    let add_edges n (_, succs) =
      List.iter (process_succ n) succs in
    Stats.MC.call () ;
    debug (fun () -> "Checking soundness starts...") ;
    create_hgraph (Lib.Int.Map.cardinal p) ;
    Int.Map.iter add_heights p ;
    Int.Map.iter add_edges p ;
    let retval = test () in
    retval


end



module SledgeHammer = struct

  external create_aut : int -> unit = "create_aut"
  external destroy_aut : unit -> unit = "destroy_aut"
  external create_vertex : int -> unit = "create_vertex"
  external tag_vertex : int -> int -> unit = "tag_vertex"
  external set_successor : int -> int -> unit = "set_successor"
  external set_trace_pair : int -> int -> int -> int -> unit = "set_trace_pair"
  external set_progress_pair : int -> int -> int -> int -> unit = "set_progress_pair"
  external check_soundness : unit -> bool = "check_soundness_buchi"
  external get_paut_hoa : unit -> string = "get_proof_aut_hoa"
  external get_taut_hoa : unit -> string = "get_trace_aut_hoa"
  external set_initial_vertex : int -> unit = "set_initial_vertex"
  external create_hgraph : int -> unit = "create_hgraph"
  external destroy_hgraph : unit -> unit = "destroy_hgraph"
  external add_node : int -> unit = "add_node"
  external add_height : int -> int -> unit = "add_height"
  external add_edge : int -> int -> unit = "add_edge"
  external add_stay : int -> int -> int -> int -> unit = "add_stay"
  external add_decrease : int -> int -> int -> int -> unit = "add_decr"
  external relational_check : int -> bool = "relational_check"
  external sd_check : unit -> bool = "sd_check"
  external xsd_check : unit -> bool = "xsd_check"
  external print_ccl : unit -> unit = "print_ccl"
  external print_stats : unit -> unit = "print_statistics"
  external test : unit -> bool = "test"

  (* Flags for applying different optimisations in the C++ code
    These values MUST match the corresponding constants in the C++ code
    Check heighted_graph.c
  *)
  let flag_fail_fast = 0b0001
  let flag_use_scc_check = 0b0010
  let flag_use_idempotence = 0b0100
  let flag_use_minimality = 0b1000
  let flag_compute_full_ccl = 0b10000

  let opts = ref 0
  let do_stats = ref false

  let fail_fast () =
    opts := !opts lor flag_fail_fast
  let use_scc_check () =
    if not (Int.equal (!opts land flag_use_idempotence) 0) then
      prerr_endline
        "Cannot use both idempotence and SCC-based loop check - ignoring SCC-based loop check!"
    else
    opts := !opts lor flag_use_scc_check
  let use_idempotence () =
    let use_min = not (Int.equal (!opts land flag_use_minimality) 0) in
    let use_scc = not (Int.equal (!opts land flag_use_scc_check) 0) in
    if use_min then
        prerr_endline
          "Cannot use both idempotence and minimality - ignoring idempotence!" ;
    if use_scc then
      prerr_endline
        "Cannot use both idempotence and SCC-based loop check - ignoring idempotence!" ;
    if (not use_min) && (not use_scc) then
      opts := !opts lor flag_use_idempotence
  let use_minimality () =
    if not (Int.equal (!opts land flag_use_idempotence) 0) then
      prerr_endline
        "Cannot use both minimality and idempotence - ignoring minimality!"
    else
      opts := !opts lor flag_use_minimality
  let compute_full_ccl () =
    opts := !opts lor flag_compute_full_ccl

  let build_HG p =
    let process_succ n (n', tps, prog) =
      add_edge n n' ;
      IntPair.Set.iter
        (fun ((h, h') as p) ->
          if IntPair.Set.mem p prog
            then add_decrease n h n' h'
            else add_stay n h n' h')
        (tps) ;
      in
    let add_heights n (tags, _) =
      Int.Set.iter (add_height n) tags in
    let add_edges n (_, succs) =
      List.iter (process_succ n) succs in
    Stats.MC.call () ;
    debug (fun () -> "Checking soundness starts...") ;
    create_hgraph (Lib.Int.Map.cardinal p) ;
    Int.Map.iter add_heights p ;
    Int.Map.iter add_edges p 

  (**Previous Automata *)
  let print_paut = ref false
  let print_taut = ref false

  let build_automata ?(init=0) p =
    Stats.MC.call () ;
    let create_tags i n = Int.Set.iter (tag_vertex i) (get_tags n) in
    let create_succs i (_, l) =
      Blist.iter (fun (j, _, _) -> set_successor i j) l
    in
    let create_trace_pairs i (_, l) =
      let do_tag_transitions (j, tvs, tps) =
        IntPair.Set.iter (fun (k, m) -> set_trace_pair i j k m) tvs ;
        IntPair.Set.iter (fun (k, m) -> set_progress_pair i j k m) tps
      in
      Blist.iter do_tag_transitions l
    in
    let size = Int.Map.cardinal p in
    let log2size =
      1 + int_of_float (ceil (log (float_of_int size) /. log 2.0))
    in
    debug (fun () -> "Checking soundness starts...") ;
    create_aut log2size ;
    Int.Map.iter (fun i _ -> create_vertex i) p ;
    Int.Map.iter create_tags p ;
    Int.Map.iter create_succs p ;
    set_initial_vertex init ;
    Int.Map.iter create_trace_pairs p 


    (*let retval = check_soundness () in
    if retval then Stats.MC.accept () else Stats.MC.reject () ;
    destroy_aut () ;
    debug (fun () ->
        "Checking soundness ends, result=" ^ if retval then "OK" else "NOT OK" ) ;
    retval*)


end

include (RelationalCheck.Ext : sig
  val fail_fast : unit -> unit
  val use_scc_check : unit -> unit
  val use_idempotence : unit -> unit
  val use_minimality : unit -> unit
  val compute_full_ccl : unit -> unit
  val do_stats : bool ref
end)

let arg_opts =
  [
    ("-spot", Arg.Unit use_spot, ": use the spot model checker to verify pre-proof validity") ;
    ("-print-paut", Arg.Set BuchiCheck.print_paut, ": print the proof automaton in HOA format" ) ;
    ("-print-taut", Arg.Set BuchiCheck.print_taut, ": print the trace automaton in HOA format" ) ;
    ("-rel-ext", Arg.Unit use_external, ": use external C++ relation-based check to verify pre-proof validity") ;
    ("-SD", Arg.Unit use_sprengerdam, ": use Sprenger-Dam check to verify pre-proof validity") ;
    ("-XSD", Arg.Unit use_xtd_sprengerdam, ": use Extended Sprenger-Dam check to verify pre-proof validity") ;
    ("-SH", Arg.Unit use_sledge_hammer, ": use sledge-hammer validty check") ;
    ("-spot-new", Arg.Unit use_spot_new, ": use new spot to verify pre-proof validity") ;
    ("-ff", Arg.Unit fail_fast, ": use fast fail in relation-based validty check") ;
    ("-scc", Arg.Unit use_scc_check, ": use SCC check in relation-based validity check") ;
    ("-idem", Arg.Unit use_idempotence, ": use idempotency optimisation in relation-based validity check") ;
    ("-min", Arg.Unit use_minimality, ": use minimality optimisation in relation-based validity check") ;
    ("-full", Arg.Unit compute_full_ccl, ": compute the full CCL") ;
    ("-rel-stats", Arg.Set do_stats, ": print out profiling stats for the relation-based validity check")
  ]

module CheckCache = Hashtbl

let ccache = CheckCache.create 1000
(* let limit = ref 1 *)

let check_proof ?(init=0) ?(minimize=true) prf =
  if (Int.Map.is_empty prf) then
    true
  else
    let () =
      if not (valid prf init) then (
        pp Format.std_formatter prf ;
        assert false )
    in
    let aprf =
      if minimize
        then minimize_abs_proof prf init
        else prf in
    if (Int.Map.is_empty aprf) then
      true
    else
      let () =
        if not (valid aprf init) then (
          pp Format.std_formatter aprf ;
          assert false )
      in
      try
        debug (fun _ -> mk_to_string pp prf) ;
        debug (fun () -> "Minimized proof:\n" ^ mk_to_string pp aprf) ;
        Stats.MCCache.call () ;
        let r = CheckCache.find ccache aprf in
        Stats.MCCache.end_call () ;
        Stats.MCCache.hit () ;
        let () =
          debug (fun _ ->
              "Found soundness result in the cache: "
              ^ if r then "OK" else "NOT OK" )
        in
        r
      with Not_found ->
        Stats.MCCache.end_call () ;
        Stats.MCCache.miss () ;
        let r =
          match !soundness_method with
          | SPOT ->
            BuchiCheck.check_proof ~init aprf
          | RELATIONAL_OCAML ->
            RelationalCheck.check_proof aprf
          | SLEDGE_HAMMER ->
            SledgeHammer.build_automata aprf;
            SledgeHammer.build_HG aprf;
            RelationalCheck.Ext.test ()
          | SPOT_NEW ->
          NewAutomata.build_HG aprf
          | _ ->
            RelationalCheck.Ext.check_proof aprf in
        Stats.MCCache.call () ;
        CheckCache.add ccache aprf r ;
        Stats.MCCache.end_call () ;
        (* if CheckCache.length ccache > !limit then                                          *)
        (*   begin                                                                            *)
        (*     debug (fun () -> "Soundness cache passed limit: " ^ (string_of_int !limit)) ;  *)
        (*     limit := 10 * !limit                                                           *)
        (*   end ;                                                                            *)
        r

        
