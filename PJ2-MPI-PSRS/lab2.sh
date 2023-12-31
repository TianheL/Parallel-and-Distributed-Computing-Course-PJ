for N in {1000,5000,10000,100000,1000000,10000000}
do  
for nprocs in {2,4,8,16}  
do

total_sum1=0
total_sum2=0
total_sum3=0

for ((i=0; i<500; i++))
do
    output=$(mpirun -n $nprocs ./lab2-test $N $i)
    num1=$(echo $output | awk '{print $1}')   
    num2=$(echo $output | awk '{print $2}')   
    total_sum1=$((total_sum1 + num1))  # 累加单线程 
    total_sum2=$((total_sum2 + num2))  # 累加多线程 
    output=$(./lab2-base $N $i)
    num3=$(echo $output | awk '{print $1}')
    total_sum3=$((total_sum3 + num3))  # 累加base
done

result=$(awk "BEGIN {printf \"%.2f\", $total_sum3 / $total_sum2}")  

echo "N: $N"
echo "nprocs: $nprocs"
echo "单线程总和为: $total_sum1"
echo "多线程总和为: $total_sum2"
echo "base 总和为: $total_sum3"
echo "加速比为: $result"
echo "----------------------"

done
done