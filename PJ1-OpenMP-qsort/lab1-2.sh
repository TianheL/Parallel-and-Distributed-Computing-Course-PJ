for arr_size in {1000,5000,10000,100000,1000000,10000000}
do  
for num_thread in {2,4,8,16}  
do  
echo $num_thread
echo $arr_size
./lab1-2 $num_thread $arr_size 
done  
done








