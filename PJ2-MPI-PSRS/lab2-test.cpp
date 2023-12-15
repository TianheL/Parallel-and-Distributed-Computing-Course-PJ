#include<ctime>
#include<iostream>
#include "mpi.h"
#include<chrono>
#include<climits>
#include<algorithm>
#include<cstring>
#include<unistd.h>
using namespace std;

void rand_array(int* arr1,int* arr2,int n) {
	for(int i = 0; i < n; i++) {
		arr2[i]=arr1[i] = rand();
	}
}

// step1: 均匀划分+局部快速排序+正则采样
void step1(int *arr,int *myArr,int mySize,int *sampled_pivots,int nprocs){
    // 均匀划分
    MPI_Scatter(arr,mySize,MPI_INT,myArr,mySize,MPI_INT,0,MPI_COMM_WORLD);
    // 局部快速排序
    sort(myArr,myArr+mySize);
    // 正则采样
    for(int i=0;i<nprocs;i++)
        sampled_pivots[i]=myArr[mySize/nprocs*i];
}

// step2: 归并排序正则采样数据+正则采样
void step2(int *sampled_pivots,int *sampled_pivots_2,int nprocs,int myId){
    // 归并排序正则采样数据
    int *merged_pivots=new int[nprocs*nprocs];  // 每个划分正则采样合起来的数组
    MPI_Gather(sampled_pivots,nprocs,MPI_INT,merged_pivots,nprocs,MPI_INT,0,MPI_COMM_WORLD);
    if(myId == 0){
        int *merged_pivots_2=new int[nprocs*nprocs]; // 放排好序的数组
        int *offset=new int[nprocs]; //存放指针偏移量
        memset(offset,0,sizeof(int)*nprocs);     // 清零
        for(int i=0;i<nprocs*nprocs;i++){
            int cur_min=INT_MAX; // 当前循环最小值
            int proc_id=-1;  // 当前循环最小值所属分区号
            for(int j=0;j<nprocs;j++){
                if((offset[j]<nprocs) & (merged_pivots[j*nprocs+offset[j]]<=cur_min)){
                    cur_min=merged_pivots[j*nprocs+offset[j]];
                    proc_id=j;
                }
            }
            merged_pivots_2[i]=cur_min;
            offset[proc_id]+=1;
        }
        // 正则采样  
        for(int i=0;i<nprocs-1;i++)
            sampled_pivots_2[i]=merged_pivots_2[(i+1)*nprocs];
        delete []merged_pivots_2;
        delete []offset;
        delete []merged_pivots;
    }
    MPI_Bcast(sampled_pivots_2,nprocs-1,MPI_INT,0,MPI_COMM_WORLD);  
}

// step3: 数据分区+分区合并
void step3(int *myArr,int mySize,int* sampled_pivots_2,int **myArr2,int* mySize2,int *recvcounts,int* rdispls,int nprocs){
    // 数据分区
    int proc_id=0;  // 分区所属proc
    int* sendcounts=new int[nprocs]; // 当前进程要发送给其他进程的数据块大小，第i个元素表示当前进程要发送给第i个进程的数据块大小
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
    MPI_Alltoall(sendcounts,1,MPI_INT,recvcounts,1,MPI_INT,MPI_COMM_WORLD);
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
    delete []sdispls;
    delete []sendcounts;
}

// step4: 归并排序
void step4(int *myArr2,int mySize2,int *recvcounts,int* rdispls,int nprocs,int myId,int *arr){
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
            if((rdispls[j]<myPartEnd[j]) & (myArr2[rdispls[j]]<=cur_min)){
                cur_min=myArr2[rdispls[j]];
                proc_id=j;
            }
        }
        mySortedArr2[i]=cur_min;
        rdispls[proc_id]+=1;
    }
    delete []myPartEnd;
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
    delete []mySortedArr2;
    delete []recvbuf;
}

