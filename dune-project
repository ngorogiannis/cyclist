(lang dune 2.2)

(name cyclist)

(package
 (name cyclist)
 (synopsis "Cyclist is a framework for building cyclic theorem provers")
 (depends
  (ocaml (>= 4.08.0))
  (stdlib-shims :build)
  (dune-configurator :build)
  (mparser-re (>= 1.3))
  (hashcons (>= 1.0.1))
  (hashset (>= 1.0.0))
  (ocamlgraph (>= 1.8.7))))

(formatting disabled)

(generate_opam_files true)