# Parallel-and-Distributed-Computing-Course-PJ
Course PJ of Parallel and Distributed Computing 

## PJ-1 OpenMP编程实现并行快速排序算法
这个实验我们使用C++进行OpenMP编程。

实现1
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
实现2-使用task指令动态创建任务
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

### 实验方法
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

### 实验环境
我们的实验环境如下图所示。
|    CPU    | Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz |
|:---------:|:------------------------------------------:|
|   Core    |                     8                      |
| Thread(s) per core |                  2                       |
|    Mem    |                    64 GB                   |
|     OS    |               Ubuntu 20.04.1               |
| g++ version |                 9.4.0                    |

### 结果
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


## PJ-2 内存不共享多机环境下Parallel sorting by regular sampling算法
这个实验我们使用C++和MPI实现Parallel sorting by regular sampling算法。

### 实现
PSRS算法的简单示意图如下所示。
![psrs.png](psrs.png)

我们的实现分为4步:
1. 均匀划分+局部快速排序+正则采样
2. 归并排序正则采样数据+正则采样
3. 数据分区+分区合并
4. 归并排序

#### Step1:均匀划分+局部快速排序+正则采样
在这一步中，我们利用MPI\_Scatter进行均匀划分，每一个进程获得自己需要局部排序的数据，并进行局部快速排序，最后通过正则采样采集nprocs个主元。
```cpp
// 均匀划分
MPI_Scatter(arr,mySize,MPI_INT,myArr,mySize,MPI_INT,0,
            MPI_COMM_WORLD);
// 局部快速排序
sort(myArr,myArr+mySize);
// 正则采样
for(int i=0;i<nprocs;i++)
  sampled_pivots[i]=myArr[mySize/nprocs*i];
```

#### Step2:归并排序正则采样数据+正则采样
在这一步中，我们创建了一个数组，在根进程中接受每一个进程正则采样得到的主元，并对合起来的数组进行正则采样，最后通过MPI\_Bcast广播给所有进程。
```cpp
// 归并排序正则采样数据
int *merged_pivots=new int[nprocs*nprocs];//正则采样合起来的数组
MPI_Gather(sampled_pivots,nprocs,MPI_INT,merged_pivots,nprocs,
           MPI_INT,0,MPI_COMM_WORLD);
if(myId == 0){
  for(int step=2*nprocs;step<=nprocs*nprocs;step*=2)
    for(int i=0;i<nprocs*nprocs;i+=step)
      Merge(merged_pivots,i,i+step/2-1,i+step-1);
  // 正则采样  
  for(int i=0;i<nprocs-1;i++)
    sampled_pivots_2[i]=merged_pivots[(i+1)*nprocs];
}
MPI_Bcast(sampled_pivots_2,nprocs-1,MPI_INT,0,MPI_COMM_WORLD);
```


#### Step3:数据分区+分区合并
在这一步中，我们计算每个进程根据选取的主元得到的每个划分的分区大小，利用MPI\_Alltoall广播需要从每个进程接受的数组大小，并计算接收数组长度之和。然后我们创建一个长度符合要求的数组，计算发送缓冲区和接收缓冲区中每个数据块起始位置的偏移量，并利用MPI\_Alltoallv进行分区合并。
```cpp
// 数据分区
int proc_id=0;  // 分区所属proc
memset(sendcounts,0,sizeof(int)*nprocs);     // 分区长度清零
for(int i=0;i<mySize;i++){
  while (proc_id<nprocs-1 & myArr[i]>sampled_pivots_2[proc_id]) 
    proc_id+=1;     // 找到当前i所属的分区
  if(proc_id==nprocs-1){     
    sendcounts[nprocs-1]=mySize-i;    // 最后一个分区大小
    break;
  }
  sendcounts[proc_id]++;     //i归入所属分区，分区大小+1
}
// 得到需要从每个进程接受的数组大小
MPI_Alltoall(sendcounts,1,MPI_INT,recvcounts,1,MPI_INT,
             MPI_COMM_WORLD);
for(int i=0;i<nprocs;i++) 
  *mySize2+=recvcounts[i];
*myArr2=new int[*mySize2];
// 计算发送缓冲区和接收缓冲区中每个数据块起始位置的偏移量
int *sdispls=new int[nprocs];//表示发送缓冲区中每个数据块的偏移量,第i个元素表示当前进程要发送给第i个进程的数据块在发送缓冲区中的起始位置
sdispls[0]=0; 
rdispls[0]=0;
for(int i=1;i<nprocs;i++){
  sdispls[i]=sendcounts[i-1]+sdispls[i-1];
  rdispls[i]=recvcounts[i-1]+rdispls[i-1];
}
// 分区合并
MPI_Alltoallv(myArr,sendcounts,sdispls,MPI_INT,*myArr2,recvcounts,rdispls,MPI_INT,MPI_COMM_WORLD);
```


