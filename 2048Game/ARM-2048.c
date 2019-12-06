#include <errno.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <bits/pthreadtypes.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
/*
	time: 2019-12-4  ---  14:53
	2048-3
	���ȣ�
		����ɣ�
			1: �߼�����ȫ�����ƣ����벼����΢���㣬û������Ϸ�����Ĵ���
			2���³��ֵ�Ԫ�ػ���  2/4 �������   --- > 12-4  --- 22:37
*/
int *plcd = NULL;
int color, oldScore=0;
int numbers[10]; 				// 0--9 ������ͼƬ
int score=0; 					// ��ǰ�÷�
int imgs[16]; 					// ����ͼƬ����Ϸ����ͼƬ����ťͼƬ
int map[6][6];					// ��Ϸ��ͼ
int fd_touch; 					// �����¼�
int fd_lcd;   					// ��Ļ
int startx=25, starty=185;  	// ��Ϸ��������Ͻǣ�������ֿ�������Ϸ��ʾ�����м�
int pid;						// ���������߳�
char *buf = "./res/1.mp3";		// �����ļ�·��

struct Local {
	int i, j;
};
struct  Local local[16];			// ͨ��ѭ�����飬ʹÿ���³��ֵ�λ�ò�ͬ
struct  Local locational[6][6];     // ÿ��ͼƬӦ�ó��ֵ�λ�� , locational[][].j  �� j ���� x ���ֵ
int  	defMove[6]; 				// �ж�һ�������Ƿ��ƶ�
int  	step;       				// �����³��ֵ�λ�ò�Ҫ�ظ�
void 	initImge();  										// ��ʼ����Ϸ��Դ
void 	showImage(int fd_bmp, int x1, int y1);				// ��ʾͼƬ
void 	showImage3(int fd, int x0, int y0);
void 	downImage();										// �ر�ͼƬ�ļ�
void 	draw(int starty, int startx);  						// ������Ϸ��ʼ����
void 	GameControl(int dir);          						// �ϲ�����
void 	GameInit();											// ��Ϸ��ʼ��
void 	GameEnd();  										// ��Ϸ��������Դ�ر�
void  	GameRun(int dirX0, int dirY0, int dirX1, int dirY1);// ��Ϸ���У��ȴ�����
void    WaitTouch();										// ��Ϸ���У��ȴ�����
void    reDrawMap();										// ��Ϸ���¿�ʼ���ػ���Ϸ����
void 	LCD_Draw_Point(int x, int y, int color);
void 	new3();												// �����µķ��飬�������ȡ��
int  	findBmpId(int x);               					// ����map[][] ��ֵ�ж�Ҫ��ʾ����ͼƬ������������
void    setScore();                                			// ����һ�����֣����ض���λ����ʾ����
void 	lcd_draw_word(int x0,int y0,int w,int h,char *data,int color);
void showWord();

/**
 * ����dataxϵ������洢���Ǻ�����ģ��
 * 
 **/
char 	data1[33*32/8]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x38,0x01,0xE0,0x00,0x3C,
                         0x00,0xF0,0x00,0x3C,0x00,0xF0,0x00,0x3C,0x00,0x70,0x00,0x3C,0x00,0x70,0x8F,0x3C,
                         0x00,0x61,0xCF,0x3C,0x7F,0xFF,0xEF,0x3C,0x30,0x07,0x0F,0x3C,0x00,0x0F,0x8F,0x3C,
                         0x00,0x0F,0x0F,0x3C,0x18,0x0E,0x0F,0x3C,0x0E,0x1E,0x0F,0x3C,0x07,0x1C,0x0F,0x3C,
                         0x03,0xFC,0x0F,0x3C,0x01,0xF8,0x0F,0x3C,0x00,0xF8,0x0F,0x3C,0x00,0x78,0x0F,0x3C,
                         0x00,0xFE,0x0F,0x3C,0x00,0xFF,0x0F,0x3C,0x01,0xCF,0x8F,0x3C,0x03,0x87,0x8F,0x3C,
                         0x07,0x03,0xCE,0x3C,0x0F,0x03,0xC0,0x3C,0x1C,0x01,0x80,0x3C,0x38,0x00,0x07,0xFC,
                         0x70,0x00,0x03,0xF8,0x40,0x00,0x00,0xF8,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,
                         0x00,0x00,0x00,0x00
                      };

