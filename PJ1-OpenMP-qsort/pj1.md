# PJ-1 OpenMP编程实现并行快速排序算法
这个实验我们使用C++进行OpenMP编程。

## 实现1
要实现并行快速排序，一种方法是将数组划分成不相交的若干部分交给不同的线程分别进行排序。当每个线程排序好后，我们得到num\_thread个排好序的数组。
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

随后的问题便变成对num\_thread个有序数组合并排序的问题。
```cpp
for(int i=0;i<right-left+1;i++){
  b[i]=s_arr[0].val;
  s_arr[0].idx++;
  if(s_arr[0].idx > s_arr[0].idx_max){
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
## 实现2-使用task指令动态创建任务
在快速排序中，在划分数组后，要递归调用sort函数分别去对左右的数组进行排序，一个很自然的想法是对两边并行执行。因此，我们利用\#pragma omp task动态创建一个新任务，去对左边进行排序，原进程对右边进行排序。因此我们可以写出如下的代码：

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

需要注意的是在main函数里首次调用并行快速排序函数时，我们需要使用\#pragma omp single，仅让1个线程执行该函数，否则会造成重复调用。

```cpp
#pragma omp parallel
{
  #pragma omp single
  qsort_parallel(arr2,0,arr_size-1);
}
```

## 实验方法
由于clock()计时在并行程序中会有严重的错误，我们换用chrono库进行微秒级定时，并跑500次取平均。
```cpp
// 串行
start=chrono::high_resolution_clock::now();
qsort_single(arr1,0,arr_size-1);
end=chrono::high_resolution_clock::now();
duration=chrono::duration_cast<chrono::microseconds>(end-start);
time_cost_single+=double(duration.count());
if(check(arr1,arr_size)==0)
  cout<<"sort error !"<<endl;
// 并行
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

## 实验环境
我们的实验环境如下图所示。
|    CPU    | Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz |
|:---------:|:------------------------------------------:|
|   Core    |                     8                      |
| Thread(s) per core |                  2                       |
|    Mem    |                    64 GB                   |
|     OS    |               Ubuntu 20.04.1               |
| g++ version |                 9.4.0                    |

## 结果
我们测试了程序在1K、5K、10K、100K、1M、10M等不同数据量以及在2、4、8、16个线程数情况下的加速比，实验结果如下。
|                   |   1K   |   5K   |   10K  |  100K  |   1M   |   10M  |
|:-----------------:|:------:|:------:|:------:|:------:|:------:|:------:|
| 2线程(人工划分)    |  **1.35** |  **1.46** |  **1.47** |  **1.58** |   1.68  |   1.73  |
| 2线程(动态创建任务)  |   0.37 |   0.86 |   1.11 |   1.57 |  **1.72** |  **1.79** |
| 4线程(人工划分)    | ***1.64*** | ***1.91*** | ***2.01*** | ***2.28*** |   2.56  |   2.74  |
| 4线程(动态创建任务)  |   0.30 |   0.53 |   0.87 |   2.20 |  **2.84** |  **3.03** |
| 8线程(人工划分)    |  **1.29** |  **1.52** |  **1.84** |  **2.26** |   2.81  |   3.24  |
| 8线程(动态创建任务)  |   0.20 |   0.25 |   0.37 |   1.70 | ***3.31*** | ***3.99*** |
| 16线程(人工划分)   |  **0.004** |  **0.04** |  **0.06** |  **0.62** |  **1.74** |   2.43  |
| 16线程(动态创建任务) |   0.003 |   0.02 |   0.05 |   0.40 |   1.32  |  **2.86** |


从以上结果，我们可以得到如下结论：
1.  在实验测试的数据量下，两种方法的加速比随着数据量的增加而增加，这说明并行程序的额外开销不可忽视。
2. 在数据量较小的时候，人工划分比动态创建任务的加速比更大，但在数据量较大时，动态创建任务却比人工划分快。我推测可能的原因是动态创建任务总的任务数量大于线程数量，这导致调度的开销在数据量小的时候尤其明显；而人工划分根据我们指定的线程数进行分别排序，不存在任务调度方面的开销，这使得人工划分的加速比在数据量较小的时候比较大。而在数据量较大的时候，动态创建任务在任务调度方面的开销相比计算来说较小，而人工划分的每个线程需要排序的数组长度较长，速度较慢。
3. 加速比随着线程数增加，有先上升再减少的关系，这可能与过高的线程数导致的额外开销有关。
4. 在数据量较小的时候，线程数小于等于8时，人工划分的加速比大于1，这与我们印象中不符，且与动态创建任务加速比远小于1形成强烈对比。我们推测可能是1K的数据量并不算小，并行的额外开销相比计算来说并不算大，于是我们减少数据量进行测试，可以发现2线程在150的数据量下加速比小于1，4线程在180的数据量下加速比小于1，8线程在600的数据量下加速比小于1。
