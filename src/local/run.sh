rm -f dump.dat result.dat
LD_LIBRARY_PATH=/home/gardei/papi/install/lib ./bin/test/reader_writer_pingpong.out
exit
module load tools/python/3.6.5-anaconda-5.2.0
python scripts/parser.py
cat result.dat
#python scripts/parser.py
#cat result.dat
echo "Done"