char 	data2[33*32/8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x0E,0x00,0x00,0x78,0x0F,0x00,
                          0x00,0x77,0x0E,0x38,0x3F,0xFF,0xFF,0xFC,0x18,0x70,0x6E,0x00,0x00,0x76,0x0E,0x70,
                          0x0F,0xFF,0xFF,0xF8,0x06,0x70,0x6E,0x00,0x00,0x77,0x0E,0x18,0x3F,0xFF,0xFF,0xFC,
                          0x18,0x70,0x6E,0x00,0x00,0x70,0x0E,0x00,0x00,0x00,0x00,0xC0,0x07,0xFF,0xFF,0xE0,
                          0x03,0x00,0x01,0xE0,0x00,0x00,0x01,0xE0,0x03,0xFF,0xFF,0xE0,0x01,0x80,0x01,0xE0,
                          0x00,0x00,0x01,0xE0,0x07,0xFF,0xFF,0xE0,0x03,0x07,0x01,0xE0,0x02,0x73,0x80,0x00,
                          0x06,0x79,0xC6,0xE0,0x06,0x71,0xC6,0x78,0x0E,0x70,0xC6,0x3C,0x1E,0x70,0x07,0x1C,
                          0x1C,0x7F,0xFF,0x9C,0x00,0x3F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                          0x00,0x00,0x00,0x00
                       };
char 	data3[33*32/8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x00,0x00,0x07,0xF0,
                          0x00,0x00,0xFF,0xF0,0x00,0x7F,0xF0,0x38,0x7F,0xF8,0x30,0x7C,0x33,0x8C,0x38,0x78,
                          0x03,0x8E,0x1C,0xF0,0x03,0x87,0x1C,0xE0,0x03,0x87,0x9D,0xC0,0x03,0x83,0xC9,0x80,
                          0x03,0x81,0xE1,0x00,0x03,0xF1,0xC0,0x70,0x3F,0xFB,0xFF,0xF8,0x1B,0x87,0x9C,0x00,
                          0x03,0x87,0x1C,0x00,0x03,0x8E,0x1C,0x00,0x03,0x98,0x1C,0x08,0x03,0x80,0x1C,0x1C,
                          0x03,0xBF,0xFF,0xFE,0x03,0x98,0x1C,0x00,0x03,0x86,0x1C,0x70,0x03,0xFF,0x1C,0x78,
                          0x0F,0xC7,0x1C,0x78,0x7F,0x07,0x1C,0x78,0x7C,0x07,0x1C,0x78,0x30,0x07,0x1C,0x78,
                          0x00,0x0F,0xFF,0xF8,0x00,0x06,0x00,0x78,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,
                          0x00,0x00,0x00,0x00
                       };
char	data4[33*4] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xC0,0x00,0x00,0x03,0xE0,
                       0x00,0x00,0x03,0xC0,0x00,0x00,0x03,0x80,0x00,0x00,0x07,0x80,0x00,0x00,0x07,0x1C,
                       0x00,0x00,0x0F,0xFE,0x00,0x01,0xFF,0xF8,0x00,0x00,0xFE,0x00,0x00,0x00,0x1C,0x00,
                       0x00,0x00,0x3C,0x03,0x00,0x00,0x3B,0xFF,0x00,0xFF,0xFF,0xFF,0x01,0xFF,0xFC,0x00,
                       0x00,0xF0,0xE0,0x00,0x00,0x00,0xE0,0x00,0x00,0x01,0xCF,0xC0,0x00,0x07,0xFF,0xE0,
                       0x00,0x07,0xC3,0xE0,0x00,0x07,0x07,0x80,0x00,0x00,0x1E,0x00,0x00,0x0E,0x3C,0x00,
                       0x00,0x0F,0xF0,0x00,0x00,0x07,0xE0,0x00,0x00,0x03,0xE0,0x00,0x00,0x03,0xE0,0x00,
                       0x00,0x01,0xE0,0x00,0x00,0x01,0xE0,0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,
                       0x00,0x00,0x00,0x00,
                   };
