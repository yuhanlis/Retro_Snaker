#include <stdio.h>
#include <stdlib.h>
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
#include<termios.h>
#include <sys/ipc.h>





#define KEY	8888
#define SIZE	100
#define NEW_DATA	1
#define OLD_DATA	0


#define RED		63488
#define GREEN	2016
#define YELLOW	65504
#define BLUE	31
#define BLACK	0
#define WHITE	65535

#define LCD_WIDTH 800
#define LCD_HEIGHT 480
#define LCD_PIXCLOCK 2

#define LCD_RIGHT_MARGIN 67
#define LCD_LEFT_MARGIN 40
#define LCD_HSYNC_LEN 31

#define LCD_UPPER_MARGIN 25
#define LCD_LOWER_MARGIN 5
#define LCD_VSYNC_LEN 1

#define LCD_XSIZE  LCD_WIDTH
#define LCD_YSIZE  LCD_HEIGHT
#define SCR_XSIZE  LCD_WIDTH
#define SCR_YSIZE  LCD_HEIGHT

#define LCD_BLANK	20
#define C_UP		( LCD_XSIZE - LCD_BLANK*2 )
#define C_RIGHT		( LCD_XSIZE - LCD_BLANK*2 )
#define V_BLACK		( ( LCD_YSIZE - LCD_BLANK*4 ) / 6 )

//共享内存全局变量

int id;//A,B共享内存文件句柄
char* sharedMem = NULL;//A共享内存指针



//游戏逻辑所需变量
unsigned int SnakeA[10][3];
unsigned int SnakeB[10][3];
unsigned int map[40][24];
unsigned int directionA=3;  //0 shang 1 xia 2zuo 3you
unsigned int directionB=3;




void init_sharedmemory(void)
{
    id=shmget(KEY,SIZE,IPC_CREAT|0666);
    sharedMem = (char*)shmat(id,NULL,0);
    memset(sharedMem,0x00,SIZE);
    
}


void getB() ///通过共享内存获取B运动方向
{
    unsigned int olddirection=directionB;
    unsigned int newdirection;
    if(sharedMem[0]==NEW_DATA)
    {
        char dir = sharedMem[1];
        switch (dir)
        {
        case 'w':
            newdirection=0;
            break;
        case 's':
            newdirection=1;
            break;
        case 'a':
            newdirection=2;
            break;
        case 'd':
            newdirection=3;
            break;
        
        default:
            break;
        }

    }
    if(newdirection==olddirection)return;
    else
        directionB=newdirection;
}





//LCD相关函数
//所需全局变量

struct fb_var_screeninfo fb_var;//当前缓冲区的可变参数，用于LCD的基地址显示
struct fb_fix_screeninfo fb_fix;//固定参数，同上
char* fb_base_addr = NULL;//映射的LCD基地址
int display_fd;//lcd设备的文件句柄
long int screensize = 0;//屏幕大小




void TFT_LCD_Test(void);//LCD测试程序
//初始化屏幕
void init_screen()
{
    display_fd=open("/dev/fb0",O_RDWR);//打开设备
    if(display_fd<0)
    {
        perror("open device buttons failed!");
        exit(1);
    }
    //printf("lcd is open\n");
    if(ioctl(display_fd,FBIOGET_FSCREENINFO,&fb_fix))
    {
        ////printf("Error reading fb fixed information.\n");
        exit(1);
    }
    //printf("Get fixed screen information OK\n");
    if(ioctl(display_fd,FBIOGET_VSCREENINFO,&fb_var))
    {
        ////printf("Error reading fb variable information.\n");
        exit(1);
    }
    screensize=fb_var.xres*fb_var.yres*fb_var.bits_per_pixel/8;
    //printf("fb_var.xres:%d\n", fb_var.xres);
	//printf("fb_var.xres:%d\n", fb_var.yres);
    fb_base_addr = (char*)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, display_fd, 0);
	//printf("buffer is ok\n");
	if (-1 == (int)fb_base_addr)
	{
		////printf("errno\n");
		exit(1);
	}
	memset(fb_base_addr, 0xff, screensize);
    //TFT_LCD_Test();
    ////printf("Screen inited\n");
    return;
    
}




