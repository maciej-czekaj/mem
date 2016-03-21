for i in {1,2,4,8,16,32,64,128,256} ; do echo -n "$i "; ./ring $i ; done | tee ring.txt
