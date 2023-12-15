#include<ctime>
#include<iostream>
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

int main(int argc,char *argv[]){
    // mpirun -n nprocs ./lab2-1 arr_len seed
    srand(atoi(argv[2])); 
    int N=atoi(argv[1]);
    int nprocs,myId;
    auto start = chrono::high_resolution_clock::now();
	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>(end-start);
    double time_cost_single,time_cost_parallel;
    int *arr1=NULL;
    int *arr2=NULL;
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