//绘制点
void draw_point(int x, int y, int color)//画点
{
	*((unsigned short*)(fb_base_addr + y * fb_var.xres * 2 + x * 2)) = color;
}
//使用某种颜色清屏
void Lcd_ClearScr(int color)//清屏
{
	unsigned int x, y;

	for (y = 0; y < SCR_YSIZE; y++)
	{
		for (x = 0; x < SCR_XSIZE; x++)
		{
			draw_point(x, y, color);
		}
	}
}
//绘制直线
void Glib_Line(int x1, int y1, int x2, int y2, int color)//画线
{
	int dx, dy, e;
	dx = x2 - x1;
	dy = y2 - y1;
	if (dx >= 0)
	{
		if (dy >= 0) // dy>=0
		{
			if (dx >= dy) // 1/8 octant
			{
				e = dy - dx / 2;
				while (x1 <= x2)
				{
					draw_point(x1, y1, color);
					if (e > 0) { y1 += 1; e -= dx; }
					x1 += 1;
					e += dy;
				}
			}
			else		// 2/8 octant
			{
				e = dx - dy / 2;
				while (y1 <= y2)
				{
					draw_point(x1, y1, color);
					if (e > 0) { x1 += 1; e -= dy; }
					y1 += 1;
					e += dx;
				}
			}
		}
		else		   // dy<0
		{
			dy = -dy;   // dy=abs(dy)

			if (dx >= dy) // 8/8 octant
			{
				e = dy - dx / 2;
				while (x1 <= x2)
				{
					draw_point(x1, y1, color);
					if (e > 0) { y1 -= 1; e -= dx; }
					x1 += 1;
					e += dy;
				}
			}
			else		// 7/8 octant
			{
				e = dx - dy / 2;
				while (y1 >= y2)
				{
					draw_point(x1, y1, color);
					if (e > 0) { x1 += 1; e -= dy; }
					y1 -= 1;
					e += dx;
				}
			}
		}
	}
	else //dx<0
	{
		dx = -dx;		//dx=abs(dx)
		if (dy >= 0) // dy>=0
		{
			if (dx >= dy) // 4/8 octant
			{
				e = dy - dx / 2;
				while (x1 >= x2)
				{
					draw_point(x1, y1, color);
					if (e > 0) { y1 += 1; e -= dx; }
					x1 -= 1;
					e += dy;
				}
			}
			else		// 3/8 octant
			{
				e = dx - dy / 2;
				while (y1 <= y2)
				{
					draw_point(x1, y1, color);
					if (e > 0) { x1 -= 1; e -= dy; }
					y1 += 1;
					e += dx;
				}
			}
		}
		else		   // dy<0
		{
			dy = -dy;   // dy=abs(dy)

			if (dx >= dy) // 5/8 octant
			{
				e = dy - dx / 2;
				while (x1 >= x2)
				{
					draw_point(x1, y1, color);
					if (e > 0) { y1 -= 1; e -= dx; }
					x1 -= 1;
					e += dy;
				}
			}
			else		// 6/8 octant
			{
				e = dx - dy / 2;
				while (y1 >= y2)
				{
					draw_point(x1, y1, color);
					if (e > 0) { x1 -= 1; e -= dy; }
					y1 -= 1;
					e += dx;
				}
			}
		}
	}
}
//绘制实心矩形
void Glib_FilledRectangle(int x1, int y1, int x2, int y2, int color)//填充矩形
{
	int i;
	for (i = y1; i <= y2; i++)
		Glib_Line(x1, i, x2, i, color);
}
//LCD测试程序
void TFT_LCD_Test(void)//LCD测试程序
{
	Lcd_ClearScr(BLACK);		//清空屏幕（黑）
	Lcd_ClearScr(WHITE);		//清空屏幕（白）
	Lcd_ClearScr(0xffff);

	Glib_FilledRectangle(LCD_BLANK, LCD_BLANK, (LCD_XSIZE - LCD_BLANK), (LCD_YSIZE - LCD_BLANK), 0x0000);						//fill a Rectangle with some color
	Glib_FilledRectangle((LCD_BLANK * 2), (LCD_BLANK * 2 + V_BLACK * 0), (C_RIGHT), (LCD_BLANK * 2 + V_BLACK * 1), 0x001f);		//fill a Rectangle with some color
	Glib_FilledRectangle((LCD_BLANK * 2), (LCD_BLANK * 2 + V_BLACK * 1), (C_RIGHT), (LCD_BLANK * 2 + V_BLACK * 2), 0x07e0);		//fill a Rectangle with some color
	Glib_FilledRectangle((LCD_BLANK * 2), (LCD_BLANK * 2 + V_BLACK * 2), (C_RIGHT), (LCD_BLANK * 2 + V_BLACK * 3), 0xf800);		//fill a Rectangle with some color
	Glib_FilledRectangle((LCD_BLANK * 2), (LCD_BLANK * 2 + V_BLACK * 3), (C_RIGHT), (LCD_BLANK * 2 + V_BLACK * 4), 0xffe0);		//fill a Rectangle with some color
	Glib_FilledRectangle((LCD_BLANK * 2), (LCD_BLANK * 2 + V_BLACK * 4), (C_RIGHT), (LCD_BLANK * 2 + V_BLACK * 5), 0xf81f);		//fill a Rectangle with some color
	Glib_FilledRectangle((LCD_BLANK * 2), (LCD_BLANK * 2 + V_BLACK * 5), (C_RIGHT), (LCD_BLANK * 2 + V_BLACK * 6), 0x07ff);		//fill a Rectangle with some color
}





