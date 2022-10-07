#include <gst/gst.h>
#include <gst/video/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SDL_DISABLE_IMMINTRIN_H 1
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// Global variable
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;

static GstFlowReturn callback(GstElement *sink,
                              void *user_data __attribute__((unused))) {
  // Retrieve buffer data
  GstSample *sample = NULL;
  g_signal_emit_by_name(sink, "pull-sample", &sample, NULL);
  if (sample == NULL) {
    return GST_FLOW_OK;
  }

  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo frame;

  if (gst_buffer_map(buffer, &frame, GST_MAP_READ)) {
    GstVideoInfo *video_info = gst_video_info_new();
    if (!gst_video_info_from_caps(video_info, gst_sample_get_caps(sample))) {
      g_warning("Failed to parse video info");
      return GST_FLOW_ERROR;
    }

    GstVideoFormat format = video_info->finfo->format;
    const int img_width = video_info->width;
    SDL_UpdateTexture(texture, NULL, frame.data, img_width * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    FILE *img = fopen("/tmp/test.csv", "w");
    for (int i = 0; i < frame.size; i += 4) {
      // Assuming BGRX format
      const uint8_t b = frame.data[i];
      const uint8_t g = frame.data[i + 1];
      const uint8_t r = frame.data[i + 2];
      const uint8_t v = 0.3 * r + 0.59 * g + 0.11 * b;
      fprintf(img, "%d ", v);
    }
    fflush(img);
    fclose(img);
    exit(0);

    // Clean up
    gst_buffer_unmap(buffer, &info);
    gst_video_info_free(video_info);
  }

  GstClockTime timestamp = GST_BUFFER_PTS(buffer);
  g_print("Timestamp=%" GST_TIME_FORMAT "\n", GST_TIME_ARGS(timestamp));

  // Clean up
  gst_sample_unref(sample);

  return GST_FLOW_OK;
}

int main(int argc, char *argv[]) {
  // GStreamer init
  gst_debug_set_default_threshold(GST_LEVEL_WARNING);
  gst_init(&argc, &argv);

  // SDL init
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init Error: %s/n", SDL_GetError());
  }

  // Get display size
  SDL_DisplayMode disp_mode;
  SDL_GetCurrentDisplayMode(0, &disp_mode);
  const int disp_w = disp_mode.w;
  const int disp_h = disp_mode.h;

  // Create window
  const int img_w = 744;
  const int img_h = 480;
  const int win_x = disp_w / 2 - img_w / 2;
  const int win_y = disp_h / 2 - img_h / 2;
  const int win_w = img_w;
  const int win_h = img_h;
  window = SDL_CreateWindow("TIS Camera",
                            win_x,
                            win_y,
                            win_w,
                            win_h,
                            SDL_WINDOW_SHOWN);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Clear render
  texture = SDL_CreateTexture(renderer,
                              SDL_PIXELFORMAT_BGRA32,
                              SDL_TEXTUREACCESS_STREAMING,
                              img_w,
                              img_h);

  // Setup pipeline
  const char *serial = "19220362";
  const char *pipeline_str =
      "tcambin name=source ! videoconvert ! appsink name=sink";
  GError *err = NULL;
  GstElement *pipeline = gst_parse_launch(pipeline_str, &err);
  if (pipeline == NULL) {
    printf("Could not create pipeline. Cause: %s\n", err->message);
    return 1;
  }

  // Setup pipeline source
  if (serial != NULL) {
    GstElement *source = gst_bin_get_by_name(GST_BIN(pipeline), "source");

    GValue val = {};
    g_value_init(&val, G_TYPE_STRING);
    g_value_set_static_string(&val, serial);

    g_object_set_property(G_OBJECT(source), "serial", &val);
    gst_object_unref(source);
  }

  // Setup pipeline sink
  GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  g_object_set(G_OBJECT(sink), "emit-signals", TRUE, NULL);
  g_signal_connect(sink, "new-sample", G_CALLBACK(callback), NULL);
  gst_object_unref(sink);

  // Start pipeline
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  g_print("Press 'enter' to stop the stream.\n");
  getchar();

  // Clean up
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}