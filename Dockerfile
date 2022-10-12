FROM ocaml/opam2

ARG DEBIAN_FRONTEND=noninteractive

ENV TERM linux

USER root

RUN apt-get --allow-releaseinfo-change update && apt-get install -y --no-install-recommends apt-utils
RUN apt-get --allow-releaseinfo-change update && apt-get install -y --no-install-recommends \
      autoconf \
      m4 \
      pkg-config \
      software-properties-common \
  && sudo rm -rf /var/lib/apt/lists/*
RUN (curl -s https://www.lrde.epita.fr/repo/debian.gpg | apt-key add -) \
  && echo 'deb http://www.lrde.epita.fr/repo/debian/ stable/' >> /etc/apt/sources.list \
  && apt-get update \
  && apt-get install -y --no-install-recommends \
       spot \
       libspot-dev

USER opam

WORKDIR /home/opam

RUN git clone https://github.com/ngorogiannis/cyclist.git \
  && cd cyclist \
  && eval `opam config env` \
  && opam update \
  && opam install -y --deps-only ./cyclist.opam \
  && dune build

ENV PATH /home/opam/cyclist/_build/install/default/bin:$PATH