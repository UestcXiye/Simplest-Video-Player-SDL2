// Simplest Video Player SDL2.cpp : 定义控制台应用程序的入口点。
//

/**
* 最简单的SDL2播放视频的例子（SDL2播放RGB/YUV）
* Simplest Video Play SDL2 (SDL2 play RGB/YUV)
*
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本程序使用SDL2播放RGB/YUV视频像素数据。
* SDL实际上是对底层绘图API（Direct3D，OpenGL）的封装，使用起来明显简单于直接调用底层API。
*
* This software plays RGB/YUV raw video data using SDL2.
* SDL is a wrapper of low-level API (Direct3D, OpenGL).
* Use SDL is much easier than directly call these low-level API.
*/

#include "stdafx.h"

#include <stdio.h>

extern "C"
{
#include "sdl/SDL.h"
};

// 报错：
// LNK2019 无法解析的外部符号 __imp__fprintf，该符号在函数 _ShowError 中被引用
// LNK2019 无法解析的外部符号 __imp____iob_func，该符号在函数 _ShowError 中被引用

// 解决办法：
// 包含库的编译器版本低于当前编译版本，需要将包含库源码用vs2017重新编译，由于没有包含库的源码，此路不通。
// 然后查到说是stdin, stderr, stdout 这几个函数vs2015和以前的定义得不一样，所以报错。
// 解决方法呢，就是使用{ *stdin,*stdout,*stderr }数组自己定义__iob_func()
#pragma comment(lib,"legacy_stdio_definitions.lib")
extern "C"
{
	FILE __iob_func[3] = { *stdin,*stdout,*stderr };
}

const int bpp = 12;

int screen_w = 640, screen_h = 360;
const int pixel_w = 320, pixel_h = 180;

unsigned char buffer[pixel_w * pixel_h * bpp / 8];

// 自定义消息类型
#define REFRESH_EVENT  (SDL_USEREVENT + 1) // Refresh Event
#define BREAK_EVENT  (SDL_USEREVENT + 2) // Break

// 线程标志位
int thread_exit = 0;// 退出标志，等于1则退出
int thread_pause = 0;// 暂停标志，等于1则暂停

// 视频播放相关参数
int delay_time = 40;

bool video_gray = false;// 是否显示黑白图像

// 画面刷新线程
int refresh_video(void *opaque)
{
	while (thread_exit == 0)
	{
		if (thread_pause == 0)
		{
			SDL_Event event;
			event.type = REFRESH_EVENT;
			// 向主线程发送刷新事件
			SDL_PushEvent(&event);
		}
		// 工具函数，用于延时
		SDL_Delay(delay_time);
	}
	thread_exit = 0;
	thread_pause = 0;
	// 需要结束播放
	SDL_Event event;
	event.type = BREAK_EVENT;
	// 向主线程发送退出循环事件
	SDL_PushEvent(&event);

	return 0;
}

int main(int argc, char* argv[])
{
	// 初始化SDL系统
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("Couldn't initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	SDL_Window *screen;
	// SDL 2.0 Support for multiple windows
	// 创建窗口SDL_Window
	screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen)
	{
		printf("SDL: Couldn't create window - exiting:%s\n", SDL_GetError());
		return -1;
	}

	// 创建渲染器SDL_Renderer
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

	Uint32 pixformat = 0;
	// IYUV: Y + U + V  (3 planes)
	// YV12: Y + V + U  (3 planes)
	pixformat = SDL_PIXELFORMAT_IYUV;

	// 创建纹理SDL_Texture
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);

	FILE *fp = NULL;
	const char file_path[] = "test_yuv420p_320x180.yuv";// 视频文件路径
	fp = fopen(file_path, "rb+");

	if (fp == NULL)
	{
		printf("Can't open this file\n");
		return -1;
	}

	SDL_Rect sdlRect;// 渲染显示面积

	// 创建画面刷新线程
	SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
	SDL_Event event;// 主线程使用的事件

	while (1)
	{
		// Wait
		SDL_WaitEvent(&event);// 从事件队列中取事件
		if (event.type == REFRESH_EVENT)
		{
			// YUV420P文件格式：
			// 一帧数据中，Y、U、V连续存储，其中Y占pixel_w*pixel_h长度，U、Y各占pixel_w*pixel_h/4长度
			if (fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp) != pixel_w * pixel_h * bpp / 8)
			{
				// Loop
				fseek(fp, 0, SEEK_SET);
				fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp);
			}
			// 若选择显示黑白图像，则将buffer的U、V数据设置为128
			if (video_gray == true)
			{
				// U、V是图像中的经过偏置处理的色度分量
				// 在偏置处理前，它的取值范围是-128-127，这时，把U和V数据修改为0代表无色
				// 在偏置处理后，它的取值范围变成了0-255，所以这时候需要取中间值，即128
				memset(buffer + pixel_w * pixel_h, 128, pixel_w * pixel_h / 2);
			}
			// 设置纹理的数据
			SDL_UpdateTexture(sdlTexture, NULL, buffer, pixel_w);

			// FIX: If window is resize
			sdlRect.x = 0;
			sdlRect.y = 0;
			sdlRect.w = screen_w;
			sdlRect.h = screen_h;

			// 使用图形颜色清除当前的渲染目标
			SDL_RenderClear(sdlRenderer);
			// 将纹理的数据拷贝给渲染器
			SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
			// 显示纹理的数据
			SDL_RenderPresent(sdlRenderer);
		}
		else if (event.type == SDL_KEYDOWN)
		{
			// 根据按下键盘键位决定事件
			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				thread_exit = 1;// 按下ESC键，直接退出播放器
				break;
			case SDLK_SPACE:
				thread_pause = !thread_pause;// 按下Space键，控制视频播放暂停
				break;
			case SDLK_F1:
				delay_time += 10;// 按下F1，视频减速
				break;
			case SDLK_F2:
				if (delay_time > 10)
				{
					delay_time -= 10;// 按下F2，视频加速
				}
				break;
			case SDLK_LSHIFT:
				video_gray = !video_gray;// 按下左Shift键，切换显示彩色/黑白图像
				break;
			default:
				break;
			}
		}
		else if (event.type == SDL_WINDOWEVENT)
		{
			// If Resize
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
		}
		else if (event.type == SDL_QUIT)
		{
			thread_exit = 1;
		}
		else if (event.type == BREAK_EVENT)
		{
			break;
		}
	}

	// 释放资源
	fclose(fp);
	SDL_DestroyTexture(sdlTexture);
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(screen);
	// 退出SDL系统
	SDL_Quit();

	return 0;
}
