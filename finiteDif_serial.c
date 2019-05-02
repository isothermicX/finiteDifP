#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "papi.h"

int main(void){
	int DEBUG = 0; //set to "1" to print a bunch of extra data to console

	int n = 100;
	int T_left   =  60;
	int T_right  =  50;
	int T_top    = 120;
	int T_bottom =   0;

	int n_rows, n_cols, row, col, i;

	n_rows = n;
	n_cols = n;

	float *T = (float *)malloc(n_rows*n_cols*sizeof(float));

	for(i=0;i<(n_cols*n_rows);i++){
		*(T+i) = 50;
	}

	long long time_1, time_2;

    //initialize PAPI
	int result;
	result=PAPI_library_init(PAPI_VER_CURRENT);
	if (result!=PAPI_VER_CURRENT) {
		fprintf(stderr,"Warning!  PAPI error %s\n",
			PAPI_strerror(result));
	}

	//put top initial vals
	for(col=1;col<(n_cols-1);col++){
		*(T + col) = T_top;
	}

	//put bottom initial vals
	for(col=1;col<(n_cols-1);col++){
		*(T + (n_rows-1)*n_cols + col) = T_bottom;
	}

	//put left initial vals
	for(row=1;row<(n_rows-1);row++){
		*(T + row*n_cols) = T_left;
	}

	//put right initial vals
	for(row=1;row<(n_rows-1);row++){
		*(T + row*n_cols + n_cols-1) = T_right;
	}

	float T_old;
	float a, b, c, d;
	float T_new;
	float lambda = 1.2;
	float e_a, e_s;
	int iters = 0;

	e_s = 0.00001;
	e_a = 1;

	time_1 = PAPI_get_real_usec();

	while(e_a > e_s){
		e_a = 0;
		for(row=1;row<(n_rows-1);row++){
			for(col=1;col<(n_cols-1);col++){
				T_old = *(T + (row*n_cols) + col);
				a = *(T + (row*n_cols) + col - 1); //left neighbor
				b = *(T + (row*n_cols) + col + 1); //right neighbor
				c = *(T + (row*n_cols) + col - 1*(n_cols)); //top neighbor
				d = *(T + (row*n_cols) + col + 1*(n_cols)); //bottom neighbor
				T_new = (a+b+c+d)/4;
				T_new = lambda*T_new + (1-lambda)*T_old;
				*(T + (row*n_cols) + col) = T_new;
				if((fabs(T_new - T_old)/T_new) > e_a){
					e_a = (float)fabs(T_new - T_old)/T_new;
					if(DEBUG) printf("%4.2lf \n",e_a);
				}
			}
		}

		iters++;
		if(iters > 9999){
			printf("ERR! Too many iterations (>9999)\n");
			return -1;
		}
	}

	time_2 = PAPI_get_real_usec();

	printf("Problem solved in %d iterations!\n", iters);
	printf("Maximum error is: %.6lf\n",e_a);
	printf("Time to solve: %lld\n",time_2-time_1);

	free(T);

	return 0;
}