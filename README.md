# dg_gstreamer_plugin
This is the repository for the DeGirum GStreamer Plugin.
Compatible with NVIDIA DeepStream pipelines, it is capable of running inference on upstream buffers and outputting NVIDIA metadata for use by downstream elements. Contains a wrapper element and a static library.

The element takes in a decoded video stream and adds metadata for the inference data from the model. Downstream elements in the pipeline can process this data using standard methods from NVIDIA DeepStream's element library, such as displaying results on screen or sending data to a server.

For more information on NVIDIA's DeepStream SDK and elements to be used in conjunction with this plugin:
- [DeepStream Plugin Guide]

**Example pipelines to show the plugin in action:**
1. Inference and visualization of 1 input video: 
```sh
gst-launch-1.0 nvurisrcbin uri=file:///<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvvideoconvert ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0
```
![1videoInference](https://user-images.githubusercontent.com/126506976/230221405-5cc76511-2896-465c-a5db-f32923a9b074.png)


2. Inference and visualization of two models on one video:
```sh
gst-launch-1.0 nvurisrcbin uri=file:///<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! dgaccelerator server_ip=<server-ip> model_name=<model-name> drop-frames=false ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> drop-frames=false ! queue ! nvvideoconvert ! queue ! nvdsosd process-mode=1 ! nvegltransform ! nveglglessink enable-last-sample=0
```
![1video2models](https://user-images.githubusercontent.com/126506976/230219445-c006a3c8-3adb-4864-aafe-f42be1f8b465.png)

3. Inference and visualization of one model on two videos:
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-1-location> ! m.sink_0 nvstreammux name=m live-source=0 buffer-pool-size=4 batch-size=2 batched-push-timeout=23976023 sync-inputs=true width=1920 height=1080 enable-padding=0 nvbuf-memory-type=0 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler rows=1 columns=2 width=1920 height=1080 nvbuf-memory-type=0 ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0 \
nvurisrcbin uri=file://<video-file-2-location> ! m.sink_1
```
![2videosInference](https://user-images.githubusercontent.com/126506976/230218942-28565ccc-d34a-478f-acb0-6493a5f5f2a5.png)

4. Inference and visualization of one model on 4 videos with frame skipping only:
```sh
gst-launch-1.0 nvurisrcbin file-loop=true uri=file://<video-file-1-location> ! m.sink_0 nvstreammux name=m batch-size=4 batched-push-timeout=23976023 sync-inputs=true width=1920 height=1080 enable-padding=0 nvbuf-memory-type=0 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler rows=2 columns=2 width=1920 height=1080 nvbuf-memory-type=0 ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0 \
nvurisrcbin file-loop=true uri=file://<video-file-2-location> ! m.sink_1 \
nvurisrcbin file-loop=true uri=file://<video-file-3-location> ! m.sink_2 \
nvurisrcbin file-loop=true uri=file://<video-file-4-location> ! m.sink_3
```
This pipeline performs inference on 4 input streams at once and visualizes the results in a window. Because we haven't assumed anything about the framerates of the input streams as well as model processing speed, frame skipping is likely to occur to keep up with real-time visualization.
![4videosInference](https://user-images.githubusercontent.com/126506976/231277598-fb717798-188c-4244-a086-de643aeb0c1e.png)

5. Improved inference and visualization of one model on 4 videos with capped framerates and frame skipping:
```sh
gst-launch-1.0 nvurisrcbin file-loop=true uri=file://<video-file-1-location> ! videorate drop-only=true max-rate=<framerate> ! m.sink_0 nvstreammux name=m batch-size=4 batched-push-timeout=23976023 sync-inputs=true width=1920 height=1080 enable-padding=0 nvbuf-memory-type=0 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler rows=2 columns=2 width=1920 height=1080 nvbuf-memory-type=0 ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0 \
nvurisrcbin file-loop=true uri=file://<video-file-2-location> ! videorate drop-only=true max-rate=<framerate> ! m.sink_1 \
nvurisrcbin file-loop=true uri=file://<video-file-3-location> ! videorate drop-only=true max-rate=<framerate> ! m.sink_2 \
nvurisrcbin file-loop=true uri=file://<video-file-4-location> ! videorate drop-only=true max-rate=<framerate> ! m.sink_3
```
The difference between this pipeline and the previous one is the addition of 4 ```videorate``` elements for maximum reliability of the pipeline. Each of these elements is set to drop upstream frames to conform to a maximum framerate. This ensures that the framerate of inference is at most ```4 * <framerate>```, which is useful to ensure continuous operation of inference on the 4 streams. However, frame skipping is still enabled and will happen if needed to ensure a smooth pipeline.

6. Inference and visualization using a cloud model:
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! dgaccelerator server_ip=<server-ip> model-name=<cloud-model-location> cloud-token=<token> ! nvvideoconvert ! nvdsosd ! nvegltransform ! nveglglessink
```
Here, we have added the additional parameter ```cloud-token=<token>```.

7. Inference without visualization (model benchmark) example:
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator server_ip=<server-ip> model-name=<model-name> drop-frames=false ! fakesink enable-last-sample=0
```
Note the addition of ```drop-frames=false```, as we don't care about syncing to real-time. This pipeline is also useful for finding the maximum processing speed of a model.


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
- drop-frames
> If enabled, frames may be dropped in order to keep up with the real-time processing speed. This is useful when visualizing output in a pipeline. However, if your pipeline doesn't need visualization you can disable this feature. Similarly, if you know that the upstream framerate (i.e. the rate at which the video data is being captured) is slower than the maximum processing speed of your model, it might be better to disable frame dropping. This can help ensure that all of the available frames are processed and none are lost, which could be important for certain applications. See examples 4, 5, and 7.
- cloud-token
> The token needed to allow connection to cloud models. See example 6.

These properties can be easily set within a `gst-launch-1.0` command, using the following syntax:
```sh
gst-launch-1.0 (...) ! dgaccelerator property1=value1 property2=value2 ! (...)
```

[DeepStream Plugin Guide]:<https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_Intro.html>
[DeepStream installation]:<https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_Quickstart.html>
[GStreamer installation]:<https://gstreamer.freedesktop.org/documentation/installing/index.html>
[OpenCV installation]:<https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html>
