#include <stddef.h>
#include <stdio.h>

// The same standard C code as before
long sum_array(long *array, size_t n) {
	long sum = 0;
	for (size_t i = 0; i < n; i++) {
		sum += array[i];
	}
	return sum;
}


int main(){
	long a[8] = {1, 2, 3 ,4, 5 ,6 ,7 ,8};
	printf("%ld\n", sum_array( a, 8));
}
