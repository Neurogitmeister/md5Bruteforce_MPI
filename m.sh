mpiexec -np 2 ./bin/md5parallel 79ec16df80b57696a03bb364410061f3 a-z,A-Z,0-9 4\
&& mpiexec -np 2 ./bin/md5parallel2 79ec16df80b57696a03bb364410061f3 a-z,A-Z,0-9 4
