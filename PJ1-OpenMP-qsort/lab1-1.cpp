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
		while (left < right && arr[right] >= base) {
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

struct sort_array {
	int val;
	int idx;
	int idx_max;
};

void qsort_parallel(int* arr, int left, int right, int thread_num) {
	if(right-left<thread_num*10)
		qsort_single(arr,left,right);
	int* right_idx=new int[thread_num];
	for (int i = 0; i < thread_num - 1; i++) {
		right_idx[i] = (right - left + 1) / thread_num * (i + 1);
	}
	right_idx[thread_num - 1] = right;

	#pragma omp parallel num_threads(thread_num) 
	{
		int idx = omp_get_thread_num();
		qsort_single(arr, (idx == 0 ? 0 : right_idx[idx - 1] + 1), right_idx[idx]);
	}
	// Merge thread_num sorted arrays
	sort_array* s_arr= new sort_array[thread_num];
	int j = 0;
	for (int i = 0; i < thread_num; i++) {
		for (j=0; j < i; j++)
			if (s_arr[j].val > arr[(i==0 ? 0 : right_idx[i - 1] + 1)])
				break;
		for (int k = i; k > j; k--)
			s_arr[k] = s_arr[k - 1];
		s_arr[j].idx = i == 0 ? 0 : right_idx[i - 1] + 1;
		s_arr[j].val = arr[s_arr[j].idx];
		s_arr[j].idx_max = right_idx[i];
	}
	int* b=new int[right-left+1];
	int t_left=thread_num;
	sort_array temp;
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
	memcpy(arr, b, (right-left+1) * sizeof(int));
	delete b;
}


int* rand_array(int n) {
	int* arr = new int[n];
	for (int i = 0; i < n; i++) {
		arr[i] = rand();
	}
	return arr;
}

int check(int* arr, int size) {
	for (int i = 0; i < size - 1; i++)
		if (arr[i] > arr[i + 1])
			return 0;
	return 1;
}

int main(int argc, char** argv) {
	srand(time(0));
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
		qsort_parallel(arr2, 0, arr_size - 1, atoi(argv[1]));
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


