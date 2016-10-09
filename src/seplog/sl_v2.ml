open Hashcons
open Symbols
open MParser
open Lib


let always_parenthesise = ref false

type _t = node hash_consed
and node =
  | True
  | Emp
  | Eq of Sl_tpair.t
  | Deq of Sl_tpair.t
  | Pto of Sl_pto.t
  | Pred of Sl_tpred.t
  | Star of _t * _t
  | Or of _t * _t
  | Exists of Sl_term.t * _t

module Form = struct
  type t = node
  
  let equal x y = match (x,y) with
    | True, True
    | Emp, Emp -> true  
    | Eq p, Eq p' 
    | Deq p, Deq p' -> Sl_tpair.equal p p'
    | Pto(pto), Pto(pto') -> Sl_pto.equal pto pto'
    | Pred(pred), Pred(pred') -> Sl_tpred.equal pred pred'
    | Star(f, g), Star(f', g') 
    | Or(f, g), Or(f', g') -> f==f' && g==g'
    | Exists(x, f), Exists(x', f') -> Sl_term.equal x x' && f==f'
    | _, _ -> false
  
  let hash = function
    | True -> 9
    | Emp -> 11
    | Eq p -> Sl_tpair.hash p
    | Deq p -> abs (19 * (Sl_tpair.hash p) + 1)
    | Pto pto -> abs (19 * (Sl_pto.hash pto) + 2)
    | Pred pred -> abs (19 * (Sl_tpred.hash pred) + 3)
    | Star(f, g) -> abs (19 * (19 * f.hkey + g.hkey) + 4)  
    | Or(f, g) -> abs (19 * (19 * f.hkey + g.hkey) + 5)
    | Exists(x, f) -> abs (19 * (19 * (Sl_term.hash x) + f.hkey) + 6)
    
  end
  
module HForm = Make(Form)  

(* enfore the variable convention *)
(* i.e. all quantifies bind distinct variables and these *)
(* are in addition distinct to all free variables *)
(* let enforce_vc f =                                           *)
(*   let rec alphanorm a v = match a.node with                  *)
(*     | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> (a, v)   *)
(*     | Star(f, g) ->                                          *)
(*       let (f', v') = alphanorm f v in                        *)
(*       let (g', v'') = alphanorm g v' in                      *)
(*       (mk_star f' g', v'')                                   *)
(*     | Or(f, g) ->                                            *)
(*       let (f', v') = alphanorm f v in                        *)
(*       let (g', v'') = alphanorm g v' in                      *)
(*       (mk_or f' g', v'')                                     *)
(*     | Exists(x, f) ->                                        *)
(*       if Sl_term.Set.mem x v then                            *)
(*         let y = Sl_term.fresh_fvar v in                      *)
(*         let f' = subst (Sl_subst.singleton x y) f in         *)
(*         let (f', v') = alphanorm f' (Sl_term.Set.add y v) in *)
(*         (mk_exists y f', v')                                 *)
(*       else                                                   *)
(*         let (f', v') = alphanorm f (Sl_term.Set.add x v) in  *)
(*         (mk_exists x f', v')                                 *)
(*   in                                                         *)
(*   fst (alphanorm f (free_vars f))                            *)

    
(* let is_dsh f =                           *)
(*   let rec disj h = match h.node with     *)
(*     | Or(f, g) -> is_symheap f && disj g *)
(*     | _ -> is_symheap h                  *)
(*   in                                     *)
(*   is_symheap f || disj f                 *)


(* let to_dsh f =                                                     *)
(*   let rec norm a = match a.node with                               *)
(*     (* (f \/ g) \/ h = f \/ (g \/ h) *)                            *)
(*     | Or({node=Or(f, g)}, h) -> mk_or f (mk_or g h)                *)
    
(*     (* (f * g) * h = f * (g * h) *)                                *)
(*     | Star({node=Star(f, g)}, h) ->  mk_star f (mk_star g h)       *)
    
(*     (* \exists x. (f \/ g) = (\exists x.f) \/ (\exists x.g) *)     *)
(*     | Exists(x, {node=Or(f,  g)}) ->                               *)
(*       mk_or (mk_exists x f) (mk_exists x g)                        *)
      
(*     (* f * (g \/ h) = (f*g) \/ (f*h) *)                            *)
(*     | Star(f, {node=Or(g, h)}) ->                                  *)
(*       mk_or (mk_star f g) (mk_star f h)                            *)
(*     (* (f \/ g) * h) = (f*h) \/ (g*h) *)                           *)
(*     | Star({node=Or(f, g)}, h) ->                                  *)
(*       mk_or (mk_star f h) (mk_star g h)                            *)
    
(*     (* (\exists x.f) * f' = \exists x .(f*f') *)                   *)
(*     (* whenever x \notin free_vars f' *)                           *)
(*     | Star({node=Exists(x, f)}, f') ->                             *)
(*       if Sl_term.Set.mem x (free_vars f') then a else              *)
(*         mk_exists x (mk_star f f')                                 *)
(*     | Star(f, {node=Exists(x, f')}) ->                             *)
(*       if Sl_term.Set.mem x (free_vars f) then a else               *)
(*         mk_exists x (mk_star f f')                                 *)
    
(*     (* | Exists(x, {node=Exists(y,f)}) ->           *)             *)
(*     (*   if (Sl_term.compare x y) <= 0 then a else  *)             *)
(*     (*     mk_exists y (mk_exists x f)              *)             *)
    
(*     | Exists(x, f) -> mk_exists x (norm f)                         *)
(*     | Or(f, g) -> mk_or (norm f) (norm g)                          *)
(*     | Star(f, g) -> mk_star (norm f) (norm g)                      *)
    
(*     (* | Eq(x,y) ->                                             *) *)
(*     (*   if (Sl_term.compare x y) <= 0 then a else mk_eq (y,x)  *) *)
(*     (* | Deq(x,y) ->                                            *) *)
(*     (*   if (Sl_term.compare x y) <= 0 then a else mk_deq (y,x) *) *)
(*     | _ -> a                                                       *)
(*   in                                                               *)
(*   let res = Lib.fixpoint equal norm (enforce_vc f) in              *)
(*   assert (is_dsh res) ; res                                        *)

(* let to_symheap f =                                        *)
(*   if not (is_symheap f) then failwith "to_symheap" else f *)







module CommonImplementation = 
  struct
    type t = _t
    
    let ft = HForm.create 251
    
    let compare f g = Pervasives.compare f.tag g.tag
    let equal f g = f==g 
    let hash f = f.Hashcons.hkey

    let rec equal_upto_tags x y = match (x.node,y.node) with
      | True,_ | Emp,_ | Eq _,_ | Deq _,_ | Pto _,_ -> equal x y
      | Pred(pred), Pred(pred') -> Sl_tpred.equal_upto_tags pred pred'
      | Star(f, g), Star(f', g') 
      | Or(f, g), Or(f', g') -> equal_upto_tags f f' && equal_upto_tags g g'
      | Exists(x, f), Exists(x', f') -> Sl_term.equal x x' && equal_upto_tags f f'
      | _, _ -> false
                
    let true_val = HForm.hashcons ft (True)
    let emp_val = HForm.hashcons ft (Emp)
    
    let mk_true () = true_val
    let mk_emp () = emp_val
    let mk_eq p =    HForm.hashcons ft (Eq p)
    let mk_deq p =   HForm.hashcons ft (Deq p)
    let mk_pto pto = HForm.hashcons ft (Pto pto)
    let mk_pred pred = HForm.hashcons ft (Pred pred)
    let mk_star f g = HForm.hashcons ft (Star(f, g))
    let mk_or f g = HForm.hashcons ft (Or(f, g))
    let mk_exists x f = 
      if not (Sl_term.is_var x) then failwith "mk_exists nil" 
      else HForm.hashcons ft (Exists(x, f))
    
    let is_true f = match f.node with
      | True -> true
      | _ -> false 
    let is_emp f = match f.node with
      | Emp -> true
      | _ -> false 
    let is_eq f = match f.node with
      | Eq _ -> true
      | _ -> false 
    let is_deq f = match f.node with
      | Deq _ -> true
      | _ -> false 
    let is_pto f = match f.node with
      | Pto _ -> true
      | _ -> false 
    let is_pred f = match f.node with
      | Pred _ -> true
      | _ -> false 
    let is_star f = match f.node with
      | Star _ -> true
      | _ -> false 
    let is_or f = match f.node with
      | Or _ -> true
      | _ -> false 
    let is_exists f = match f.node with
      | Exists _ -> true
      | _ -> false 
    
    let is_atom f = match f.node with
      | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> true
      | _ -> false 
  
    let dest_true f = match f.node with
      | True -> ()
      | _ -> failwith "dest_true"
    let dest_emp f = match f.node with
      | Emp -> ()
      | _ -> failwith "dest_emp"
    let dest_eq f = match f.node with
      | Eq p -> p
      | _ -> failwith "dest_eq" 
    let dest_deq f = match f.node with
      | Deq p -> p
      | _ -> failwith "dest_deq" 
    let dest_pto f = match f.node with
      | Pto p -> p
      | _ -> failwith "dest_pto" 
    let dest_pred f = match f.node with
      | Pred p -> p
      | _ -> failwith "dest_pred" 
    let dest_star f = match f.node with
      | Star(f, g) -> (f, g)
      | _ -> failwith "dest_star"
    let dest_or f = match f.node with
      | Or(f, g) -> (f, g)
      | _ -> failwith "dest_or" 
    let dest_exists f = match f.node with
      | Exists(x, f) -> (x, f)
      | _ -> failwith "dest_exist"

    let rec pp fmt f = 
      let paren p fmt f =
        if (not !always_parenthesise) && (is_atom f || is_star f || p f) then 
          pp fmt f 
        else 
          Format.fprintf fmt "@[%s%a%s@]" symb_lp.str pp f symb_rp.str 
      in
      match f.node with
      | True -> Format.pp_print_string fmt keyw_true.str
      | Emp -> Format.pp_print_string fmt symb_emp.str
      | Eq(x,y) -> 
        Format.fprintf fmt "@[%a%s%a@]" Sl_term.pp x symb_eq.str Sl_term.pp y  
      | Deq(x,y) -> 
        Format.fprintf fmt "@[%a%s%a@]" Sl_term.pp x symb_deq.str Sl_term.pp y  
      | Pto p -> 
        Sl_pto.pp fmt p
      | Pred p ->
        Sl_tpred.pp fmt p
      | Exists(x, f) ->
        Format.fprintf fmt "@[%s %a%s %a@]" 
          keyw_exists.str Sl_term.pp x symb_dot.str (paren is_exists) f
      | Star(f, g) ->
        Format.fprintf fmt "@[%a%s%a@]"
          (paren (fun _ -> false)) f symb_star.sep (paren (fun _ -> false)) g 
      | Or(f, g) ->
        Format.fprintf fmt "@[%a%s%a@]"
          (paren is_or) f symb_or.sep (paren is_or) g 
    
    let to_string = mk_to_string pp 
    
    let parse =
      let parse_infix_pair symb st = 
        pair Sl_term.parse (parse_symb symb >> Sl_term.parse) st
      in
      let connectives = 
        [ (* abstraction is necessary to avoid capture to a fixed poly-type *)
          [ Infix  ((fun st -> (parse_symb symb_star >>$ mk_star) st), Assoc_right) ];
          [ Prefix (fun st -> (parse_symb keyw_exists >> Sl_term.parse >>= 
                     (fun x -> parse_symb symb_dot >>$ mk_exists x)) st) ];
          [ Infix  ((fun st -> (parse_symb symb_or >>$ mk_or) st), Assoc_right) ]
        ] 
      in
      let rec paren_atom st =
        ( attempt (parse_symb symb_lp >> parse << parse_symb symb_rp) <|>
          attempt (parse_symb keyw_true >>$ mk_true ()) <|>
          attempt (parse_symb keyw_emp >>$ mk_emp ()) <|>
          attempt (parse_infix_pair symb_eq |>> mk_eq) <|> 
          attempt (parse_infix_pair symb_deq |>> mk_deq) <|>
          attempt (Sl_pto.parse |>> mk_pto) <|>
                  (Sl_tpred.parse |>> mk_pred)
        ) st
      and parse st = expression connectives paren_atom st 
      in  parse
    
    let rec map_atoms fn g = match g.node with
      | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> fn g
      | Star(l, r) -> mk_star (map_atoms fn l) (map_atoms fn r) 
      | Or(l, r) -> mk_or (map_atoms fn l) (map_atoms fn r)
      | Exists(x, f) -> mk_exists x (map_atoms fn f) 

    let rec exists_atoms pred g = match g.node with
      | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> pred g
      | Star(l, r) | Or(l, r) -> exists_atoms pred l || exists_atoms pred r
      | Exists(_, f) -> exists_atoms pred f

    let rec find_map_atoms fn g = match g.node with
      | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> fn g
      | Exists(_, f) -> find_map_atoms fn f
      | Star(l, r) | Or(l, r) ->
        match find_map_atoms fn l with
        | None -> find_map_atoms fn r
        | v -> v
    
    let find_atoms pred f = 
      find_map_atoms (fun g -> Option.pred pred g) f
    
    let rec fold_atoms fn g a =
      match g.node with
        | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> fn g a
        | Star(l, r) | Or(l, r) -> fold_atoms fn r (fold_atoms fn l a) 
        | Exists(_, f) -> fold_atoms fn f a 
    
    let rec _terms free f = match f.node with 
      | True | Emp -> Sl_term.Set.empty
      | Eq(x,y) | Deq(x,y) -> Sl_term.Set.add y (Sl_term.Set.singleton x)
      | Pto p -> Sl_pto.terms p
      | Pred pred -> Sl_tpred.terms pred
      | Star(f, g) | Or(f, g) -> Sl_term.Set.union (_terms free f) (_terms free g) 
      | Exists(x, f) -> 
        (if free then Sl_term.Set.remove else Sl_term.Set.add) x (_terms free f)
        
    let terms f = _terms false f
    let free_vars f = Sl_term.filter_vars (_terms true f)
    
    
    (* substitution *)
    let rec subst theta f = match f.node with
      | True | Emp -> f
      | Eq p -> mk_eq (Sl_tpair.subst theta p)
      | Deq p -> mk_deq (Sl_tpair.subst theta p)
      | Pto p -> mk_pto (Sl_pto.subst theta p)
      | Pred p -> mk_pred (Sl_tpred.subst theta p)
      | Star(f, g) -> mk_star (subst theta f) (subst theta g)
      | Or(f, g) -> mk_or (subst theta f) (subst theta g) 
      | Exists(x, f) -> mk_exists x (subst (Sl_term.Map.remove x theta) f)
    
    let rec substitutable x t f = match f.node with 
      | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> true
      | Star(g, h) | Or(g, h) -> substitutable x t g && substitutable x t h 
      | Exists(y, g) -> 
        not (Sl_term.Set.mem x (free_vars g)) ||
        not (Sl_term.equal y t) && substitutable x t g

		(* capture avoiding, no touching existential quantifiers *)    
    let subst theta f = 
      assert (Sl_term.Map.for_all (fun x t -> substitutable x t f) theta) ;
      subst theta f     
    
    (* alpha renaming *)
    (* let alpha theta f =                                                         *)
    (*   let rec aux active inactive f =                                           *)
    (*     if Sl_subst.is_empty active && Sl_subst.is_empty inactive then f else   *)
    (*     match f.node with                                                       *)
    (*     | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> subst active f          *)
    (*     | Star(f, g) -> mk_star (aux active inactive f) (aux active inactive g) *)
    (*     | Or(f, g) -> mk_or (aux active inactive f) (aux active inactive g)     *)
    (*     | Exists(x, f) ->                                                       *)
    (*       if Sl_term.Map.mem x active then                                      *)
    (*         let active' = Sl_term.Map.remove x active in                        *)
    (*         mk_exists x (aux active' inactive f)                                *)
    (*       else if Sl_term.Map.mem x inactive then                               *)
    (*         let x' = Sl_term.Map.find x inactive in                             *)
    (*         let active' = Sl_term.Map.add x x' active in                        *)
    (*         let inactive' = Sl_term.Map.remove x inactive in                    *)
    (*         mk_exists x' (aux active' inactive' f)                              *)
    (*       else                                                                  *)
    (*         mk_exists x (aux active inactive f)                                 *)
    (*   in                                                                        *)
    (*   aux Sl_subst.empty theta f                                                *)
    
    let tags f = 
      fold_atoms
        (fun g a -> if is_pred g then Tags.add (Sl_tpred.tag (dest_pred g)) a else a)
        f
        Tags.empty

    let tag_pairs f = Tagpairs.mk (tags f)
  
    module Zipper =
      struct
        type path =
          | Top
          | StarL of path * _t
          | StarR of _t * path
          | OrL of path * _t
          | OrR of _t * path
          | ExistsD of Sl_term.t * path
    
        type t = _t * path
    
        let change (_, p) f = (f, p)
    
        let rec zip (f, p) = match p with
          | Top -> f
          | StarL(q, r) -> zip (mk_star f r, q)
          | StarR(l, q) -> zip (mk_star l f, q)
          | OrL(q, r) -> zip (mk_or f r, q)
          | OrR(l, q) -> zip (mk_or l f, q)
          | ExistsD(x, q) -> zip (mk_exists x f, q)
    
        let rec remove ?(empty=true_val) = function
          | Top -> empty
          | ExistsD(_, q) -> remove ~empty q
          | StarL(q, f) | OrL(q, f) | StarR(f, q) | OrR(f, q) -> zip (f, q)
        
    
    (* let fold f g a =                                                        *)
    (*   let rec aux f ((g, path) as loc) a =                                  *)
    (*     match g.node with                                                   *)
    (*       | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> f loc a           *)
    (*       | Star(l, r) ->                                                   *)
    (*         aux f (r, StarR(l, path)) (f loc (aux f (l, StarL(path, r)) a)) *)
    (*       | Or(l, r) ->                                                     *)
    (*         aux f (r, OrR(l, path)) (f loc (aux f (l, OrL(path, r)) a))     *)
    (*       | Exists(x, g) ->                                                 *)
    (*         f loc (aux f (g, ExistsD(x, path)) a)                           *)
    (*   in                                                                    *)
    (*   aux f (g, Top) a                                                      *)
    
    (* let iter f g =                                                        *)
    (*   let rec aux f ((g, path) as loc) =                                  *)
    (*     match g.node with                                                 *)
    (*       | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> f loc           *)
    (*       | Star(l, r) ->                                                 *)
    (*         aux f (l, StarL(path, r)) ; aux f (r, StarR(l, path)) ; f loc *)
    (*       | Or(l, r) ->                                                   *)
    (*         aux f (l, OrL(path, r)) ; aux f (r, OrR(l, path)) ; f loc     *)
    (*       | Exists(x, g) -> aux f (g, ExistsD(x, path)) ; f loc           *)
    (*   in                                                                  *)
    (*   aux f (g, Top)                                                      *)
    
    
    (* let find p f =                                             *)
    (*   let rec search pred ((f, p) as loc) =                    *)
    (*     if pred f then Some loc else                           *)
    (*     match f.node with                                      *)
    (*       | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> None *)
    (*       | Star(l, r) ->                                      *)
    (*         let res = search pred (l, StarL(p, r)) in          *)
    (*         if Option.is_some res then res else                *)
    (*         search pred (r, StarR(l, p))                       *)
    (*       | Or(l, r) ->                                        *)
    (*         let res = search pred (l, OrL(p, r)) in            *)
    (*         if Option.is_some res then res else                *)
    (*         search pred (r, OrR(l, p))                         *)
    (*       | Exists(x, g) -> search pred (g, ExistsD(x, p))     *)
    (*   in                                                       *)
    (*   search p (f, Top)                                        *)
    
        let search_atom fn f =
          let rec search fn ((f, p) as loc) =
            match f.node with
              | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> fn loc
              | Exists(x, g) -> search fn (g, ExistsD(x, p))
              | Star(l, r) ->
                begin
                  match search fn (l, StarL(p, r)) with
                  | None -> search fn (r, StarR(l, p))
                  | res -> res
                end
              | Or(l, r) ->
                begin
                  match search fn (l, OrL(p, r)) with
                  | None -> search fn (r, OrR(l, p))
                  | res -> res
                end
          in
          search fn (f, Top)
      
      end
  end

module Atom =
  struct
    include CommonImplementation
    
    let check a  = assert (is_atom a)
    let check_fun f a = check a ; f a
    let check_res f a = let res = f a in check res ; res

    let unify
      ?(sub_check=Sl_subst.trivial_check)
      ?(cont=Sl_unifier.trivial_continuation) 
      ?(init_state=Sl_unifier.empty_state) f g =
      check f ; check g ; 
      match (f.node, g.node) with
      | True, True | Emp, Emp -> cont init_state
      | Eq p, Eq p' | Deq p, Deq p' -> 
        Sl_tpair.unify ~sub_check ~cont ~init_state p p'
      | Pto(pto), Pto(pto') -> 
        Sl_pto.unify ~sub_check ~cont ~init_state pto pto'
      | Pred(pred), Pred(pred') -> 
        Sl_tpred.unify ~sub_check ~cont ~init_state pred pred'
      | _, _ -> None
    
    let parse st =
      (parse >>= 
        (fun f -> if not (is_atom f) then fail "Not an atom" else return f))
        st  
    let of_string = mk_of_string parse


    let equal = check_fun (check_fun equal)
    let compare = check_fun (check_fun compare)
    let hash = check_fun hash
    let to_string = check_fun to_string
    let pp fmt = check_fun (pp fmt)
    let of_string = check_res of_string
    let subst theta = check_res (check_fun (subst theta))
    let terms = check_fun terms
    let free_vars = check_fun free_vars
    let subst theta = check_res (check_fun (subst theta))
    let tags = check_fun tags
    let tag_pairs = check_fun tag_pairs
    
  end

module SymHeap =
  struct
    include CommonImplementation

    let is_symheap f =
      let rec body h = match h.node with
        | Star(f, g) -> is_atom f && body g
        | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ -> true
        | Or _ | Exists _ -> false
      in
      let rec exists bound h = match h.node with 
        | Exists(x, k) -> 
          not (Sl_term.Set.mem x bound) && exists (Sl_term.Set.add x bound) k
        | True | Emp | Eq _ | Deq _ | Pto _ | Pred _ | Star _ -> body h
        | Or _ -> false
      in
      exists Sl_term.Set.empty f

    let check a  = assert (is_symheap a)
    let check_fun f a = check a ; f a
    let check_res f a = let res = f a in check res ; res
        
    let parse st =
      (parse >>= 
        (fun f -> if not (is_symheap f) then fail "Not a symheap" else return f))
        st  
    let of_string = mk_of_string parse
    
    let bound_vars sh =
      let rec loop vs n = match n.node with
        | Exists (x, f) -> loop (Sl_term.Set.add x vs) f
        | _ -> vs in
      loop Sl_term.Set.empty sh
    
    let dest_atom f = 
      if is_atom f then f else failwith "Not an atoms"
      
    let of_atom f = f
    
    let fold = fold_atoms
    
    let find = find_atoms
    
    let exists = exists_atoms
    
    (* let _build_uf f =                                                          *)
    (*     fold                                                                   *)
    (*       (fun atom a -> if is_eq atom then Sl_uf.add (dest_eq atom) a else a) *)
    (*       f                                                                    *)
    (*       Sl_uf.empty                                                          *)
    
    (* explicitly recursive version for recursive memoisation *) 
    let rec _build_uf f = match f.node with
      | Exists(_, g) -> _build_uf g
      | Eq eq -> Sl_uf.of_list [eq] (* this is lazy *)
      | Star(g,h) -> Sl_uf.union (_build_uf g) (_build_uf h)
      | _ -> Sl_uf.empty
    
		let equates f x y = 
      Sl_uf.equates (_build_uf f) x y
    
    let disequates f x y =
      exists
        (fun atom -> 
          not (is_deq atom) ||
          let (w, z) = dest_deq atom in
          equates f x w && equates f y z
          || 
          equates f x z && equates f y w 
        )
        f
    
    let find_lval x f =
      Option.map
        dest_pto 
        (find_atoms 
          (fun atom -> is_pto atom && equates f x (fst (dest_pto atom))) 
          f)
    
    let idents f =
      fold 
        (fun atom a -> 
          if is_pred atom then 
            Sl_predsym.MSet.add (Sl_tpred.predsym (dest_pred atom)) a
          else
            a
        ) 
        f
        Sl_predsym.MSet.empty

    let inconsistent f = 
      exists
        (fun atom -> is_deq atom && let (x,y) = dest_eq atom in equates f x y) 
        f

    let memory_consuming f =
      not (exists is_pred f) || exists is_pto f 
        
    let constructively_valued h =
      let freevars = free_vars h in
      let existvars = bound_vars h in
      let is_cvalued cvalued v =
        Sl_term.Set.exists (equates h v) cvalued ||
        exists 
          (fun atom ->
            is_pto atom &&
            let (y,zs) = dest_pto atom in 
            Sl_term.Set.mem y cvalued && Blist.exists (Sl_term.equal v) zs) 
          h in
      let rec aux cvalued rest =
        let new_cvalued = Sl_term.Set.filter (is_cvalued cvalued) rest in
        if Sl_term.Set.is_empty new_cvalued 
        then
          Sl_term.Set.is_empty rest 
        else
          aux 
            (Sl_term.Set.union cvalued new_cvalued) 
            (Sl_term.Set.diff rest new_cvalued) in
      aux freevars existvars
    
    let subst_tags tagpairs f =
      map_atoms
        (fun g -> 
          if is_pred g then 
            mk_pred (Sl_tpred.subst_tag tagpairs (dest_pred g))
          else
            g
        )
        f
    
    let rec unify
      ?(sub_check=Sl_subst.trivial_check)
      ?(cont=Sl_unifier.trivial_continuation)
      ?(init_state=Sl_unifier.empty_state) f g =
        let foreach_g (f_atom, f_path) (g_atom, g_path) =
          let newcont state =
            let f' = Zipper.remove f_path in
            let g' = Zipper.remove g_path in
            match (f'.node, g'.node) with
            | True, True -> cont state
            | _ -> unify ~sub_check ~cont ~init_state:state f' g' in
          Atom.unify ~sub_check ~cont:newcont ~init_state f_atom g_atom in
        let foreach_f f_loc =
          Zipper.search_atom (fun g_loc -> foreach_g f_loc g_loc) g in 
        Zipper.search_atom foreach_f f

		let rec strip_quantifiers f = match f.node with
      | Exists(_, g) -> strip_quantifiers g
      | _ -> f
      
		
    let unify
      ?(sub_check=Sl_subst.trivial_check)
      ?(cont=Sl_unifier.trivial_continuation)
      ?(init_state=Sl_unifier.empty_state) f g =
        check f ; check g;
				let f',g' = Pair.map strip_quantifiers (f,g) in
        let f_exs,g_exs = Pair.map bound_vars (f,g) in
        let unicheck theta t t' = 
          sub_check theta t t' && 
          (
            not (Sl_term.Set.mem t f_exs) ||
            not (Sl_term.Set.mem t' g_exs) ||
            Sl_term.Set.mem t f_exs && 
            ( Sl_term.is_nil t' ||
              Sl_term.Set.mem t' g_exs && 
              Sl_term.Map.for_all (fun _ t'' -> not (Sl_term.equal t' t'')) theta
            )
          )
        in
        unify ~sub_check:unicheck ~cont ~init_state f' g' 

    let alpha_rename theta f =
      let rec aux g = match g.node with
        | Exists(x, h) -> mk_exists (Sl_term.subst theta x) (aux h)
        | _ -> subst theta g
      in
      let qs = bound_vars f in
      let xs = Sl_term.Map.fold (fun x _ a -> Sl_term.Set.add x a) theta Sl_term.Set.empty in
      let ts = Sl_term.Map.fold (fun _ t a -> Sl_term.Set.add t a) theta Sl_term.Set.empty in
      assert (Sl_term.Set.subset xs qs) ;
      assert (Sl_term.Set.cardinal ts = Sl_term.Map.cardinal theta) ;
      aux f 

    let equal = check_fun (check_fun equal)
    let compare = check_fun (check_fun compare)
    let hash = check_fun hash
    let to_string = check_fun to_string
    let pp fmt = check_fun (pp fmt)
    let of_string = check_res of_string
    
    let terms = check_fun terms
    let free_vars = check_fun free_vars
    let subst theta = check_res (check_fun (subst theta))
    let tags = check_fun tags
    let tag_pairs = check_fun tag_pairs

    let bound_vars = check_fun bound_vars
    let is_atom = check_fun is_atom
    let dest_atom = Atom.check_res (check_fun dest_atom)
    let of_atom = check_res (Atom.check_fun of_atom)
    let fold fn = check_fun (fold (Atom.check_fun fn))
    let exists pred = check_fun (exists (Atom.check_fun pred)) 
    let find pred f = 
      Option.map (check_fun Fun.id) (check_fun (find (Atom.check_fun pred)) f)
    
    let equates = check_fun equates
    let disequates = check_fun disequates
    let find_lval x = check_fun (find_lval x)
    let idents = check_fun idents
    let inconsistent = check_fun inconsistent
    let memory_consuming = check_fun memory_consuming
    let constructively_valued = check_fun constructively_valued
                         
  end


module type CommonInterface = 
  sig
    include Utilsigs.BasicType
    val equal_upto_tags : t -> t -> bool
    val parse : (t, 'a) MParser.parser
    val of_string : string -> t
    
    val mk_true : unit -> t
    val mk_emp : unit -> t
    val mk_eq : Sl_tpair.t -> t
    val mk_deq : Sl_tpair.t -> t
    val mk_pto : Sl_pto.t -> t 
    val mk_pred  : Sl_tpred.t -> t
  
    val is_true : t -> bool
    val is_emp : t -> bool
    val is_eq : t -> bool
    val is_deq : t -> bool
    val is_pto : t -> bool  
    val is_pred  : t -> bool
  
    val dest_true : t -> unit
    val dest_emp : t -> unit
    val dest_eq : t -> Sl_tpair.t
    val dest_deq : t -> Sl_tpair.t
    val dest_pto : t -> Sl_pto.t
    val dest_pred  : t -> Sl_tpred.t
    
    val terms : t -> Sl_term.Set.t
    val free_vars : t -> Sl_term.Set.t
    val subst : Sl_subst.t -> t -> t
    val tags : t -> Tags.t
    val tag_pairs : t -> Tagpairs.t
  end
    