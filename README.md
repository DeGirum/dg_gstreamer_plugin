# dg_gstreamer_plugin
This is the repository for the DeGirum Gstreamer Plugin.
Compatible with NVIDIA DeepStream pipelines, it is capable of running inference on upstream buffers and outputting NVIDIA metadata for use by downstream elements. Contains a wrapper element and a static library.

The element takes in a decoded video stream and adds metadata for the inference data from the model. Downstream elements in the pipeline can process this data using standard methods from NVIDIA DeepStream's element library, such as displaying results on screen or sending data to a server.

For more information on NVIDIA's DeepStream SDK and elements to be used in conjunction with this plugin:
- [DeepStream Plugin Guide]

**Example pipelines to show the plugin in action:**
1. Inference and visualization of 1 input video: 
```sh
gst-launch-1.0 filesrc location=<video-file-location> ! qtdemux ! h264parse ! nvv4l2decoder enable-max-performance=1 ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! dgfilternv server_ip=<server-ip> model_name=<model-name> ! nvvideoconvert ! nvdsosd ! nvegltransform ! nveglglessink
```

2. Inference and visualization of two models in parallel on one video:
```sh
gst-launch-1.0 filesrc location=<video-file-location> ! qtdemux ! queue ! h264parse ! nvv4l2decoder enable-max-performance=1 ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! tee name=t ! queue ! dgfilternv server_ip=<server-ip> model_name=<model-name> ! queue ! mix.sink_0 nvstreammux name=mix batch-size=1 width=1920 height=1080 ! nvvideoconvert ! nvdsosd process-mode=1 ! nvegltransform ! nveglglessink sync=false \
t. ! queue ! dgfilternv server_ip=<server-ip> model_name=<model-name> ! queue ! mix.sink_1
```
3. Inference and visualization of one model on two videos:
```sh
gst-launch-1.0 filesrc location=<video-file-1-location> ! qtdemux ! queue ! h264parse ! nvv4l2decoder enable-max-performance=1 ! queue ! m.sink_0 nvstreammux name=m batch-size=2 width=1920 height=1080 ! dgfilternv server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler width=1920 height=1080 buffer-pool-size=4 ! queue ! nvdsosd process-mode=1 ! nvegltransform ! nveglglessink \
filesrc location=<video-file-2-location> ! qtdemux ! queue ! h264parse ! nvv4l2decoder enable-max-performance=1 ! queue ! m.sink_1
```
This plugin requires a [DeepStream installation] and a [GStreamer installation].

| Plugin | Required Version |
| ------ | ------ |
| DeepStream | 6.2+ |
| GStreamer | 1.16.3+ |



[DeepStream Plugin Guide]:<https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_Intro.html>
[DeepStream installation]:<https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_Quickstart.html>
[GStreamer installation]:<https://gstreamer.freedesktop.org/documentation/installing/index.html>
