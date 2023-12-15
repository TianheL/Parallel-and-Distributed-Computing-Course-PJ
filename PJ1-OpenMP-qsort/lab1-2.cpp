#include<iostream>
#include<cstdlib>
#include<ctime>
#include<chrono>
#include<omp.h>
#include<cstring>
using namespace std;

int partition(int* arr, int left, int right) {
	int base = arr[left];
	while (left < right) {
		while (left < right && arr[right] >= base){
			right--;
		}
		arr[left] = arr[right];
		while (left < right && arr[left] <= base) {
			left++;
		}
		arr[right] = arr[left];
	}
	arr[left] = base;
	return left;
}

void qsort_single(int* arr, int left, int right) {
	if (left < right) {
		int part = partition(arr, left, right);
		qsort_single(arr, left, part - 1);
		qsort_single(arr, part + 1, right);
	}
}

void qsort_parallel(int* arr, int left, int right) {
	// cout << "thread created: " << omp_get_thread_num() << endl;
	if (left < right) {
		int part = partition(arr, left, right);
		#pragma omp task
			qsort_parallel(arr, left, part - 1);
		qsort_parallel(arr, part + 1, right);
	}
}


int* rand_array(int n) {
	int* arr = new int[n];
	for (int i = 0; i < n; i++) {
		arr[i] = rand();
	}
	return arr;
}

int check(int* arr, int size) {
	for (int i = 0; i < size-1; i++)
		if (arr[i] > arr[i + 1])
			return 0;
	return 1;
}

int main(int argc, char** argv) {
	srand(time(0));
	omp_set_num_threads(atoi(argv[1]));
	int arr_size = atoi(argv[2]);
	double time_cost_single = 0;
	double time_cost_parallel = 0;
	auto start = chrono::high_resolution_clock::now();
	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
	for (int i = 0; i < 500; i++) {
		int* arr1 = rand_array(arr_size);
		int* arr2 = new int[arr_size];
		memcpy(arr2, arr1, arr_size * sizeof(int));
		// Sequential
		start = chrono::high_resolution_clock::now();
		qsort_single(arr1, 0, arr_size - 1);
		end = chrono::high_resolution_clock::now();
		duration = chrono::duration_cast<chrono::microseconds>(end - start);
		time_cost_single += double(duration.count());
		if (check(arr1, arr_size) == 0)
			cout << "sort error !" << endl;
		// Parallel
		start = chrono::high_resolution_clock::now();
		#pragma omp parallel
		{
			#pragma omp single
			qsort_parallel(arr2, 0, arr_size - 1);
		}
		end = chrono::high_resolution_clock::now();
		duration = chrono::duration_cast<chrono::microseconds>(end - start);
		time_cost_parallel += double(duration.count());
		if (check(arr2, arr_size) == 0)
			cout << "sort error !" << endl;
		for(int i=0;i<arr_size;i++){
			if(arr1[i]!=arr2[i])
				cout<<"error"<<endl;
		}
	}
	cout << "Cost (Single Thread): " << time_cost_single/500 << "μs" << endl;
	cout << "Cost (Parallel): " << time_cost_parallel/500 << "μs" << endl;
	cout << "Speedup: "<< time_cost_single/time_cost_parallel << endl;
}

