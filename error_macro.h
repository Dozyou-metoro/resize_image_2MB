#include<stdio.h>
#include<cerrno>
#include<cstring>
#include<Windows.h>

#define ERROR_PRINT(str,no) printf("\nERROR:%s\nerrno:%d\nMESSAGE:%s\nFILE:%s,%d,%s\nRETURN_NUM:%d",strerror(errno),errno,str,__FILE__,__LINE__,__FUNCTION__,no);\
							fflush(stdout); \
							Sleep(3000);\
							exit(no);