//-----------按键相关函数--------------------

int buttons_fd;
char buttons[2]={'0','0'};
char first_flag=0;


void initbuttons()
{
    buttons_fd=open("/dev/dial_key",0);
    if(buttons_fd<0)
    {
        perror("open device buttons failed!");
        exit(1);
    }
    char current_buttons[2];
    char realchar;
    
    

}


void getA() //获取A的按键方向
{
    char current_buttons[2];
    char realchar;
    if(read(buttons_fd,current_buttons,sizeof(current_buttons))!=sizeof(current_buttons))
    {
        perror("read buttons:");
        exit(1);
    }
    if(buttons[0]!=current_buttons[0]||buttons[1]!=current_buttons[1])
        {
                buttons[0]=current_buttons[0];
                buttons[1]=current_buttons[1];
                switch (buttons[0])
                {   
                case 51:
					switch(buttons[1])
					{
						case 51:realchar='1';break;
						case 50:realchar='2';break;
						case 49:realchar='3';break;
						case 48:realchar='A';break;
					}break;
				case 50:
					switch(buttons[1])
					{
						case 51:realchar='4';break;
						case 50:realchar='5';break;
						case 49:realchar='6';break;
						case 48:realchar='B';break;
					}break;
				case 49:
					switch(buttons[1])
					{
						case 51:realchar='7';break;
						case 50:realchar='8';break;
						case 49:realchar='9';break;
						case 48:realchar='C';break;
					}break;
				case 48:
					switch(buttons[1])
					{
						case 51:realchar='*';break;
						case 50:realchar='0';break;
						case 49:realchar='#';break;
						case 48:realchar='D';break;
					}break;
                }
            }
            unsigned int olddirection=directionA;
            unsigned int newdirection=directionA;
            switch (realchar)
            {
            case '2':
                newdirection=0;
                break;
            case '5':
                newdirection=1;
                break;
            case '4':
                newdirection=2;
                break;
            case '6':
                newdirection=3;
                break;
            
            default:
                break;
            }
            if(olddirection==newdirection)return;
            else
            {
                directionA=newdirection;
                return;
            }
            


}



















void delay(long time)
{
	long tmp = 25;
	while( time-- )
	{
		while(tmp--); 
		tmp = 25;
	}
}


void drawSnake()        //将蛇位置映射到地图
{
    int i;
    for(i=0;i<10;i++)
    {
        if(SnakeA[i][2])
        {
            int x=SnakeA[i][0];
            int y=SnakeA[i][1];
            map[x][y]=1;
        }
        if(SnakeB[i][2])
        {
            map[SnakeB[i][0]][SnakeB[i][1]]=2;
        }
    }
}


void clear(void)        //清空地图
{
    int i,j;
    for( i=0;i<40;i++)
    {
        for(j=0;j<24;j++)
        {
            map[i][j]=0;
        }
    }

}

