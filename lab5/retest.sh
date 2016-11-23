#!/bin/bash
make
echo "output:"
./a.out ./testcases/test$1.tig
echo "expected:"
cat ./refs-5/test$1.out
