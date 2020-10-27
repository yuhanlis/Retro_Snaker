#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<sys/types.h>
#include<fcntl.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<sys/select.h>
#include<sys/time.h>
#include<errno.h>
#include<pthread.h>
#include<linux/fb.h>
#include<sys/mman.h>
#include<string.h>
#include<sys/shm.h>

#define KEY		8888
#define SIZE	100
#define NEW_DATA	1
#define OLD_DATA	0
char getch()
{
	char c;
	system("stty raw");
	c=getchar();
	system("stty cooked");
	return c;
}
int main()
{
	int id;//共享内存句柄
	char* sharedMem =NULL;//共享内存指针
	id = shmget(KEY,SIZE,IPC_CREAT|0666);//创建共享内存
	sharedMem = (char*)shmat(id, NULL, 0);
	memset(sharedMem,0x00,SIZE);
	while(1)
	{
        *(sharedMem+1)=getch();
		sharedMem[0] = NEW_DATA;
        if(*(sharedMem+1)=='0')
        break;
	}
	//解除共享内存链接
	shmdt(sharedMem);
	//销毁共享内存
	shmctl(id, IPC_RMID,NULL);
	printf("PlayerB is end!\n");
	return 0;
}