void Draw(void)         //显示在屏幕上
{
    int i,j;
    for(i=0;i<40;i++)
    {
        for(j=0;j<24;j++)
        {
            switch (map[i][j])
            {
            case 0:
                Glib_FilledRectangle(i*LCD_BLANK,j*LCD_BLANK,(i+1)*LCD_BLANK,(j+1)*LCD_BLANK,WHITE);
                break;
            case 1:
                Glib_FilledRectangle(i*LCD_BLANK,j*LCD_BLANK,(i+1)*LCD_BLANK,(j+1)*LCD_BLANK,BLUE);
                break;
            case 2:
                Glib_FilledRectangle(i*LCD_BLANK,j*LCD_BLANK,(i+1)*LCD_BLANK,(j+1)*LCD_BLANK,GREEN);
                break;
            default:
                break;
            }
        }
    }
    return;
}


void initSnake(void)    //初始化蛇身
{
    int i,j;
    for(i=0;i<10;i++)
    {
        SnakeA[i][0]=10-i;
        SnakeA[i][1]=1;
        SnakeA[i][2]=1;

        SnakeB[i][0]=10-i;
        SnakeB[i][1]=10;
        SnakeB[i][2]=1;
    }
}

void Amove()        //A蛇移动
{
    unsigned int x,y;
    unsigned int i=9;
    switch (directionA)
    {
    case 0://上
        /* code */
        x=0;
        y=-1;
        break;
    case 1://下
        /* code */
        x=0;
        y=1;
        break;
    case 2://左
        /* code */
        x=-1;
        y=0;
        break;
    case 3://右
        /* code */
        x=1;
        y=0;
        break;   
    
    default:
        break;
    }
while (i>0)
{
    if(SnakeA[i][2] != 0)
      {
             SnakeA[i][0] = SnakeA[i-1][0];
             SnakeA[i][1] = SnakeA[i-1][1];
      }
         i--;
}

    SnakeA[0][0]+=x;
    SnakeA[0][1]+=y;
}




void Bmove()    //B蛇移动
{
    unsigned int x,y;
    unsigned int i=9;
    switch (directionB)
    {
    case 0://上
        /* code */
        x=0;
        y=-1;
        break;
    case 1://下
        /* code */
        x=0;
        y=1;
        break;
    case 2://左
        /* code */
        x=-1;
        y=0;
        break;
    case 3://右
        /* code */
        x=1;
        y=0;
        break;   
    
    default:
        break;
    }
while (i>0)
{
    if(SnakeB[i][2] != 0)
      {
             SnakeB[i][0] = SnakeB[i-1][0];
             SnakeB[i][1] = SnakeB[i-1][1];
         }
         i--;
}

    SnakeB[0][0]+=x;
    SnakeB[0][1]+=y;
}


unsigned int ifBreak(void)      //1A胜，2B胜，C平局
{
    unsigned int i;
    if(SnakeA[0][0]==0||SnakeA[0][0]==40||SnakeA[0][1]==0||SnakeA[0][1]==24)
    {
        return 2;
    }else if(SnakeB[0][0]==0||SnakeB[0][0]==40||SnakeB[0][1]==0||SnakeB[0][1]==24)
    {
        return 1;
    }
    for(i = 1; i < 10; i++)
   {
         if(SnakeA[0][0] == SnakeA[i][0] && SnakeA[0][1] == SnakeA[i][1])
             return 2;
             else if(SnakeB[0][0] == SnakeB[i][0] && SnakeB[0][1] == SnakeB[i][1])
             return 1;
   }

   for(i=0;i<10;i++)
   {
       if(SnakeA[0][0]==SnakeB[i][0]&&SnakeA[0][1]==SnakeB[i][1])
       {
           if(i==0)
           {
               return 0;
           }
           else return 2;
       }

       if(SnakeB[0][0]==SnakeA[i][0]&&SnakeB[0][1]==SnakeA[i][1])
       {
           if(i==0)
           {
               return 0;
           }
           else return 1;
       }
   }
   return 3;

}



void game(void )
{
    //初始化设备和游戏资源
    initbuttons();
    init_sharedmemory();
    init_screen();
    // //printf("LCD inited!\n");
    // //printf("LCD inited!\n");
    // //printf("LCD inited!\n");
    initSnake();
    ////printf("Snake init!\n");
    drawSnake();
    Draw();
    while(1)
    {
        if(sharedMem[0]==NEW_DATA)
        {
            sharedMem[0]=OLD_DATA;
            break;
        }
        

    }

        
        while(1)
        {
        
        unsigned int res;
        getA();
        getB();
        Amove();
        Bmove();
        clear();
        drawSnake();
        Draw();
        res=ifBreak();
        if(res!=3)
        {
            switch (res)
            {
            case 0:
                /* code */
                //printf("平局");
                break;
            case 1:
                //printf("A获胜");
                break;
            case 2:
                //printf("B获胜");
                break;
            
            default:
                break;
            }
            shmdt(sharedMem);
            exit(0);
        }

        
        delay(1000*10);
    }
    
}
	











