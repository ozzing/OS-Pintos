#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>

int main(int argc, char *argv[]){

	bool success =true;
	int arg[4];
	if(argc==5){
		for(int i=1;i<argc;i++){
			arg[i-1]=atoi(argv[i]);
		}	
		int fibo=fibonacci(arg[0]);
	        int max=max_of_four_int(arg[0], arg[1], arg[2], arg[3]);

		if(fibo == -1 || max == -1){
			printf("additional failed : invalid return\n");
			success=false;
		}
		else{	
			printf("%d %d\n", fibo, max);
		}
	}
	else {
		printf("additional failed : invalid argument number\n");
		success=false;
	}
	return success ? 1 : -1;
}

