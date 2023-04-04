# dg_gstreamer_plugin
This is the repository for the DeGirum GStreamer Plugin.
Compatible with NVIDIA DeepStream pipelines, it is capable of running inference on upstream buffers and outputting NVIDIA metadata for use by downstream elements. Contains a wrapper element and a static library.

The element takes in a decoded video stream and adds metadata for the inference data from the model. Downstream elements in the pipeline can process this data using standard methods from NVIDIA DeepStream's element library, such as displaying results on screen or sending data to a server.

For more information on NVIDIA's DeepStream SDK and elements to be used in conjunction with this plugin:
- [DeepStream Plugin Guide]

**Example pipelines to show the plugin in action:**
1. Inference and visualization of 1 input video: 
```sh
gst-launch-1.0 nvurisrcbin uri=file:///<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvvideoconvert ! nvdsosd ! nvegltransform ! nveglglessink
```

2. Inference and visualization of two models in parallel on one video:
```sh
gst-launch-1.0 nvurisrcbin uri=file:///<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! tee name=t ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! queue ! mix.sink_0 nvstreammux name=mix batch-size=1 width=1920 height=1080 ! nvvideoconvert ! nvdsosd process-mode=1 ! nvegltransform ! nveglglessink sync=false \
t. ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! queue ! mix.sink_1
```
3. Inference and visualization of one model on two videos:
```sh
gst-launch-1.0 nvurisrcbin uri=file:///<video-file-1-location> ! queue ! m.sink_0 nvstreammux name=m batch-size=2 width=1920 height=1080 ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler width=1920 height=1080 buffer-pool-size=4 ! queue ! nvdsosd process-mode=1 ! nvegltransform ! nveglglessink \
nvurisrcbin uri=file:///<video-file-2-location> ! queue ! m.sink_1
```
This plugin requires a [DeepStream installation], a [GStreamer installation], and an [OpenCV installation].

| Package | Required Version |
| ------ | ------ |
| DeepStream | 6.2+ |
| GStreamer | 1.16.3+ |
| OpenCV | 4+ |

**Plugin Properties**

The DgAccelerator element has several parameters than can be set to configure inference.
- server-ip
> The server ip address to connect to for running inference. Default value: 100.122.112.76
- model-name
> The full name of the model to be used for inference. Default value: yolo_v5s_coco--512x512_quant_n2x_orca_1
- processing-height
> The height of the accepted input stream for the model. Default value: 512
- processing-width
> The width of the accepted input stream for the model. Default value: 512
- gpu-id
> The index of the GPU for hardware acceleration, usually only changed when multiple GPU's are installed. Default value: 0

These properties can be easily set within a `gst-launch-1.0` command, using the following syntax:
```sh
gst-launch-1.0 (...) ! dgaccelerator property1=value1 property2=value2 ! (...)
```

[DeepStream Plugin Guide]:<https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_Intro.html>
[DeepStream installation]:<https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_Quickstart.html>
[GStreamer installation]:<https://gstreamer.freedesktop.org/documentation/installing/index.html>
[OpenCV installation]:<https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html>