int main(int argc,char *argv[]){
    // mpirun -n nprocs ./lab2-1 arr_len seed
    srand(atoi(argv[2])); 
    MPI_Init(&argc,&argv);
    int N=atoi(argv[1]);
    int nprocs,myId;
    auto start = chrono::high_resolution_clock::now();
	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>(end-start);
    double time_cost_single,time_cost_parallel;
    int *arr1=NULL;
    int *arr2=NULL;
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myId);
    if(myId==0){
        arr1=new int[N];
		arr2=new int[N];
        rand_array(arr1,arr2,N);
        // 单进程
		start = chrono::high_resolution_clock::now();
		sort(arr2,arr2+N);
		end = chrono::high_resolution_clock::now();
		duration = chrono::duration_cast<chrono::microseconds>(end - start);
        time_cost_single=double(duration.count());
        // cout << "Cost (Single Thread): " << time_cost_single << "μs" << endl;
        if(N==10000000)
            cout<<int(time_cost_single/100)<<endl;
        else
            cout << time_cost_single << endl;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    start = chrono::high_resolution_clock::now();
    int *arr1_add;
    int add_num=0;
    if(N%nprocs!=0)
        add_num=((N/nprocs)+1)*nprocs-N;
    if(myId==0){
        if(add_num>0){
            arr1_add=new int[N+add_num];
            memcpy(arr1_add,arr1,N*sizeof(int));
            for(int i=0;i<add_num;i++)
                arr1_add[N+i]=INT_MAX;
        }
        else
            arr1_add=arr1;
    }        
    // step1: 均匀划分+局部快速排序+正则采样
    int mySize=(N+add_num)/nprocs;    // 每个划分的大小
    int* myArr=new int[mySize];    // 每个划分的数组
    int* sampled_pivots=new int[nprocs];    // 每个划分的正则采样的主元数组
    step1(arr1_add,myArr,mySize,sampled_pivots,nprocs);
    // step2: 归并采样数据+正则采样
    int *sampled_pivots_2=new int[nprocs-1];    // 第二次正则采样数组
    step2(sampled_pivots,sampled_pivots_2,nprocs,myId);
    delete []sampled_pivots;
    // step3: 数据分区+分区合并
    
    int* recvcounts=new int[nprocs];   // 每个进程将要接收的数据块大小,第i个元素表示从第i个进程将要接收的数据块大小
    int* myArr2;    // 当前进程分区合并后的数组
    int mySize2=0;  // 当前进程分区合并后的数组
    int *rdispls=new int[nprocs];//表示接收缓冲区中每个数据块的偏移量,第i个元素表示从第i个进程接收到的数据块在接收缓冲区中的起始位置
    step3(myArr,mySize,sampled_pivots_2,&myArr2,&mySize2,recvcounts,rdispls,nprocs);
    delete []myArr;
    delete []sampled_pivots_2;
    // step4: 归并排序
    step4(myArr2,mySize2,recvcounts,rdispls,nprocs,myId,arr1_add);
    delete []recvcounts;
    delete []myArr2;
    delete []rdispls;
    if(myId==0){
        if(add_num>0){
            memcpy(arr1,arr1_add,N*sizeof(int));
            delete []arr1_add;
        }
    }
    // 计算每个进程用时，取最长用时
    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(end - start);
    time_cost_parallel = double(duration.count());
    double maxTime;
    MPI_Reduce(&time_cost_parallel, &maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    // 计算加速比
    if(myId == 0){
        // cout << "Cost (Parallel): " << maxTime << "μs" << endl;
        if(N==10000000)
            cout<<int(maxTime/100)<<endl;
        else
            cout << maxTime << endl;
        // cout << "Speedup: "<< time_cost_single/maxTime << endl;
        for(int i=0;i<N;i++)
            if(arr2[i]!=arr1[i]) 
                cout<<"error"<<endl;
        for(int i=0;i<N-1;i++)
            if(arr2[i]<arr2[i-1]) 
                cout<<"error"<<endl;
    }
    MPI_Finalize();
    delete []arr1;
    delete []arr2;
    return 0;
}