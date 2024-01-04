# PJ-2 Parallel Sorting by Regular Sampling Algorithm in a Non-Shared Memory Multi-Machine Environment
*Read this in [Chinese](pj2.md)*

We use C++ and MPI to implement the Parallel Sorting by Regular Sampling algorithm in this experiment.

## Implement
The simple schematic diagram of the PSRS algorithm is as follows.
![psrs.png](psrs.png)

Our implementation is divided into 4 steps:
1. Uniform division + Local quick sort + Regular sampling
2. Merge sort regular sampling data + Regular sampling
3. Data partition + Partition merge
4. Merge sort

## Step1:Uniform division + Local quick sort + Regular sampling
In this step, we use MPI\_Scatter to uniformly divide the data, so that each process gets their own data to sort, and then perform local quicksort. Finally, we collect nprocs pivot elements through regular sampling.
```cpp
// Uniform division
MPI_Scatter(arr,mySize,MPI_INT,myArr,mySize,MPI_INT,0,MPI_COMM_WORLD);
// Local quick sort
sort(myArr,myArr+mySize);
// Regular sampling
for(int i=0;i<nprocs;i++)
  sampled_pivots[i]=myArr[mySize/nprocs*i];
```

## Step2:Merge sort regular sampling data + Regular sampling
In this step, we create an array, obtain the main elements from each process through regular sampling in the root process, regularly sample the combined array, and finally broadcast it to all processes through MPI\_Bcast.
```cpp
// Merge sort regular sampling data
int *merged_pivots=new int[nprocs*nprocs];  // An array composed of regular sampled main elements from each process
MPI_Gather(sampled_pivots,nprocs,MPI_INT,merged_pivots,nprocs,MPI_INT,0,MPI_COMM_WORLD);
if(myId == 0){
int *merged_pivots_2=new int[nprocs*nprocs]; // An sorted array
int *offset=new int[nprocs]; // Store pointer offset
memset(offset,0,sizeof(int)*nprocs);     // Set zero
for(int i=0;i<nprocs*nprocs;i++){
int cur_min=INT_MAX; // Current minimum value of the loop
int proc_id=-1;  // Partition number to which the current minimum value belongs.
for(int j=0;j<nprocs;j++){
    if((offset[j]<nprocs) & (merged_pivots[j*nprocs+offset[j]]<=cur_min)){
        cur_min=merged_pivots[j*nprocs+offset[j]];
        proc_id=j;
    }
}
merged_pivots_2[i]=cur_min;
offset[proc_id]+=1;
}
// Regular sampling  
for(int i=0;i<nprocs-1;i++)
sampled_pivots_2[i]=merged_pivots_2[(i+1)*nprocs];
delete []merged_pivots_2;
delete []offset;
delete []merged_pivots;
}
MPI_Bcast(sampled_pivots_2,nprocs-1,MPI_INT,0,MPI_COMM_WORLD); 
```


## Step3:Data partition + Partition merge
In this step, we calculate the size of each partition in each process based on the selected pivot, use MPI\_Alltoall to broadcast the array sizes that need to be received from each process, and calculate the sum of the lengths of the receiving arrays. Then we create an array of the required length, calculate the offset of the starting position of each data block in the send and receive buffers, and use MPI\_Alltoallv to merge the partitions.
```cpp
// Data partition
int proc_id=0;  // The proc_id of current partition
int* sendcounts=new int[nprocs]; // The size of the data blocks that the current process needs to send to other processes, where the i-th element represents the size of the data block that the current process needs to send to the i-th process.
memset(sendcounts,0,sizeof(int)*nprocs);     // Set partition length to zero
for(int i=0;i<mySize;i++){
    while (proc_id<nprocs-1 & myArr[i]>sampled_pivots_2[proc_id]) 
        proc_id+=1;     // Find the partition that i currently belongs to.
    if(proc_id==nprocs-1){     
        sendcounts[nprocs-1]=mySize-i;    // Last partition size
        break;
    }
    sendcounts[proc_id]++;     // i belongs to the assigned section，partition size +1
}
// Get the size of the array needed from each process.
MPI_Alltoall(sendcounts,1,MPI_INT,recvcounts,1,MPI_INT,MPI_COMM_WORLD);
for(int i=0;i<nprocs;i++) 
*mySize2+=recvcounts[i];
*myArr2=new int[*mySize2];
// Calculate the offset of the starting position for each data block in the send buffer and receive buffer.
int *sdispls=new int[nprocs];// This represents the offset of each data block in the send buffer, with the i-th element indicating the starting position of the data block that the current process is sending to the i-th process.
sdispls[0]=0; 
rdispls[0]=0;
for(int i=1;i<nprocs;i++){
    sdispls[i]=sendcounts[i-1]+sdispls[i-1];
    rdispls[i]=recvcounts[i-1]+rdispls[i-1];
}
// Partition merge
MPI_Alltoallv(myArr,sendcounts,sdispls,MPI_INT,*myArr2,recvcounts,rdispls,MPI_INT,MPI_COMM_WORLD);
delete []sdispls;
delete []sendcounts;
```


