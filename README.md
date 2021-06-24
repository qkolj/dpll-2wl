# dpll-2wl

A DPLL SAT solver with two watched literals (2WL) optimization

---

## Usage

The solver expects a SAT problem encoded as a CNF formula in DIMACS file format as the first argument. It outputs either ```UNSAT``` if given formula is unsatisfiable, or ```SAT``` and valuation that satisfies given formula. ```dpll-2wl-debug``` outputs a debug message for every step during the DPLL procedure. Example usage:

```console
user@host:dpll-2wl$ ./dpll-2wl examples/aim-50-1_6-yes1-4.cnf 
SAT
-1 2 -3 -4 -5 6 -7 -8 -9 10 11 -12 -13 14 15 16 -17 18 19 20 -21 22 23 24 25 -26 -27 
  ↪ -28 -29 -30 -31 32 -33 -34 35 36 -37 -38 39 40 -41 42 43 44 -45 46 47 -48 -49 50
user@host:dpll-2wl$ ./dpll-2wl examples/hole6.cnf 
UNSAT

```

---

Some of the examples are taken from https://people.sc.fsu.edu/~jburkardt/data/cnf/cnf.html