char	data5[33*4]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,
                      0x00,0x00,0x3F,0xFF,0x00,0x00,0x3F,0x1E,0x00,0x00,0x38,0x3C,0x00,0x00,0x70,0xF8,
                      0x00,0x00,0x7F,0xF0,0x00,0x00,0xFF,0xE0,0x00,0x00,0xE1,0xF8,0x00,0x01,0xC7,0xF0,
                      0x00,0x03,0xDF,0x00,0x00,0x03,0xFF,0x00,0x00,0x07,0x0F,0xF8,0x00,0x0F,0xFF,0xF8,
                      0x00,0x0F,0xFE,0xF0,0x00,0x1D,0xDD,0xE0,0x00,0x39,0xFF,0xE0,0x00,0x71,0xFC,0x18,
                      0x00,0xE1,0xBF,0xFE,0x01,0xFF,0xFF,0xBE,0x03,0xFF,0xF7,0x3C,0x07,0xB8,0x77,0x38,
                      0x0E,0x38,0xFF,0x38,0x1C,0x7F,0xE7,0x78,0x78,0x77,0x80,0x70,0x00,0xF0,0x06,0xF0,
                      0x00,0xE0,0x07,0xE0,0x00,0x00,0x07,0xC0,0x00,0x00,0x03,0x80,0x00,0x00,0x02,0x00,
                      0x00,0x00,0x00,0x00,
                  };
char	data6[33*4]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x07,0xC0,0x00,0x00,0x1F,0xF0,0x00,0x00,0x7F,0xF0,0x00,
                      0x00,0xFD,0xF8,0x00,0x01,0xF0,0xF8,0x00,0x03,0xE0,0xF8,0x00,0x03,0xC0,0xF0,0x00,
                      0x00,0x00,0xF0,0x00,0x00,0x01,0xE0,0x00,0x00,0x03,0xC0,0x00,0x00,0x07,0xC0,0x00,
                      0x00,0x0F,0x80,0x00,0x00,0x1F,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x78,0x00,0x00,
                      0x00,0xF0,0x00,0x00,0x03,0xE0,0x00,0x00,0x07,0xC0,0x00,0x00,0x0F,0x80,0x00,0x00,
                      0x1F,0x00,0x00,0x00,0x3F,0xFF,0x00,0x00,0x7F,0xFE,0x00,0x00,0x3F,0xFE,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,
                  };
char	data7[33*4]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x1F,0xE0,0x00,0x00,0x3F,0xF0,0x00,
                      0x00,0x7D,0xF0,0x00,0x00,0xF0,0xF0,0x00,0x01,0xE0,0xF0,0x00,0x03,0xE0,0xF0,0x00,
                      0x03,0xC0,0xF0,0x00,0x07,0xC0,0xF0,0x00,0x07,0x80,0xF0,0x00,0x07,0x81,0xE0,0x00,
                      0x0F,0x01,0xE0,0x00,0x0F,0x01,0xE0,0x00,0x0F,0x03,0xC0,0x00,0x1E,0x03,0xC0,0x00,
                      0x1E,0x07,0x80,0x00,0x1E,0x07,0x80,0x00,0x1E,0x0F,0x00,0x00,0x1E,0x1F,0x00,0x00,
                      0x1F,0x3E,0x00,0x00,0x0F,0xFC,0x00,0x00,0x0F,0xF8,0x00,0x00,0x03,0xE0,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,
                  };
char	data8[33*4]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0xF0,0x00,0x00,0x01,0xF0,0x00,
                      0x00,0x03,0xF0,0x00,0x00,0x07,0xE0,0x00,0x00,0x0F,0xE0,0x00,0x00,0x1F,0xE0,0x00,
                      0x00,0x3F,0xC0,0x00,0x00,0x7F,0xC0,0x00,0x00,0xFB,0xC0,0x00,0x01,0xF3,0x80,0x00,
                      0x03,0xC7,0x80,0x00,0x07,0x87,0x80,0x00,0x0F,0x0F,0x00,0x00,0x1E,0x0F,0x00,0x00,
                      0x3F,0xFF,0xE0,0x00,0x3F,0xFF,0xC0,0x00,0x3F,0xFF,0xC0,0x00,0x00,0x1E,0x00,0x00,
                      0x00,0x3C,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x38,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,
                  };
char	data9[33*4]= {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x07,0x80,0x00,0x00,0x3F,0xE0,0x00,0x00,0x7F,0xF0,0x00,
                      0x00,0xF9,0xF8,0x00,0x01,0xE0,0xF8,0x00,0x01,0xE0,0xF0,0x00,0x03,0xC0,0xF0,0x00,
                      0x03,0xC1,0xF0,0x00,0x03,0xC3,0xE0,0x00,0x03,0xFF,0xC0,0x00,0x01,0xFF,0x80,0x00,
                      0x03,0xFF,0x80,0x00,0x07,0xFF,0x80,0x00,0x0F,0x87,0xC0,0x00,0x1E,0x03,0xC0,0x00,
                      0x1E,0x03,0xC0,0x00,0x3C,0x07,0x80,0x00,0x3C,0x07,0x80,0x00,0x3C,0x0F,0x00,0x00,
                      0x3E,0x3F,0x00,0x00,0x3F,0xFE,0x00,0x00,0x1F,0xFC,0x00,0x00,0x0F,0xE0,0x00,0x00,
                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                      0x00,0x00,0x00,0x00,
                  };

