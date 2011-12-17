for f in `ls *.cpp *.h`
do
    cp $f $f.txt
# do something on $f
done