int  main(void)
{   
    game();
     return 0;
}












/*int display_fd;//lcd设备的文件句柄
	long int screensize = 0;//屏幕大小

	//打开LCD驱动
	display_fd = open("/dev/fb0", O_RDWR);//打开设备
	if (display_fd < 0) {  //打开设备失败
		perror("open device buttons failed！");
		exit(1);
	}

	if (ioctl(display_fd, FBIOGET_FSCREENINFO, &fb_fix)) {
		exit(1);
	}
	Get variable screen information 	
	if (ioctl(display_fd, FBIOGET_VSCREENINFO, &fb_var)) {
		exit(1);
	}
	//初始化屏幕
	screensize = fb_var.xres * fb_var.yres * fb_var.bits_per_pixel / 8;
	fb_base_addr = (char*)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, display_fd, 0);
	if (-1 == (int)fb_base_addr)
	{
		exit(1);
	}
	memset(fb_base_addr, 0xff, screensize);
     //game();


     int id;//共享内存文件句柄
	char* sharedMem = NULL;//共享内存指针
	id = shmget(KEY,SIZE,IPC_CREAT|0666);
	sharedMem = (char*)shmat(id, NULL, 0);
	memset(sharedMem,0x00,SIZE);
    initSnake();
    unsigned int res;


    int buttons_fd;
	char buttons[2] = {'0', '0'};
	char first_flag = 0;

	buttons_fd = open("/dev/dial_key", 0);
	if (buttons_fd < 0) {
		perror("open device buttons failed！");
		exit(1);
	}





    while(1)
    {
    

        int i;
		char current_buttons[2];
		char realchar;
		if (read(buttons_fd, current_buttons, sizeof current_buttons) != sizeof current_buttons) {
			perror("read buttons:");
			exit(1);
		}
		if(first_flag == 0)
			first_flag = 1;
		else
		{
			if (buttons[0] != current_buttons[0] || buttons[1] != current_buttons[1]) 
			{
				buttons[0] = current_buttons[0];
				buttons[1] = current_buttons[1];
				switch(buttons[0])
				{
				case 51:
					switch(buttons[1])
					{
						case 51:realchar='1';break;
						case 50:realchar='2';break;
						case 49:realchar='3';break;
						case 48:realchar='A';break;
					}break;
				case 50:
					switch(buttons[1])
					{
						case 51:realchar='4';break;
						case 50:realchar='5';break;
						case 49:realchar='6';break;
						case 48:realchar='B';break;
					}break;
				case 49:
					switch(buttons[1])
					{
						case 51:realchar='7';break;
						case 50:realchar='8';break;
						case 49:realchar='9';break;
						case 48:realchar='C';break;
					}break;
				case 48:
					switch(buttons[1])
					{
						case 51:realchar='*';break;
						case 50:realchar='0';break;
						case 49:realchar='#';break;
						case 48:realchar='D';break;
					}break;
				}
				//printf("key(%c) is pressed!\n", realchar);
			}
		}
        switch (realchar)
        {
        case '2':
            directionA=0;
            break;
        case '4':
            directionA=2;
            break;
        
        case '5':
            directionA=1;
            break;
        case '6':
            directionA=3;
            break;
        default:
            break;
        }


        if (sharedMem[0] == NEW_DATA)
		{
            switch (sharedMem[1])
            {
            case 'w':
                directionB=0;
                break;
            case 's':
                directionB=1;
                break;
            case 'a':
                directionB=2;
                break;
            case 'd':
                directionB=3;
                break;
            
            default:
                break;
            }
			sharedMem[0] = OLD_DATA;
		}

        
        Amove();
        Bmove();
        clear();
        drawSnake();
        Draw();
        res=ifBreak();
        if(res!=3)
        {
            switch (res)
            {
            case 0:
                /* code 
                //printf("平局");
                break;
            case 1:
                //printf("A获胜");
                break;
            case 2:
                //printf("B获胜");
                break;
            
            default:
                break;
            }
            shmdt(sharedMem);
            return;
        }
        
    delay(1000*10);
    
    }*/
