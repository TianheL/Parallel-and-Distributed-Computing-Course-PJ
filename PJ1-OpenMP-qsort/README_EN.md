# PJ-1 OpenMP Implementation of Parallel QuickSort Algorithm
*Read this in [Chinese](pj1.md)*

We used C++ for OpenMP programming in this experiment.

## Implement 1-Parallel Sorting by Manual Partitioning

One way to achieve parallel quicksort is to divide the array into non-overlapping parts and have different threads sort them separately. When each thread has finished sorting, we have num_thread sorted arrays.
```cpp
int* right_idx=new int[thread_num];
for(int i=0;i<thread_num-1;i++) {
  right_idx[i]=(right-left+1)/thread_num*(i+1);
}
right_idx[thread_num-1]=right;

#pragma omp parallel num_threads(thread_num)
{
  int idx=omp_get_thread_num();
  qsort_single(arr,(idx==0?0:right_idx[idx-1]+1),right_idx[idx]);
}
```

The subsequent problem then becomes a problem of merge sorting num_thread ordered arrays.
```cpp
for(int i=0;i<right-left+1;i++){
  b[i]=s_arr[0].val;
  s_arr[0].idx++;
  if(s_arr[0].idx>s_arr[0].idx_max){
    t_left--;
    for(j=0;j<t_left;j++) 
      s_arr[j]=s_arr[j+1];
  }
  else{
    s_arr[0].val=arr[s_arr[0].idx];
    temp=s_arr[0];
    for(j=1;j<t_left;j++){
      if(s_arr[j].val<=temp.val)
        s_arr[j-1]=s_arr[j];
      else
        break;
      s_arr[j]=temp;
    }
  }
}
```
## Implement2-Using the *task* Command to Dynamically Create Tasks
In quicksort, after dividing the array, we need to recursively call the sort function to sort the left and right arrays separately. It's natural to think of doing this in parallel for both sides. Therefore, we use \#pragma omp task to dynamically create a new task to sort the left side, while the original process sorts the right side. As a result, we can write the following code:

```cpp
void qsort_parallel(int* arr,int left,int right){
  if(left<right){
    int part = partition(arr,left,right);
    #pragma omp task
      qsort_parallel(arr,left,part-1);
    qsort_parallel(arr,part+1,right);
  }
}
```

The key thing to note is that when calling the parallel quicksort function for the first time in the main function, we need to use \#pragma omp single to ensure that only one thread executes the function. Otherwise, it will result in duplicate calls.

```cpp
#pragma omp parallel
{
  #pragma omp single
  qsort_parallel(arr2,0,arr_size-1);
}
```

## Experimental Method
Due to serious errors with clock() timing in parallel programs, we switch to using the chrono library for microsecond-level timing and run it 500 times to take the average.
```cpp
// Serial
start=chrono::high_resolution_clock::now();
qsort_single(arr1,0,arr_size-1);
end=chrono::high_resolution_clock::now();
duration=chrono::duration_cast<chrono::microseconds>(end-start);
time_cost_single+=double(duration.count());
if(check(arr1,arr_size)==0)
  cout<<"sort error !"<<endl;
// Parallel
start=chrono::high_resolution_clock::now();
#pragma omp parallel
{
  #pragma omp single
  qsort_parallel(arr2,0,arr_size-1);
}
end=chrono::high_resolution_clock::now();
duration=chrono::duration_cast<chrono::microseconds>(end-start);
time_cost_parallel+=double(duration.count());
```

You can compile using the following command.
```
g++ lab1-1.cpp -o lab1-1 -fopenmp
g++ lab1-2.cpp -o lab1-2 -fopenmp
```

### Single Round Test
```
./lab1-1 {Num_Thread} {Array_Length}
./lab1-2 {Num_Thread} {Array_Length}
```

### Multi-Round Test
```
bash lab2-1.sh
bash lab2-2.sh
```

## Experiment Environment
Our experimental environment is shown in the figure below.
|    CPU    | Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz |
|:---------:|:------------------------------------------:|
|   Core    |                     8                      |
| Thread(s) per core |                  2                       |
|    Mem    |                    64 GB                   |
|     OS    |               Ubuntu 20.04.1               |
| g++ version |                 9.4.0                    |

## Result
We test the program's speedup ratio under different data volumes such as 1K, 5K, 10K, 100K, 1M, 10M, and different thread numbers including 2, 4, 8, and 16. The experimental results are as follows.
|                   |   1K   |   5K   |   10K  |  100K  |   1M   |   10M  |
|:-----------------:|:------:|:------:|:------:|:------:|:------:|:------:|
| 2线程(Manual Partition)  |  **1.35** |  **1.46** |  **1.47** |  **1.58** |   1.68  |   1.73  |
| 2线程(Dynamic Task)  |   0.37 |   0.86 |   1.11 |   1.57 |  **1.72** |  **1.79** |
| 4线程(Manual Partition)    | ***1.64*** | ***1.91*** | ***2.01*** | ***2.28*** |   2.56  |   2.74  |
| 4线程(Dynamic Task)  |   0.30 |   0.53 |   0.87 |   2.20 |  **2.84** |  **3.03** |
| 8线程(Manual Partition)    |  **1.29** |  **1.52** |  **1.84** |  **2.26** |   2.81  |   3.24  |
| 8线程(Dynamic Task)  |   0.20 |   0.25 |   0.37 |   1.70 | ***3.31*** | ***3.99*** |
| 16线程(Manual Partition)   |  **0.004** |  **0.04** |  **0.06** |  **0.62** |  **1.74** |   2.43  |
| 16线程(Dynamic Task) |   0.003 |   0.02 |   0.05 |   0.40 |   1.32  |  **2.86** |


From the above results, we can draw the following conclusions:
1. Under the experimental test data, the speedup ratio of the two methods increases with the increase of data volume, indicating that the additional overhead of parallel programs cannot be ignored.
2. When the data volume is small, manual partitioning has a greater speedup ratio than dynamically creating tasks, but when the data volume is large, dynamically creating tasks is faster than manual partitioning. I speculate that the reason may be that the total number of tasks dynamically created is greater than the number of threads, which results in particularly obvious scheduling overhead when the data volume is small; while manual partitioning sorts according to the number of threads we specify, there is no task scheduling overhead, which makes the speedup ratio of manual partitioning relatively large when the data volume is small. However, when the data volume is large, the scheduling overhead of dynamically creating tasks is relatively small compared to the calculations, and each thread of manual partitioning needs to sort a longer array length, resulting in slower speed.
3. The speedup ratio increases and then decreases as the number of threads increases, which may be related to the additional overhead caused by excessive number of threads.
4. When the data volume is small, the speedup ratio of manual partitioning is greater than 1 when the number of threads is less than or equal to 8, which is contrary to our impression, and forms a strong contrast with the speedup ratio of dynamically creating tasks far less than 1. We speculate that perhaps a data volume of 1K is not considered small, and the additional overhead of parallelism is not large compared to the calculation, so we reduced the data volume for testing, and found that the speedup ratio of 2 threads at 150 data volume is less than 1, the speedup ratio of 4 threads at 180 data volume is less than 1, and the speedup ratio of 8 threads at 600 data volume is less than 1.
