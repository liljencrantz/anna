#include <stdio.h>
#include <stdlib.h>
#include <complex.h>

int mandelbrot(complex p0, int max_step)
{
    complex p = p0;
    int step = 0;
    while(cabs(p) < 2.0 && ++step < max_step) 
    {
        p = p*p + p0;
    }
    return step;
}

int main()
{
    double y=-1.3;
    while(y<=0.9)
    {
	double x=-2.0;
	while(x<=1.3)
	{
	    printf("%c", " .::!!!ooooOOOOO################################################ "[mandelbrot(x+I*y,1024)/16]);
	    x += 0.025;
	}
	y += 0.05;
	printf("\n");
    }
}
