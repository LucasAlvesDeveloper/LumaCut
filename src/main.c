#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include <libavcodec/packet.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>

#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define TESTFILE_PATH "assets/videos/timing.mp4"

typedef struct Context {
	SDL_Window *window;
	SDL_Renderer *renderer;
} Context;

Context context = {
	.window = NULL,
	.renderer = NULL,
};

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int y;

	// Open file
	sprintf(szFilename, "frame%d.ppm", iFrame);
	pFile = fopen(szFilename, "wb");
	if (pFile == NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for (y = 0; y < height; y++)
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

	// Close file
	fclose(pFile);
}

int main(void) {
	// Abrir o arquivo;
	AVFormatContext *file_ctx = avformat_alloc_context();
	avformat_open_input(&file_ctx, TESTFILE_PATH, NULL, NULL);
	if (file_ctx == NULL) {
		fprintf(stderr, "Could not open file! check if path is correct.\n");
		return 1;
	}

	// Preenche as informações do arquivo
	if (avformat_find_stream_info(file_ctx, NULL) < 0) {
		fprintf(stderr, "Could not get stream information!\n");
		return 1;
	}

	// Imprime as informações para debug
	av_dump_format(file_ctx, 0, TESTFILE_PATH, 0);

	// Achar a faixa de vídeo
	int video_stream_index = -1;
	for (size_t i = 0; i < file_ctx->nb_streams; i++) {
		AVStream *stream = file_ctx->streams[i];

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			break;
		}
	}

	if (video_stream_index < 0) {
		fprintf(stderr, "Could not find a video stream on file.\n");
		return 1;
	}

	// Acha o decodificador do vídeo
	const AVCodec *video_decoder = avcodec_find_decoder(file_ctx->streams[video_stream_index]->codecpar->codec_id);

	// Cria o contexto e preenche com as informações desse decoder
	AVCodecContext *video_decoder_ctx = avcodec_alloc_context3(video_decoder);
	avcodec_parameters_to_context(video_decoder_ctx, file_ctx->streams[video_stream_index]->codecpar);

	if (avcodec_open2(video_decoder_ctx, video_decoder, NULL) < 0) {
		fprintf(stderr, "Could not open video decoder.\n");
		return 1;
	}

	AVFrame *orig_frame = av_frame_alloc();
	AVFrame *frame_rgb = av_frame_alloc();

	size_t buff_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, video_decoder_ctx->width, video_decoder_ctx->height, 16);
	uint8_t *buffer = av_malloc(buff_size * sizeof(uint8_t));

	av_image_fill_arrays(
		frame_rgb->data, frame_rgb->linesize, buffer, AV_PIX_FMT_YUV420P, video_decoder_ctx->width, video_decoder_ctx->height, 16
	);

	SwsContext *sws_ctx = sws_getContext(
		video_decoder_ctx->width,
		video_decoder_ctx->height,
		video_decoder_ctx->pix_fmt,
		video_decoder_ctx->width,
		video_decoder_ctx->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);

	AVPacket *packet = av_packet_alloc();

	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	context.window = SDL_CreateWindow("Player de Vídeo Massa!", 1280, 720, SDL_WINDOW_HIDDEN);
	context.renderer = SDL_CreateRenderer(context.window, NULL);
	SDL_SetRenderVSync(context.renderer, 1);
	SDL_ShowWindow(context.window);

	SDL_Texture *rendered_frame = SDL_CreateTexture(
		context.renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, video_decoder_ctx->width, video_decoder_ctx->height
	);

	uint64_t startTime = SDL_GetTicks();
	uint64_t endTime = SDL_GetTicks();
	double deltaTimeSec = 0.0;

	double counter = 0.0;

	const double fps = av_q2d(file_ctx->streams[video_stream_index]->r_frame_rate);

	bool running = true;
	while (running) {
		endTime = SDL_GetTicks();
		deltaTimeSec = (double)(endTime - startTime) / 1000.0;
		startTime = endTime;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_QUIT:
				running = false;
				break;
			default:
				break;
			}
		}

		counter += deltaTimeSec;

		if (counter >= 1.0 / 60.0) {
			counter = 0.0;
			if (av_read_frame(file_ctx, packet) == 0) {
				if (packet->stream_index == video_stream_index) {
					if (avcodec_send_packet(video_decoder_ctx, packet) == 0) {
						while (avcodec_receive_frame(video_decoder_ctx, orig_frame) == 0) {
							sws_scale(
								sws_ctx,
								(uint8_t const *const *)orig_frame->data,
								orig_frame->linesize,
								0,
								video_decoder_ctx->height,
								frame_rgb->data,
								frame_rgb->linesize
							);

							SDL_UpdateYUVTexture(
								rendered_frame,
								NULL,
								frame_rgb->data[0],
								frame_rgb->linesize[0],
								frame_rgb->data[1],
								frame_rgb->linesize[1],
								frame_rgb->data[2],
								frame_rgb->linesize[2]
							);
						}
					}
				}
				av_packet_unref(packet);
			}
		}

		SDL_SetRenderDrawColor(context.renderer, 200, 200, 200, 255);
		SDL_RenderClear(context.renderer);

		SDL_RenderTexture(context.renderer, rendered_frame, NULL, NULL);

		SDL_SetRenderScale(context.renderer, 2.f, 2.f);
		SDL_RenderDebugTextFormat(context.renderer, 0, 0, "FPS: %i", (int)(1.0 / deltaTimeSec));
		SDL_RenderDebugTextFormat(context.renderer, 0, 8, "Frame time: %f", deltaTimeSec);
		SDL_SetRenderScale(context.renderer, 1.f, 1.f);

		SDL_RenderPresent(context.renderer);
	}

	SDL_DestroyTexture(rendered_frame);

	SDL_DestroyRenderer(context.renderer);
	context.renderer = NULL;
	SDL_DestroyWindow(context.window);
	context.window = NULL;

	SDL_Quit();

	sws_freeContext(sws_ctx);
	sws_ctx = NULL;
	av_free(buffer);
	buffer = NULL;
	av_frame_free(&frame_rgb);
	av_frame_free(&orig_frame);
	avcodec_free_context(&video_decoder_ctx);
	avformat_close_input(&file_ctx);

	return 0;
}
