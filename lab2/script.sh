rmdir -rf out
mkdir out
echo "executing pm (FCFS)"
./pm 1 > out/out1 
echo "grabbing tail of output"
tail out/out1 -n 4 >> out/complete
echo "\n" >> out/complete

echo "executing pm (SRTF)"
./pm 2 > out/out2 
echo "grabbing tail of output"
tail out/out12-n 4 >> out/complete
echo "\n" >> out/complete


echo "executing pm (RR) quantum 10ms"
./pm 3 10 > out/out3
echo "grabbing tail of output"
tail out/out3 -n 4 >> out/complete
echo "\n" >> out/complete


echo "executing pm (RR) quantum 20ms"
./pm 3 20 > out/out3
echo "grabbing tail of output"
tail out/out3 -n 4 >> out/complete
echo "\n" >> out/complete

echo "executing pm (RR) quantum 50ms"
./pm 3 50 > out/out3
echo "grabbing tail of output"
tail out/out3 -n 4 >> out/complete
echo "\n" >> out/complete

echo "executing pm (RR) quantum 250ms"
./pm 3 250 > out/out3
echo "grabbing tail of output"
tail out/out3 -n 4 >> out/complete
echo "\n" >> out/complete

echo "executing pm (RR) quantum 500ms"
./pm 3 500 > out/out3
echo "grabbing tail of output"
tail out/out3 -n 4 >> out/complete
echo "\n" >> out/complete

echo "complete"
