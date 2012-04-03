#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include "SDL.h"
#include "SDL_thread.h"

#include <android/log.h>
#define INFO(msg) __android_log_write(ANDROID_LOG_INFO,"video.c",msg);

int main(int argc, char *argv[])
{
  AVFormatContext *pFormatCtx;
  int             i, videoStream;
  AVCodecContext  *pCodecCtx;
  AVCodec         *pCodec;
  AVFrame         *pFrame; 
  AVPacket        packet;
  int             frameFinished;
  float           aspect_ratio;
  static struct SwsContext *img_convert_ctx;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* txt;
void* pixels;
int pitch;
Uint32 format;
int access;
int w;
int h;
SDL_Rect        rect;
SDL_Event       event;


  if(argc < 2) {
    INFO("argc < 2");
    return 1;
  }

INFO("main begin. argv[1] as follow");
INFO(argv[1]);

/*****************************************************/
/* FFmpeg */
/*****************************************************/

  av_register_all();

  if(av_open_input_file(&pFormatCtx, argv[1], NULL, 0, NULL)!=0)
  {
    INFO("av_open_file failed");
    return -1;
  }

INFO("stream info found");
  
  if(av_find_stream_info(pFormatCtx)<0)
    return -1;
  
  //dump_format(pFormatCtx, 0, argv[1], 0);
  
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return -1;

INFO("videoStream found");
  
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
    INFO("Unsupported codec!\n");
    return -1;
  }
  
  if(avcodec_open(pCodecCtx, pCodec)<0)
    return -1;
  
  pFrame=avcodec_alloc_frame();

/*****************************************************/
/* /FFmpeg */
/*****************************************************/

/*****************************************************/
/* SDL init */
/*****************************************************/
  
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
    INFO("Could not initialize SDL");
    return 1;
  }
  
//********** Window and Renderer **********//

// Create the window where we will draw.
window = SDL_CreateWindow("Texture - Window - Video",
                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                        512, 512,
                        SDL_WINDOW_SHOWN);

// We must call SDL_CreateRenderer in order for draw calls to affect this window.
renderer = SDL_CreateRenderer(window, -1, 0);
/*
SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
SDL_RenderClear(renderer);
SDL_RenderPresent(renderer);
SDL_Delay(100);
INFO("render present done. white window");
*/


//********** texture **********//

txt = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, 128, 128);

if(txt == NULL) INFO("txt == NULL");
SDL_QueryTexture(txt, &format, &access, &w, &h);
if(format == SDL_PIXELFORMAT_YV12) INFO("query txt, format: YV12");
if(access == SDL_TEXTUREACCESS_STREAMING) INFO("query txt, access: STREAMING");
/*****************************************************/
/* /SDL init */
/*****************************************************/


INFO("codec name:");
INFO(pCodec->name);
INFO("before this is the codec name. Getting into stream decode:");
/*****************************************************/
/* video stream */
/*****************************************************/

  i=0;
  while(av_read_frame(pFormatCtx, &packet)>=0) {
    if(packet.stream_index==videoStream) {
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, 
			   &packet);
      if(frameFinished) {
INFO("frame decoded finished.");

SDL_LockTexture(txt, NULL, &pixels, &pitch);

	AVPicture pict;

/*
	pict.data[0] = (Uint32*)pixels + 0 * format;
	pict.data[1] = (Uint32*)pixels + 2 * format;
	pict.data[2] = (Uint32*)pixels + 1 * format;

	//??? try it first
	pict.linesize[0] = pitch;
	pict.linesize[1] = pitch;
	pict.linesize[2] = pitch;
*/

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
 pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
 PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pict.data, pict.linesize);
	
SDL_UnlockTexture(txt);

/*
rect.x = 0;
rect.y = 0;
rect.w = pCodecCtx->width;
rect.h = pCodecCtx->height;
*/

//int
//SDL_RenderCopy(renderer, txt, NULL, NULL);
//SDL_RenderClear(renderer);
//SDL_RenderPresent(renderer);
      
      }

    }

    av_free_packet(&packet);
/*
    SDL_PollEvent(&event);
    switch(event.type) {
    case SDL_QUIT:
      SDL_Quit();
      exit(0);
      break;
    default:
      break;
    }
*/
  }

/*****************************************************/
/* /video stream */
/*****************************************************/

  av_free(pFrame);
  
  avcodec_close(pCodecCtx);
  
  av_close_input_file(pFormatCtx);
  
  return 0;
}
