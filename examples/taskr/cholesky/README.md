Cholesky Decomposition Sandbox
=======================================

A Cholesky sandbox to test task dependencies and compare runtime system implementations  

Running
-----------

```
./run_all.sh N bs 

where:
 * N = Matrix Size
 * bs = Block Size (where applicable)
 * [--check] to verify residual
```

examples:

``` 
./run_all.sh 2048 128
./run_all.sh 2048 128 --check
```



