echo $1
echo $2

for (( i=0; i<$2; i++ )); do
./bzip2 -dkf $1
done
