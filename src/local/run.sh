rm -f dump.dat result.dat
./bin/protocols.out
#module load tools/python/3.6.5-anaconda-5.2.0
cat result.dat
#python scripts/parser.py
#cat result.dat
echo "Done"
