// Simplest Video Player SDL2.cpp : �������̨Ӧ�ó������ڵ㡣
//

/**
* ��򵥵�SDL2������Ƶ�����ӣ�SDL2����RGB/YUV��
* Simplest Video Play SDL2 (SDL2 play RGB/YUV)
*
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* �޸ģ�
* ���ĳ� Liu Wenchen
* 812288728@qq.com
* ���ӿƼ���ѧ/������Ϣ
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* ������ʹ��SDL2����RGB/YUV��Ƶ�������ݡ�
* SDLʵ�����ǶԵײ��ͼAPI��Direct3D��OpenGL���ķ�װ��ʹ���������Լ���ֱ�ӵ��õײ�API��
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

// ����
// LNK2019 �޷��������ⲿ���� __imp__fprintf���÷����ں��� _ShowError �б�����
// LNK2019 �޷��������ⲿ���� __imp____iob_func���÷����ں��� _ShowError �б�����

// ����취��
// ������ı������汾���ڵ�ǰ����汾����Ҫ��������Դ����vs2017���±��룬����û�а������Դ�룬��·��ͨ��
// Ȼ��鵽˵��stdin, stderr, stdout �⼸������vs2015����ǰ�Ķ���ò�һ�������Ա���
// ��������أ�����ʹ��{ *stdin,*stdout,*stderr }�����Լ�����__iob_func()
#pragma comment(lib,"legacy_stdio_definitions.lib")
extern "C"
{
	FILE __iob_func[3] = { *stdin,*stdout,*stderr };
}

const int bpp = 12;

int screen_w = 640, screen_h = 360;
const int pixel_w = 320, pixel_h = 180;

unsigned char buffer[pixel_w * pixel_h * bpp / 8];

// �Զ�����Ϣ����
#define REFRESH_EVENT  (SDL_USEREVENT + 1) // Refresh Event
#define BREAK_EVENT  (SDL_USEREVENT + 2) // Break

// �̱߳�־λ
int thread_exit = 0;// �˳���־������1���˳�
int thread_pause = 0;// ��ͣ��־������1����ͣ

// ��Ƶ������ز���
int delay_time = 40;

bool video_gray = false;// �Ƿ���ʾ�ڰ�ͼ��

// ����ˢ���߳�
int refresh_video(void *opaque)
{
	while (thread_exit == 0)
	{
		if (thread_pause == 0)
		{
			SDL_Event event;
			event.type = REFRESH_EVENT;
			// �����̷߳���ˢ���¼�
			SDL_PushEvent(&event);
		}
		// ���ߺ�����������ʱ
		SDL_Delay(delay_time);
	}
	thread_exit = 0;
	thread_pause = 0;
	// ��Ҫ��������
	SDL_Event event;
	event.type = BREAK_EVENT;
	// �����̷߳����˳�ѭ���¼�
	SDL_PushEvent(&event);

	return 0;
}

int main(int argc, char* argv[])
{
	// ��ʼ��SDLϵͳ
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("Couldn't initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	SDL_Window *screen;
	// SDL 2.0 Support for multiple windows
	// ��������SDL_Window
	screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen)
	{
		printf("SDL: Couldn't create window - exiting:%s\n", SDL_GetError());
		return -1;
	}

	// ������Ⱦ��SDL_Renderer
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

	Uint32 pixformat = 0;
	// IYUV: Y + U + V  (3 planes)
	// YV12: Y + V + U  (3 planes)
	pixformat = SDL_PIXELFORMAT_IYUV;

	// ��������SDL_Texture
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);

	FILE *fp = NULL;
	const char file_path[] = "test_yuv420p_320x180.yuv";// ��Ƶ�ļ�·��
	fp = fopen(file_path, "rb+");

	if (fp == NULL)
	{
		printf("Can't open this file\n");
		return -1;
	}

	SDL_Rect sdlRect;// ��Ⱦ��ʾ���

	// ��������ˢ���߳�
	SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
	SDL_Event event;// ���߳�ʹ�õ��¼�

	while (1)
	{
		// Wait
		SDL_WaitEvent(&event);// ���¼�������ȡ�¼�
		if (event.type == REFRESH_EVENT)
		{
			// YUV420P�ļ���ʽ��
			// һ֡�����У�Y��U��V�����洢������Yռpixel_w*pixel_h���ȣ�U��Y��ռpixel_w*pixel_h/4����
			if (fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp) != pixel_w * pixel_h * bpp / 8)
			{
				// Loop
				fseek(fp, 0, SEEK_SET);
				fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp);
			}
			// ��ѡ����ʾ�ڰ�ͼ����buffer��U��V��������Ϊ128
			if (video_gray == true)
			{
				// U��V��ͼ���еľ���ƫ�ô����ɫ�ȷ���
				// ��ƫ�ô���ǰ������ȡֵ��Χ��-128-127����ʱ����U��V�����޸�Ϊ0������ɫ
				// ��ƫ�ô��������ȡֵ��Χ�����0-255��������ʱ����Ҫȡ�м�ֵ����128
				memset(buffer + pixel_w * pixel_h, 128, pixel_w * pixel_h / 2);
			}
			// �������������
			SDL_UpdateTexture(sdlTexture, NULL, buffer, pixel_w);

			// FIX: If window is resize
			sdlRect.x = 0;
			sdlRect.y = 0;
			sdlRect.w = screen_w;
			sdlRect.h = screen_h;

			// ʹ��ͼ����ɫ�����ǰ����ȾĿ��
			SDL_RenderClear(sdlRenderer);
			// ����������ݿ�������Ⱦ��
			SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
			// ��ʾ���������
			SDL_RenderPresent(sdlRenderer);
		}
		else if (event.type == SDL_KEYDOWN)
		{
			// ���ݰ��¼��̼�λ�����¼�
			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				thread_exit = 1;// ����ESC����ֱ���˳�������
				break;
			case SDLK_SPACE:
				thread_pause = !thread_pause;// ����Space����������Ƶ������ͣ
				break;
			case SDLK_F1:
				delay_time += 10;// ����F1����Ƶ����
				break;
			case SDLK_F2:
				if (delay_time > 10)
				{
					delay_time -= 10;// ����F2����Ƶ����
				}
				break;
			case SDLK_LSHIFT:
				video_gray = !video_gray;// ������Shift�����л���ʾ��ɫ/�ڰ�ͼ��
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

	// �ͷ���Դ
	fclose(fp);
	SDL_DestroyTexture(sdlTexture);
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(screen);
	// �˳�SDLϵͳ
	SDL_Quit();

	return 0;
}
