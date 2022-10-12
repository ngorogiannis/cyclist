# Cyclist

*Cyclist* is a framework for building cyclic theorem provers based on a sequent calculus.
In addition, over the years several decision procedures or algorithms have been integrated,
focusing on Separation Logic with inductively defined predicates.

For information on building *Cyclist*, the available tools included and the papers underpinning those tools, visit

[www.cyclist-prover.org](http://www.cyclist-prover.org)

## ESOP 2023 Paper Submission

This version of the repository contains the code referred to in the submission entitled "Fermat without Büchi: Non-Automata-Based Criteria for Infinite Descent in Cyclic Proofs".

The most straightforward way to run this code is using Docker.

This directory contains a Dockerfile that can be used to build an image in which
the source code and all its dependencies are pre-compiled.

To build the Docker image, run the following command from this directory:

```[shell]
  > docker build --file=Dockerfile.ESOP2023 -t cyclist .
```

An interactive container of this image can then be run with the following command:

```[shell]
  > docker run -ti cyclist
```