//////////////////            main             /////////////////
int main() {
	//����Ļ�ļ�
	GameInit();			// ��Դ��ʼ��
	WaitTouch();    	// �ȴ�����
	GameEnd(); 			// ��Դ�ͷ�
	return 0;
}
////////////////////////////////////////////////////////////////
void GameInit() {
	//�����������ֵ��߳�
	pid = fork();
	//�̴߳����ɹ�
	if(pid==0) {
		execlp("madplay", "madplay", buf, "-r", NULL);		//����madplay��������
	}
	//����Ļ�ļ�
	fd_lcd = open("/dev/fb0",O_RDWR);
	if(fd_lcd < 0) {
		perror("open lcd error");
		return ;
	}
	int i, j;
	initImge(); 											// ����ͼƬ
	//�γ�ӳ���ϵ,plcdָ��ӳ�������е��׵�ַ(Ҳ���ǵ�һ��int��λ��)
	plcd = mmap(NULL,480*800*4,PROT_READ | PROT_WRITE,MAP_SHARED,fd_lcd,0);
	if(plcd == NULL) {
		perror("mmap failed");
		return;
	}
	for (i = 0; i < 480; ++i) { // ��ʼ������
		for(j = 0; j < 800; ++j) {
			*(plcd + 800 * i + j) = 0x050505;
		}
	}
	draw(starty, startx); 			// ��ʾ��ʼ����Ϸ���棬 ����  map[][]   ��ʾ 16 ������ĸ���
	showImage3(imgs[13], 657, 25);
	showImage3(imgs[12], 657, 405); // ���¿�ʼ��ť����
	setScore();    					// ��ʼ����
	showWord();
}
// ��Ϸ���У��ȴ�����
void WaitTouch() {
	fd_touch = open("/dev/input/event0", O_RDWR);
	//printf("test\n");
	if(fd_touch < 0) {
		perror("��ʧ��");
		return;
	}
	struct input_event ev;
	int ret;
	int i, x1, y1;
	int dirX0, dirY0, dirX1, dirY1;
//	printf("test2\n");
	while(1) {
		ret = read(fd_touch, &ev, sizeof(ev));
		//printf("δ����");
		if(ret != sizeof(ev)) {
			continue;
		}
		if(ev.type == EV_ABS && ev.code == ABS_X) {
			x1 = ev.value;
//			printf("x1 = %d\n", x1);
		} else if(ev.type == EV_ABS && ev.code == ABS_Y) {
			y1 = ev.value;
//			printf("y1 = %d\n", y1);
		} else if(ev.type == EV_KEY && ev.code == BTN_TOUCH && ev.value == 0) {  // �����յ�
//			printf("leave\n");
			dirX1 = x1;
			dirY1 = y1;
		} else if(ev.type == EV_KEY && ev.code == BTN_TOUCH && ev.value == 1) {  // �������
			// �հ���
			dirX0 = x1;
			dirY0 = y1;
			//  restart ������������
			if(dirX0 > 657 && dirX0 < 757 && dirY0 > 405 && dirY0 < 455) {
				reDrawMap();  // ��Ϸ�����ػ�
				continue;
			}
			dirX1 = dirY1 = -1;
		} else if(dirX0 != -1 && dirY0 != -1 && dirX1 != -1 && dirY1 != -1) {   // ��������
			GameRun(dirX0, dirY0, dirX1, dirY1);  								// ����֮��Ľ���״̬����
		}
	}
}
void GameRun(int dirX0, int dirY0, int dirX1, int dirY1) {  					// ͼƬ����
	int lx = abs(dirX0 - dirX1);
	int ly = abs(dirY0 - dirY1);
	if(lx < 50 && ly < 50) {
		return;
	}
	int i;
	if(lx >= ly) { 																// ˮƽ����仯��
		if(dirX1 - dirX0 > 0) {     											// �����ƶ�
//					printf("right\n");
			GameControl(0);
		} else {
//					printf("left\n");  											// �����ƶ�
			GameControl(1);
		}
	} else {
		if(dirY1 - dirY0 <= 0) {
//					printf("up\n");
			GameControl(2);
		} else {
//					printf("down\n");
			GameControl(3);
		}
	}
	for(i = 1; i <= 4; ++i) {
		if(defMove[i] != -1) {
			new3();
			GameOver();
//			printf("%d \n", score);												// �����µķ���, ���һử����
			if(score > oldScore) {
				setScore();
				oldScore = score;
			}
			break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// ���Ʋ���
void draw(int starty, int startx) {  											// ����Ļ����ʾ��ǰ�ĵ�ͼ
	// ������ͼ���ػ�
	int i, j;
	int id;
	for(i=1; i <= 4; ++i) {
		for(j = 1; j <= 4; ++j) {
			id = findBmpId(map[i][j]);
			showImage3(imgs[id], locational[i][j].j, locational[i][j].i);
		}
	}
}
// �������¿�ʼ��֮���ػ��ͼ
void reDrawMap() {
	//�����������ֵ��߳�
	kill(pid, 9);
	pid = fork();
	//�̴߳����ɹ�
	if(pid==0) {
		execlp("madplay", "madplay", buf, "-r", NULL);							//����madplay��������
	}
	score = 0;
	oldScore = 0;
	int i, j, xl, yl;
	for(i = 1; i <= 4; ++i) { // ��ͼ�ػ�
		for(j = 1; j <= 4; ++j) {
			map[i][j] = 0;
		}
	}
	i = 0;
	while(i < 3) {  // �������λ�ò��� 3 �� 2
		srandom(time(NULL)); // set seed
		xl = random()%4+1;
		srandom(time(NULL)+444);
		yl = random()%4+1;
		if(map[xl][yl] == 0) {
			map[xl][yl] = 2;
			i++;
		}
	}
	for (i = 0; i < 480; ++i) { // ��ʼ������
		for(j = 0; j < 800; ++j) {
			*(plcd + 800 * i + j) = 0x050505;
		}
	}

	showImage3(imgs[13], 657, 25);
	showImage3(imgs[12], 657, 405); // ���¿�ʼ��ť����

	setScore();    			// ��ʼ����
	draw(starty, startx);  // �����ػ�
	showWord();
}
void showWord() {
	int l1, r1;
	l1 = 60;
	r1 = 60;
	lcd_draw_word(10+l1,10+r1,32,33,data1,0xfcfcfc);
	lcd_draw_word(10+l1,42+r1,32,33,data2,0xfcfcfc);
	lcd_draw_word(10+l1,72+r1,32,33,data3,0xfcfcfc);
	lcd_draw_word(10+l1,104+r1,32,33,data4,0xfcfcfc);
	lcd_draw_word(10+l1,136+r1,32,33,data5,0xfcfcfc);
	lcd_draw_word(10+l1,168+r1,32,33,data6,0xfcfcfc);
	lcd_draw_word(10+l1,200+r1,32,33,data7,0xfcfcfc);
	lcd_draw_word(10+l1,232+r1,32,33,data8,0xfcfcfc);
	lcd_draw_word(10+l1,264+r1,32,33,data9,0xfcfcfc);
}


void setScore() { // ���Ʒ���
	int x, y;
	x = 657 + 60;
	y = 205;
//	printf("%d \n", score);
	if(score >= 100) {
		showImage3(numbers[score%10], x, y);
		showImage3(numbers[score%100/10], x-30, y);
		showImage3(numbers[score/100], x-60, y);
	} else if(score >= 10) {
//		printf("%d \n", score);
		showImage3(numbers[score%10], x, y);
		showImage3(numbers[score/10], x-30, y);
	} else {
		showImage3(numbers[score%10], x, y);
	}
}
///////////////////////////////////////////////////
void initImge() {  // ����Դ�����ڴ�
	int i = 0, j;
	int xl, yl;
	imgs[0]    = open("./res/0.bmp", O_RDWR);
	imgs[1]    = open("./res/2.bmp", O_RDWR);
	imgs[2]    = open("./res/4.bmp", O_RDWR);
	imgs[3]    = open("./res/8.bmp", O_RDWR);
	imgs[4]    = open("./res/16.bmp", O_RDWR);
	imgs[5]    = open("./res/32.bmp", O_RDWR);
	imgs[6]    = open("./res/64.bmp", O_RDWR);
	imgs[7]    = open("./res/128.bmp", O_RDWR);
	imgs[8]    = open("./res/256.bmp", O_RDWR);
	imgs[9]    = open("./res/512.bmp", O_RDWR);
	imgs[10]   = open("./res/1024.bmp", O_RDWR);
	imgs[11]   = open("./res/2048.bmp", O_RDWR);
	numbers[0] = open("./res/n0.bmp", O_RDWR);  // ����ͼƬ
	numbers[1] = open("./res/n1.bmp", O_RDWR);
	numbers[2] = open("./res/n2.bmp", O_RDWR);
	numbers[3] = open("./res/n3.bmp", O_RDWR);
	numbers[4] = open("./res/n4.bmp", O_RDWR);
	numbers[5] = open("./res/n5.bmp", O_RDWR);
	numbers[6] = open("./res/n6.bmp", O_RDWR);
	numbers[7] = open("./res/n7.bmp", O_RDWR);
	numbers[8] = open("./res/n8.bmp", O_RDWR);
	numbers[9] = open("./res/n9.bmp", O_RDWR);
	imgs[12]   = open("./res/restart.bmp", O_RDWR);  // ���¿�ʼ��ť
	imgs[13]   = open("./res/score.bmp", O_RDWR);  // ���¿�ʼ��ť
	imgs[14]   = open("./res/gameOver.bmp", O_RDWR);
	while(i < 3) {  // �������λ�ò��� 3 �� 2
		srandom(time(NULL)); // set seed
		xl = random()%4+1;
		srandom(time(NULL)+444);
		yl = random()%4+1;
		if(map[xl][yl] == 0) {
			map[xl][yl] = 2;
			i++;
		}
	}
	for(i = 0; i <= 5; ++i) {
		map[0][i] = -1;
		map[5][i] = -1;
		map[i][0] = -1;
		map[i][5] = -1;
	}
	int jianxi1 = 0;
	int jianxi2 = 0;
	for(i=1; i <= 4; ++i) {
		for(j = 1; j <= 4; ++j) {
			local[4*(i-1)+(j-1)].i = i;
			local[4*(i-1)+(j-1)].j = j;
			locational[i][j].j = starty+(j-1)*100 + jianxi1; // x ��
			locational[i][j].i = jianxi2 + startx+(i-1)*100; // y ��, ÿ��СͼƬ��λ�ö���ȷ����
			jianxi1 += 10;
		}
		jianxi1 = 0;
		jianxi2 += 10;
	}
}
void downImage() {   // �ر�ͼƬ�ļ�
	int i;
	for(i=0; i<11; ++i) {
		close(imgs[i]);

	}

}
int findBmpId(int x) {  // ���� map[i][j] ��ֵ�ж�ʹ������ͼƬ
	if(x == 0) {
		return 0;
	}
	return log(x)/log(2);
}
void showImage(int fd_bmp, int x1, int y1) { // �� 100 * 100 ͼƬ��ʾ����Ļ���ض���λ��
	int start=0x36; // ƫ�� 36���ֽڿ�ʼ��ȡͼƬ������
	int color;
	int ret, i, j;
	char buf[4];
	if(x1 < 800 && x1 > 0 && y1 < 480 && y1 > 0) {
		lseek(fd_bmp, start, SEEK_SET);
		for(i = 0; i < 100; ++i) {
			for(j = x1; j < x1+100; ++j) {
				color = 0;
				ret = read(fd_bmp, buf, 4);
				if(ret<0) {
					perror("read width failed\n");
				}
				color = (buf[3] << 24)|(buf[2] << 16)|(buf[1] << 8) | buf[0];
				*(plcd + 800 * (100 - i + y1) + j) = color;
				lseek(fd_bmp, start+=3, SEEK_SET);
			}
		}
	} else {
		perror("local x, y erro");
	}
}
/////////////////////////////////////////////
void new3() { // �ڿհ׵�λ�ó����µ�Ԫ��
	int i, j;
	int k=0;
	int tmp;
	while(1) {
		step = (step+1)%16; 	// �������ϴγ��ֵ�λ��
		i = local[step].i;
		j = local[step].j;
		if(map[i][j] == 0) {

			srandom(time(NULL)); // set seed
			tmp = random()%2;
			if(tmp == 0) {
				map[i][j] = 2;
			} else {
				map[i][j] = 4;
			}
			showImage(imgs[tmp+1], locational[i][j].j, locational[i][j].i);
			break;
		}
		if(k > 18) { // ������û�ҵ�����ѭ��
			break;
		}
		k++;
	}
}
void GameControl(int dir) {  // ������
	int i, j, tmp, k, id;
	for(i=1; i <= 4; ++i) {  // ������е��ж�û�б�����
		defMove[i] = -1;
	}
	switch(dir) {
		case 0: // ����
			for(i = 1; i <= 4; ++i) {
				for(j = 3; j >= 1; --j) {
					// �ڻ�����ʱ���ֱ�ӻ�����
					if(map[i][j] != 0) { 	// �� 0 Ԫ�ز���Ҫ�ж�
						k = j+1;
						while(map[i][k] == 0) {
							k++;
						}
						if(map[i][j] == map[i][k]) {
							map[i][k] *= 2;
							map[i][j] = 0;
							++score;
							// ������ֱ�ӻ�����
							id = findBmpId(map[i][j]);
							showImage3(imgs[id], locational[i][j].j, locational[i][j].i);
							id = findBmpId(map[i][k]);
							showImage3(imgs[id], locational[i][k].j, locational[i][k].i);
							defMove[i] = 1; // ���һ�£���һ�иı��
						} else { 			//if(map[i][k] == -1)
							if(k-j == 1) {  // ��Ч����
								continue;
							}
							// map[i][k] ֻ��Ϊ -1 ������Чֵ
							tmp = map[i][j];
							map[i][j] = 0;
							map[i][k-1] = tmp;
							// ������ֱ�ӻ�����
							id = findBmpId(map[i][j]);
							showImage3(imgs[id], locational[i][j].j, locational[i][j].i);

							id = findBmpId(map[i][k-1]);
							showImage3(imgs[id], locational[i][k-1].j, locational[i][k-1].i);

							defMove[i] = 1; // ���һ�£���һ�иı��
						}
					}
				}
			}
			break;
		case 1: // ����
			for(i = 1; i <= 4; ++i) {
				for(j = 2; j <= 4; ++j) {
					if(map[i][j] != 0) {
						k = j-1;
						while(map[i][k] == 0) {
							k--;
						}
						if(map[i][j] == map[i][k]) {
							map[i][k] *= 2;
							map[i][j] = 0;
							++score;
							id = findBmpId(map[i][j]);
							showImage3(imgs[id], locational[i][j].j, locational[i][j].i);
							id = findBmpId(map[i][k]);
							showImage3(imgs[id], locational[i][k].j, locational[i][k].i);
							defMove[i] = 1; // ���һ�£���һ�иı��
						} else  {
							if(k == j-1) {
								continue;
							}
							tmp = map[i][j];
							map[i][j] = 0;
							map[i][k+1] = tmp;


							id = findBmpId(map[i][j]);
							showImage3(imgs[id], locational[i][j].j, locational[i][j].i);
							id = findBmpId(map[i][k+1]);
							showImage3(imgs[id], locational[i][k+1].j, locational[i][k+1].i);
							defMove[i] = 1; // ���һ�£���һ�иı��
						}
					}
				}
			}
			break;
		case 2: 					  		// ����
			for(i = 1; i <= 4; ++i) { 		// i ��ʾ����  ��
				for(j = 2; j <= 4; ++j) {
					if(map[j][i] != 0) {
						k = j-1;
						while(map[k][i] == 0) {
							k--;
						}
						if(map[j][i] == map[k][i]) {
							map[k][i] *= 2;
							map[j][i] = 0;
							++score;
							id = findBmpId(map[j][i]);
							showImage3(imgs[id], locational[j][i].j, locational[j][i].i);

							id = findBmpId(map[k][i]);
							showImage3(imgs[id], locational[k][i].j, locational[k][i].i);
							defMove[i] = 1; // ���һ�£���һ�иı��

						} else  {
							if(k==j-1) { 	// û�в������˳�
								continue;
							}
							tmp = map[j][i];
							map[j][i] = 0;
							map[k+1][i] = tmp;
							id = findBmpId(map[j][i]);
							showImage3(imgs[id], locational[j][i].j, locational[j][i].i);
							id = findBmpId(map[k+1][i]);
							showImage3(imgs[id], locational[k+1][i].j, locational[k+1][i].i);
							defMove[i] = 1; // ���һ�£���һ�иı��
						}
					}
				}
			}
			break;
		case 3: // ����
			for(i = 1; i <= 4; ++i) {
				for(j = 3; j >= 1; --j) {
					if(map[j][i] != 0) {
						k = j+1;
						while(map[k][i] == 0) {
							k++;
						}
						if(map[j][i] == map[k][i]) {
							map[k][i] *= 2;
							map[j][i] = 0;
							++score;
							id = findBmpId(map[j][i]);
							showImage3(imgs[id], locational[j][i].j, locational[j][i].i);

							id = findBmpId(map[k][i]);
							showImage3(imgs[id], locational[k][i].j, locational[k][i].i);
							defMove[i] = 1; // ���һ�£���һ�иı��
						} else {
							if(k==j+1) {
								continue;
							}
							tmp = map[j][i];
							map[j][i] = 0;
							map[k-1][i] = tmp;

							id = findBmpId(map[j][i]);
							showImage3(imgs[id], locational[j][i].j, locational[j][i].i);

							id = findBmpId(map[k-1][i]);
							showImage3(imgs[id], locational[k-1][i].j, locational[k-1][i].i);
							defMove[i] = 1; // ���һ�£���һ�иı��
						}
					}
				}
			}
			break;
	}
}
void showImage3(int fd, int x0, int y0) {
	unsigned char buf[4];
	int ret;
	int width,height;
	unsigned char b ,g, r ,a;
	int color;
	int i = 0;
	int x;
	int y;
	int depth;
	lseek(fd, 0 ,SEEK_SET);
	read(fd, buf, 2);

	lseek(fd, 0x12, SEEK_SET);
	read(fd, buf, 4);
	width =  (buf[0]) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	lseek(fd, 0x16, SEEK_SET);
	read(fd, buf, 4);
	height =  (buf[0]) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

	lseek(fd, 0x1c, SEEK_SET);
	read(fd, buf, 2);
	depth = (buf[0]) | (buf[1] << 8);
	if (depth != 24 && depth != 32) {
		printf("Not Support\n");
		close(fd);
		return ;
	}
	int bytes_per_line = abs(width) * (depth / 8);
	int laizi = 0;
	if (bytes_per_line % 4 != 0) {
		laizi = 4 - bytes_per_line % 4;
	}
	int bytes_line = bytes_per_line + laizi;
	int size = bytes_line * abs(height);
	unsigned char *p = malloc(size);
	lseek(fd, 54, SEEK_SET);
	read(fd, p, size);
	for (y = 0; y < abs(height); y++) {
		for (x = 0 ; x  < abs(width); x++ ) {
			b = p[i++];
			g = p[i++];
			r = p[i++];
			if (depth == 32) {
				a = p[i++];
			} else {
				a = 0;
			}
			color = (a << 24) | (r << 16) | (g << 8) | (b);
			LCD_Draw_Point( width > 0 ? x0 + x : x0 + abs(width) - 1 -x,
			                height > 0 ? y0 + height - 1 - y : y0 + y,
			                color);
		}
		i = i + laizi;
	}
	free(p);
}
void LCD_Draw_Point(int x, int y, int color) {
	if (x >= 0 &&  x < 800 &&  y >= 0 && y < 480) {
		*(plcd + 800*y + x) = color;
	}
}
void GameEnd() { 				// ��Դ�ͷ�
	close(fd_touch);  			// �ر���Ļ�ļ�
	munmap(plcd, 800*480*4);  	// �ͷ�ӳ��
	downImage();         		// �ر�ͼƬ�ļ�
}
int GameOver() {
	int i, j;
	int cnt = 0;
	int k;
	int x1[] = {0, 0, 1, -1};
	int y1[] = {-1, 1, 0, 0};
	for(i = 1; i <= 4; ++i) {
		for(j = 1; j <= 4; ++j) {
			if(map[i][j] == 0) {
				continue;
			}
			for(k = 0; k < 4; ++k) {
				if(map[i][j] == map[i+x1[k]][j+y1[k]]) {
					break;
				}
			}
			if(k == 4) {
				cnt++;
			}
		}
	}
	if(cnt == 16) {
		showImage3(imgs[14], 200, 90);
		kill(pid,9);						//ɱ���������ֵĽ��̣�ֹͣ��������
		return -1;  						// ��Ϸ����
	} else {
		return 1;  							// ��Ϸ����
	}

}

void lcd_draw_word(int x0,int y0,int w,int h,char *data,int color) {
	int i,k;
	for(i=0; i < w*h/8; i++) {
		for(k=0; k<8; k++) {
			if((data[i]<<k)&0x80) {
				LCD_Draw_Point(x0+(i*8+k)%w,y0+i/(w/8),color);
			}
		}
	}
}
