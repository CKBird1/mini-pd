# Status

Date: 2026-09-07

Most of basic flow done, testing basic but done, parser basic but done, HPWL and Random done, working on Snap now.

Date: 2026-09-08

Snap done. Beginning work on quadratic / force placer then abacus.
Quadratic done. Beginning work on Abacus.
Abacus tetris done, beginning work on clusters for true abacus.

Date: 2026-09-09

Finished true abacus using clusters. HPWL has improved.
Bookshelf v1 reader in. Gold: tiny/medium/large .aux match .bench.
large.aux 10-seed mean HPWL (200 cells):

  path               HPWL  legal
  quadratic+abacus  12099  200/200  wins 10/10
  random+snap       20624  200/200  +70%
  quadratic+snap    20924   92/200  overflow
  random+abacus     46527  200/200  +285%

Snap after random beats Abacus on HPWL only because left-pack
collapses cells toward x=0. Not a better legalizer once quadratic
clusters.