## Step4:Merge sort
In this step, each process performs a merge sort on its own nprocs sorted arrays, then calculates the offset of each data block in the receiving buffer of the root process, and uses MPI\_Gatherv to gather the sorted arrays to the root process.
```cpp
// Calculate the position of the end of each partition +1
int* myPartEnd=new int[nprocs]; 
for(int i=1;i<nprocs;i++)
    myPartEnd[i-1]=rdispls[i];
myPartEnd[nprocs-1]=mySize2;
// Merge Sort
int* mySortedArr2=new int[mySize2];
for(int i=0;i<mySize2;i++){
    int cur_min=INT_MAX; // Current minimum value in the loop
    int proc_id=-1;  // Partition number to which the current loop's minimum value belongs
    for(int j=0;j<nprocs;j++){
        if((rdispls[j]<myPartEnd[j]) & (myArr2[rdispls[j]]<=cur_min)){
            cur_min=myArr2[rdispls[j]];
            proc_id=j;
        }
    }
    mySortedArr2[i]=cur_min;
    rdispls[proc_id]+=1;
}
delete []myPartEnd;
// Collect sorted arrays to the root process
int* recvbuf=new int[nprocs];  // Pointer to the buffer of the parent process, storing the size sent by each process.
MPI_Gather(&mySize2,1,MPI_INT,recvbuf,1,MPI_INT,0,MPI_COMM_WORLD);
// Calculate the offset of each data block in the root process's receive buffer.
if(myId == 0){
    rdispls[0]=0;
    for(int i=1;i<nprocs;i++)
        rdispls[i]=recvbuf[i-1]+rdispls[i-1];
}
MPI_Gatherv(mySortedArr2,mySize2,MPI_INT,arr,recvbuf,rdispls,MPI_INT,0,MPI_COMM_WORLD);
delete []mySortedArr2;
delete []recvbuf;
```

## Experimental method
We also use the chrono library for microsecond-level timing, and run 500 times to get the average. For multiprocessing, we use MPI\_Reduce to obtain the time consumption of the longest-running process in order to obtain the correct time consumption.
```cpp
MPI_Reduce(&time_cost_parallel,&maxTime,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
  ```
We find that when nprocs is equal to 16, there is a significant time difference between using only the root process for quick sorting and serial sorting in an environment without mpi. Therefore, we use a separate serial sorting without mpi programming as a baseline to calculate the speedup ratio.

```
--lab2.cpp Multithreading Timer
--lab2-base.cpp Used for single-thread timing test (serial sorting without mpi programming as baseline)
--lab2-test.cpp Used for multi-thread timing test (same as lab2.cpp, but removed some output for script timing)
--lab2.sh Test script
```

Please make sure MPICH/OpenMPI is installed. If not installed, you can install it using the following commands.
```terminal
sudo apt-get install mpich
```

### Single-Round Test
You can compile using the following code.
```terminal
mpic++ -o lab2 lab2.cpp
```
You can start the test with the following code:
```bash
mpirun -n {Num_procs} ./lab2-1 {Array_Length} {seed}
```

### Multi-Round Test
You can compile using the following code.
```terminal
mpic++ -o lab2-test lab2-test.cpp
g++ -o lab2-base lab2-base.cpp
```

You can start the test with the following code:
```bash
bash lab2.sh
```

## Experiment Environment
Our experiment environment is shown in the following table.
| Component               | Type        |
|--------------------|-------------------|
| CPU                | AMD Ryzen 7 5800H |
| Core               | 8                 |
| Thread(s) per core | 2                 |
| Mem                | 16 GB             |
| OS                 | Ubuntu 20.04.1    |
| MPICH Version      | 4.0               |

## Result
We test the program's speedup ratio under different data sizes such as 1K, 5K, 10K, 100K, 1M, 10M, and different numbers of threads like 2, 4, 8, 16. The experimental results are as follows.
|        | 1K   | 5K   | 10K  | 100K | 1M   | 10M  |
|--------|------|------|------|------|------|------|
| 2 Thread   | **0.76** | 1.33 | 1.41 | 1.35 | 1.60 | 1.87 |
| 4 Thread   | 0.47 | **1.64** | **2.17** | 2.03 | 3.02 | 3.41 |
| 8 Thread   | 0.15 | 0.70 | 1.26 | **2.50** | 3.91 | 4.67 |
| 16 Thread | 0.05 | 0.28 | 0.38 | 1.61 | **4.18** | **5.00** |

From the above results, we can draw the following conclusions:
1. Under the experimental test data, with a fixed number of threads, the speedup ratio increases as the data volume increases. This indicates that the additional overhead of parallel programs cannot be ignored.
2. When the data volume is small, the serial algorithm itself is already fast enough, and the additional overhead of parallel algorithms such as startup and communication makes the parallel algorithm slower than the serial algorithm. This problem becomes more noticeable as the number of threads increases.
3. The number of threads corresponding to the maximum speedup of parallel algorithms increases with the increase of data volume. This indicates that increasing the data volume is necessary to better offset the additional overhead of more threads.
4. When the data volume is large, parallel algorithms demonstrate a significant advantage. For example, with 16 threads and a data volume of 10M, the speedup ratio can even reach 5. In addition, the efficiency of the PSRS algorithm is also very high. With 2/4 threads and a data volume of 10M, the speedup ratio is very close to the number of threads, which highlights the importance of selecting appropriate pivot elements.
