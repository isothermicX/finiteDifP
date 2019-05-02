#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "papi.h"

#define DEBUG 0
#define MAX_ITER 3500

// Calculates how many rows are given, as maximum, to each thread
int get_max_rows(const int num_threads, const int n) {
	return (int)(ceil((n-2) / num_threads) + 2);
}

void mat_stats(float **mat, float **T, const int n){
	double sum = 0;
	int i = 0;
	double tmp;
	double max = 0;
	double min = 0;

	const int size = n*n;

	for(i=0;i<(size-n);i++){
		tmp = ((*T)[i] - (*mat)[i]) / ((*T)[i]);
		sum += tmp;
		if(tmp > max){
			max = tmp;
		}
		if(tmp < min){
			min = tmp;
		}
	}

	printf("max err: %.6lf\n",max);
	printf("min err: %.6lf\n",min);
	printf("sum err: %.6lf\n",sum);
}

void alloc_matrix(float **mat, const int n, const int T_top, const int T_bottom, const int T_left, const int T_right) {

	*mat = (float *) malloc(n * n * sizeof(float));

	const int size = n*n;
	for (int i = 0; i < size; i++) {
		(*mat)[i] = 50;
	}

	int i;
	
	for(i=0;i<n;i++){
		(*mat)[i] = T_top; 			   //put top initial vals
		(*mat)[n*(n-1)+i] = T_bottom;  //put bottom initial vals
		(*mat)[i*n] = T_left; 		   //put left initial vals
		(*mat)[i*n + (n-1)] = T_right; //put right initial vals
	}

	if(DEBUG){
		for(i=0;i<size;i++){
			printf("%6.2lf ",(*mat)[i]);
			if(i%n == n-1) printf("\n");
		}
	}
}

void solver(float **mat, const int n, const int num_ths, const int max_cells_per_th){
//	double diff;

//	int done = 0;
	int cnt_iter = 0;
//	const int mat_dim = n*n;

	while((cnt_iter < MAX_ITER)){
//		diff = 0;

		#pragma omp parallel for num_threads(num_ths) schedule(static, max_cells_per_th) collapse(2) //reduction(+:diff)
		for(int i=1;i<(n-1);i++){
			for(int j=1;j<(n-1);j++){
				const int pos = (i*n)+j;
//				const float temp = (*mat)[pos];

				(*mat)[pos] = 
					0.25*(
						  (*mat)[pos-1]
						+ (*mat)[pos-n]
						+ (*mat)[pos+1]
						+ (*mat)[pos+n]
					);

//				diff += abs((*mat)[pos] - temp)/(*mat)[pos];
			}
		}

//		if(diff/mat_dim < TOL){
//			done = 1;
//		}
		cnt_iter++;
	}

	printf("iterations parallel: %d\n",cnt_iter);
//	printf("Maximum error is: %.6lf\n",diff);
}

void solver_serial(float **T, const int n){
	float T_old;
	float a, b, c, d;
	float T_new;
	float lambda = 1.2;
	float e_a, e_s;
	int iters = 0;
	int i, j, pos;

	e_s = 0.00001;
	e_a = 1;

	do{
		e_a = 0;
		for(i=1;i<(n-1);i++){
			for(j=1;j<(n-1);j++){
				pos = (i*n) + j;
				T_old = (*T)[pos];
				a = (*T)[pos-1]; //left neighbor
				b = (*T)[pos+1]; //right neighbor
				c = (*T)[pos-n]; //top neighbor
				d = (*T)[pos+n]; //bottom neighbor
				T_new = (a+b+c+d)/4;
				T_new = lambda*T_new + (1-lambda)*T_old;
				(*T)[pos] = T_new;
				if((fabs(T_new - T_old)/T_new) > e_a){
					e_a = (float)fabs(T_new - T_old)/T_new;
					if(DEBUG) printf("%4.2lf \n",e_a);
				}
			}
		}

		iters++;
	}while(e_a > e_s);

	printf("iterations serial:   %d\n",iters);
}

int main(void){
	const int n = 100;
	const int T_left   =  60;
	const int T_right  =  50;
	const int T_top    = 120;
	const int T_bottom =   0;

	float *mat, *T;
	alloc_matrix(&mat,n,T_top,T_bottom,T_left,T_right);
	alloc_matrix(&T,n,T_top,T_bottom,T_left,T_right);

	const int max_threads = omp_get_max_threads();
	const int max_rows = get_max_rows(max_threads, n);
	const int max_cells = max_rows * (n-2);

	long long time_1, time_2, time_3;

    //initialize PAPI
	int result;
	result=PAPI_library_init(PAPI_VER_CURRENT);
	if (result!=PAPI_VER_CURRENT) {
		fprintf(stderr,"Warning!  PAPI error %s\n",
			PAPI_strerror(result));
	}

	time_1 = PAPI_get_real_usec();

	solver(&mat,n,max_threads,max_cells);

	time_2 = PAPI_get_real_usec();

	solver_serial(&T,n);

	time_3 = PAPI_get_real_usec();

	mat_stats(&mat,&T,n);

	printf("Time to solve parallel: %6lld\n",time_2-time_1);
	printf("Time to solve serial:   %6lld\n",time_3-time_2);
	printf("Cores used: %d.\n",max_threads);

	free(mat);

	return 0;
}