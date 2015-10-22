echo -n a1\ 
./l1.sh ./threads 16 a
echo -n a2\ 
./l1.sh ./threads 128 a

echo -n s1\ 
./l1.sh ./threads 16 s
echo -n s2\ 
./l1.sh ./threads 128 s

echo -n r1\ 
./l1.sh ./threads 16 r
echo -n r2\ 
./l1.sh ./threads 128 r
