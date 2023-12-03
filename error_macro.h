#include<stdio.h>
#include<cerrno>
#include<cstring>

#define ERROR_PRINT(str,no) printf("ERROR:%s/nMESSAGE:%s/nFILE:%s,%d\n:RETURN_NUM:%d",strerror(errno),str,__FILE__,__LINE__,no);\
							fflush(stdout); \
							exit(no);