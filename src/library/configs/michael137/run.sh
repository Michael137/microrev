rm -f dump.dat result.dat
LD_LIBRARY_PATH=/home/mbuch/papi/install/lib ./bin/protocols.out
#module load tools/python/3.6.5-anaconda-5.2.0
python scripts/a_parser.py
cat result.dat
echo "Done"