#### Step4:归并排序
在这一步中，每个进程对自己nprocs个已经排好序的数组进行归并排序，然后计算根进程的接收缓冲区中每个数据块的偏移量，并利用MPI\_Gatherv收集排序好的数组到根进程。
```cpp
// 计算每个分区结尾位置+1的位置
int* myPartEnd=new int[nprocs]; 
for(int i=1;i<nprocs;i++)
  myPartEnd[i-1]=rdispls[i];
myPartEnd[nprocs-1]=mySize2;
// 归并排序
int* mySortedArr2=new int[mySize2];
for(int i=0;i<mySize2;i++){
  int cur_min=INT_MAX; // 当前循环最小值
  int proc_id=-1;  // 当前循环最小值所属分区号
  for(int j=0;j<nprocs;j++){
    if((rdispls[j]<myPartEnd[j])&(myArr2[rdispls[j]]<=cur_min)){
      cur_min=myArr2[rdispls[j]];
      proc_id=j;
    }
  }
  mySortedArr2[i]=cur_min;
  rdispls[proc_id]+=1;
}
// 收集排序好的数组到根进程
int* recvbuf=new int[nprocs];  //指向根进程的缓冲区的指针,存放所有进程发送的子列表大小
MPI_Gather(&mySize2,1,MPI_INT,recvbuf,1,MPI_INT,0,MPI_COMM_WORLD);
// 计算根进程的接收缓冲区中每个数据块的偏移量
if(myId == 0){
  rdispls[0]=0;
  for(int i=1;i<nprocs;i++)
    rdispls[i]=recvbuf[i-1]+rdispls[i-1];
}
MPI_Gatherv(mySortedArr2,mySize2,MPI_INT,arr,recvbuf,rdispls,MPI_INT,0,MPI_COMM_WORLD);
```

### 实验方法
我们同样使用chrono库进行微秒级定时，并跑500次取平均。对于多进程，我们使用MPI\_Reduce获取用时最长进程的耗时以得到正确的耗时。
```cpp
MPI_Reduce(&time_cost_parallel,&maxTime,1,MPI_DOUBLE,MPI_MAX,
           0,MPI_COMM_WORLD);
  ```
我们观察到在nprocs等于16时，只利用根进程进行快速排序时与不用mpi的环境中进行串行排序时间差距较大，因此我们另外使用一个不用mpi编程的串行排序作为基准计算加速比。

### 实验环境
我们的实验环境如下图所示。
| 项目               | 详情              |
|--------------------|-------------------|
| CPU                | AMD Ryzen 7 5800H |
| Core               | 8                 |
| Thread(s) per core | 2                 |
| Mem                | 16 GB             |
| OS                 | Ubuntu 20.04.1    |
| MPICH Version      | 4.0               |

### 结果
我们测试了程序在1K、5K、10K、100K、1M、10M等不同数据量以及在2、4、8、16个线程数情况下的加速比，实验结果如下。
|        | 1K   | 5K   | 10K  | 100K | 1M   | 10M  |
|--------|------|------|------|------|------|------|
| 2线程   | **0.76** | 1.33 | 1.41 | 1.35 | 1.60 | 1.87 |
| 4线程   | 0.47 | **1.64** | **2.17** | 2.03 | 2.88 | 3.41 |
| 8线程   | 0.15 | 0.70 | 1.26 | **2.50** | 3.81 | 4.67 |
| 16线程 | 0.05 | 0.28 | 0.38 | 1.61 | **3.85** | **5.00** |

从以上结果，我们可以得到如下结论：
1. 在实验测试的数据量下，固定线程数，加速比随着数据量的增加而增加，这说明并行程序的额外开销不可忽视。
2. 在数据量较小的时候，串行算法本身就已经足够快速，而并行算法额外的启动以及通信等一系列开销使得并行算法比串行算法还慢。这一问题随着线程数增加更为明显。
3. 并行算法最大加速比对应的线程数随着数据量的增加而增加。这说明了增大数据量才能更好地抵消更多线程所带来的额外开销。
4. 在数据量较大的时候，并行算法展现出了非常大的优势，在16线程、10M数据量的情况下，加速比甚至达到了5。此外，PSRS算法的效率也很高，在2/4线程、10M数据量下，加速比十分接近于线程数，这说明了选取合适主元的重要性。


## 使用Hadoop框架MapReduce编程模型实现WordCount算法




