rmdir -rf out
mkdir out
echo "executing pm (FCFS)"
./pm 1 > out/out1 
tail out/out1 -n 5 >> out/complete

echo "executing pm (SRTF)"
./pm 2 > out/out2 
tail out/out2 -n 5 >> out/complete

echo "executing pm (RR) quantum 10ms"
./pm 3 10 > out/out3
tail out/out3 -n 5 >> out/complete

echo "executing pm (RR) quantum 20ms"
./pm 3 20 > out/out4
tail out/out4 -n 5 >> out/complete

echo "executing pm (RR) quantum 50ms"
./pm 3 50 > out/out5
tail out/out5 -n 5 >> out/complete

echo "executing pm (RR) quantum 250ms"
./pm 3 250 > out/out6
tail out/out6 -n 5 >> out/complete

echo "executing pm (RR) quantum 500ms"
./pm 3 500 > out/out7
tail out/out7 -n 5 >> out/complete

echo "complete"
