#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
	int a, b, c, d;

	if(argc != 5)
			return EXIT_FAILURE;

	a = atoi(argv[1]);
	b = atoi(argv[2]);
	c = atoi(argv[3]);
	d = atoi(argv[4]);

	if(a < 1)
			return EXIT_FAILURE;
	printf("%dth fibonacci number : %d\nsum of four integers : %d\n", a, pibonacci(a), sum_of_four_integers(a,b,c,d));

  return EXIT_SUCCESS;
}
