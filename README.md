# dg_gstreamer_plugin
This is the repository for the DeGirum GStreamer Plugin.
Compatible with NVIDIA DeepStream pipelines, it is capable of running AI inference using DeGirum Orca&trade; AI hardware accelerator on upstream buffers 
and outputting NVIDIA metadata for use by downstream elements. 

The element takes in a decoded video stream, performs AI inference on each frame, and adds AI inference results as metadata. Downstream elements in the 
pipeline can process this metadata using standard methods from NVIDIA DeepStream's element library, such as displaying results on screen or sending data to a server.

For more information on NVIDIA's DeepStream SDK and elements to be used in conjunction with this plugin refer to [DeepStream Plugin Guide].

## Table of Contents
- [Example pipelines to show the plugin in action](#example-pipelines-to-show-the-plugin-in-action)
  - [Inference and visualization of 1 input video](#1-inference-and-visualization-of-1-input-video)
  - [Inference and visualization of two models on one video](#2-inference-and-visualization-of-two-models-on-one-video)
  - [Inference and visualization of one model on two videos](#3-inference-and-visualization-of-one-model-on-two-videos)
  - [Inference and visualization of one model on 4 videos with frame skipping only](#4-inference-and-visualization-of-one-model-on-4-videos-with-frame-skipping-only)
  - [Improved inference and visualization of 4 videos with capped framerates and frame skipping](#5-improved-inference-and-visualization-of-4-videos-with-capped-framerates-and-frame-skipping)
  - [Improved inference and visualization of 8 videos with capped framerates and frame skipping](#6-improved-inference-and-visualization-of-8-videos-with-capped-framerates-and-frame-skipping)
  - [Inference and visualization using a cloud model](#7-inference-and-visualization-using-a-cloud-model)
  - [Inference without visualization (model benchmark) example](#8-inference-without-visualization-model-benchmark-example)
  - [Inference and visualization with tracking](#9-inference-and-visualization-with-tracking)
  - [Inference and visualization of Pose Estimation](#10-inference-and-visualization-of-pose-estimation)
  - [Inference and visualization of Classification](#11-inference-and-visualization-of-classification)
  - [Inference and visualization of Segmentation](#12-inference-and-visualization-of-segmentation)
- [Plugin Properties](#plugin-properties)
- [Installation](#dependencies)



***

# Example pipelines to show the plugin in action


## 1. Inference and visualization of 1 input video
```sh
gst-launch-1.0 nvurisrcbin uri=file:///<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvvideoconvert ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0
```
![1videoInference](https://user-images.githubusercontent.com/126506976/230221405-5cc76511-2896-465c-a5db-f32923a9b074.png)


## 2. Inference and visualization of two models on one video
```sh
gst-launch-1.0 nvurisrcbin uri=file:///<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! dgaccelerator server_ip=<server-ip> model_name=<model-one-name> drop-frames=false box-color=pink ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-two-name> drop-frames=false box-color=red ! queue ! nvvideoconvert ! queue ! nvdsosd process-mode=1 ! nvegltransform ! nveglglessink enable-last-sample=0
```
![1video2models-color](https://user-images.githubusercontent.com/126506976/232162664-c9a50e45-afda-4c10-8a81-c4c0f93e0de5.png)
*Note the usage of the box-color property to distinguish between the two models.*

## 3. Inference and visualization of one model on two videos
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-1-location> ! m.sink_0 nvstreammux name=m live-source=0 buffer-pool-size=4 batch-size=2 batched-push-timeout=23976023 sync-inputs=true width=1920 height=1080 enable-padding=0 nvbuf-memory-type=0 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler rows=1 columns=2 width=1920 height=1080 nvbuf-memory-type=0 ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0 \
nvurisrcbin uri=file://<video-file-2-location> ! m.sink_1
```
![2videosInference](https://user-images.githubusercontent.com/126506976/230218942-28565ccc-d34a-478f-acb0-6493a5f5f2a5.png)

## 4. Inference and visualization of one model on 4 videos with frame skipping only
```sh
gst-launch-1.0 nvurisrcbin file-loop=true uri=file://<video-file-1-location> ! m.sink_0 nvstreammux name=m batch-size=4 batched-push-timeout=23976023 sync-inputs=true width=1920 height=1080 enable-padding=0 nvbuf-memory-type=0 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler rows=2 columns=2 width=1920 height=1080 nvbuf-memory-type=0 ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0 \
nvurisrcbin file-loop=true uri=file://<video-file-2-location> ! m.sink_1 \
nvurisrcbin file-loop=true uri=file://<video-file-3-location> ! m.sink_2 \
nvurisrcbin file-loop=true uri=file://<video-file-4-location> ! m.sink_3
```
This pipeline performs inference on 4 input streams at once and visualizes the results in a window. Because we haven't assumed anything about the framerates of the input streams as well as model processing speed, frame skipping is likely to occur to keep up with real-time visualization.
![4videosInference](https://user-images.githubusercontent.com/126506976/231277598-fb717798-188c-4244-a086-de643aeb0c1e.png)

## 5. Improved inference and visualization of 4 videos with capped framerates and frame skipping
```sh
gst-launch-1.0 nvurisrcbin file-loop=true uri=file://<video-file-1-location> ! videorate drop-only=true max-rate=<framerate> ! m.sink_0 nvstreammux name=m batch-size=4 batched-push-timeout=23976023 sync-inputs=true width=1920 height=1080 enable-padding=0 nvbuf-memory-type=0 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler rows=2 columns=2 width=1920 height=1080 nvbuf-memory-type=0 ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0 \
nvurisrcbin file-loop=true uri=file://<video-file-2-location> ! videorate drop-only=true max-rate=<framerate> ! m.sink_1 \
nvurisrcbin file-loop=true uri=file://<video-file-3-location> ! videorate drop-only=true max-rate=<framerate> ! m.sink_2 \
nvurisrcbin file-loop=true uri=file://<video-file-4-location> ! videorate drop-only=true max-rate=<framerate> ! m.sink_3
```
The difference between this pipeline and the previous one is the addition of 4 ```videorate``` elements for maximum reliability of the pipeline. Each of these elements is set to drop upstream frames to conform to a maximum framerate. This ensures that the framerate of inference is at most ```4 * <framerate>```, which is useful to ensure continuous operation of inference on the 4 streams. However, frame skipping is still enabled and will happen if needed to ensure a smooth pipeline.

## 6. Improved inference and visualization of 8 videos with capped framerates and frame skipping

Stemming directly from the previous example, it's possible to achieve smooth inference on 8 (or more) videos at once so long as their framerates are capped.

```sh
gst-launch-1.0 nvurisrcbin file-loop=true uri=file://<video-file-1-location> ! videorate drop-only=true max-rate=12 ! m.sink_0 \
nvstreammux name=m live-source=0 buffer-pool-size=4 batch-size=8 batched-push-timeout=23976023 sync-inputs=true width=1920 height=1080 enable-padding=0 nvbuf-memory-type=0 ! queue ! dgaccelerator server_ip=<server-ip> model_name=<model-name> ! nvmultistreamtiler rows=2 columns=4 width=1920 height=1080 nvbuf-memory-type=0 ! queue ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0 \
nvurisrcbin file-loop=true uri=file://<video-file-2-location> ! videorate drop-only=true max-rate=12 ! m.sink_1 \
nvurisrcbin file-loop=true uri=file://<video-file-3-location> ! videorate drop-only=true max-rate=12 ! m.sink_2 \
nvurisrcbin file-loop=true uri=file://<video-file-4-location> ! videorate drop-only=true max-rate=12 ! m.sink_3 \
nvurisrcbin file-loop=true uri=file://<video-file-5-location> ! videorate drop-only=true max-rate=12 ! m.sink_4 \
nvurisrcbin file-loop=true uri=file://<video-file-6-location> ! videorate drop-only=true max-rate=12 ! m.sink_5 \
nvurisrcbin file-loop=true uri=file://<video-file-7-location> ! videorate drop-only=true max-rate=12 ! m.sink_6 \
nvurisrcbin file-loop=true uri=file://<video-file-8-location> ! videorate drop-only=true max-rate=12 ! m.sink_7
```
![8videosInference](https://user-images.githubusercontent.com/126506976/231595499-87f37c00-6a57-453f-9181-b62d5e75b930.png)

*Note that this pipeline runs smoothly provided we know that ```<model-name>``` can handle at least 8 * 12 = 96 frames per second.*

## 7. Inference and visualization using a cloud model
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! dgaccelerator server_ip=<server-ip> model-name=<cloud-model-location> cloud-token=<token> ! nvvideoconvert ! nvdsosd ! nvegltransform ! nveglglessink
```
Here, we have added the additional parameter ```cloud-token=<token>```.

## 8. Inference without visualization (model benchmark) example
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator server_ip=<server-ip> model-name=<model-name> drop-frames=false ! fakesink enable-last-sample=0
```
Note the addition of ```drop-frames=false```, as we don't care about syncing to real-time. This pipeline is also useful for finding the maximum processing speed of a model.

## 9. Inference and visualization with tracking
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator server_ip=<server-ip> model-name=<model-name> drop-frames=false ! nvtracker ll-lib-file=/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so ! nvvideoconvert ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0
```
![1videoInferenceTracking](https://user-images.githubusercontent.com/126506976/234064084-11fdc31a-97dc-491b-a651-132823f5285e.png)

Here, this pipeline uses a default ```nvtracker``` configuration. It can be configured by modifying the properties ```ll-config-file``` and ```ll-lib-file``` in nvtracker.
***

## 10. Inference and visualization of Pose Estimation
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator processing-width=<width> processing-height=<height> server_ip=<server-ip> model-name=<model-name> drop-frames=false ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0
```
![1videoPoseEstimation](https://github.com/DeGirum/dg_gstreamer_plugin/assets/126506976/ebe9be93-9031-4691-8301-d9080a366a9e)

## 11. Inference and visualization of Classification
```sh
gst-launch-1.0 nvurisrcbin uri=file://<video-file-location> ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator processing-width=<width> processing-height=<height> server_ip=<server-ip> model-name=<model-name> drop-frames=false ! nvdsosd ! queue ! nvegltransform ! nveglglessink enable-last-sample=0
```
![1videoClassification](https://github.com/DeGirum/dg_gstreamer_plugin/assets/126506976/e51c5c1a-9863-4a4d-a038-2f39aec82450)

## 12. Inference and visualization of Segmentation
```sh
gst-launch-1.0 nvurisrcbin file-loop=true uri=file://<video-file-location> ! videorate drop-only=true max-rate=24 ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator processing-width=<width> processing-height=<height> server_ip=<server-ip> model-name=<model-name> drop-frames=false ! nvsegvisual width=1920 height=1080 ! queue ! nvegltransform ! nveglglessink enable-last-sample=0
```
![1videoSegmentation](https://github.com/DeGirum/dg_gstreamer_plugin/assets/126506976/7d5a447c-0d71-40dc-9a73-a30b67d8741f)

For Segmentation models, it is helpful to cap the framerate of the incoming video to match the model speed using the ```videorate``` element.

# Plugin Properties

The DgAccelerator element has several parameters than can be set to configure inference.

| Property Name | Default Value | Description |
|---------------|---------------|-------------|
| `box-color`   | `red`         | The color of the boxes in visualization pipelines. Choose from red, green, blue, cyan, pink, yellow, black. |
| `cloud-token` | `null`        | The [DeGirum Cloud API access token](https://cs.degirum.com) needed to allow connection to DeGirum cloud models. See example 7. |
| `drop-frames` | `true`        | If enabled, frames may be dropped in order to keep up with the real-time processing speed. This is useful when visualizing output in a pipeline. However, if your pipeline does not need visualization you can disable this feature. Similarly, if you know that the upstream framerate (i.e. the rate at which the video data is being captured) is slower than the maximum processing speed of your model, it might be better to disable frame dropping. This can help ensure that all of the available frames are processed and none are lost, which could be important for certain applications. See examples 4, 5, 6, and 8. |
| `gpu-id`      | `0`           | The index of the GPU for hardware acceleration, usually only changed when multiple GPU's are installed.  |
| `model-name`  | `yolo_v5s_coco--512x512_quant_n2x_orca_1` | The full name of the DeGirum AI model to be used for inference. |
| `processing-height` | `512` | The height of the accepted input stream for the model. |
| `processing-width`  | `512` | The width of the accepted input stream for the model. |
| `server-ip`   | `localhost` | The DeGirum AI server IP address or hostname to connect to for running AI inferences. |

These properties can be easily set within a `gst-launch-1.0` command, using the following syntax:
```sh
gst-launch-1.0 (...) ! dgaccelerator property1=value1 property2=value2 ! (...)
```

***

# Dependencies

This plugin requires a [DeepStream installation], a [GStreamer installation], and an [OpenCV installation].

| Package | Required Version |
| ------ | ------ |
| DeepStream | 6.2+ |
| GStreamer | 1.16.3+ |
| OpenCV | 4.2+ |


**Installation Steps:**

1. Clone the repository:
    
    ```git clone https://github.com/DeGirum/dg_gstreamer_plugin.git```

1. Enter the directory:
    
    ```cd dg_gstreamer_plugin```

1. Update the submodule:
   
   ```git submodule update --init```

> We have provided a shell script to install the above [dependencies](#dependencies). 
>
> **If you already have DeepStream and OpenCV installed, please skip to [building the plugin](#build-the-plugin).**

## Install dependencies with our script (optional):

### For Jetson systems:

1.  Download the Jetson tar file for DeepStream from [this link](https://developer.nvidia.com/downloads/deepstream-sdk-v620-jetson-tbz2)

2.  Run the script with the tar file's location as an argument: 

```./installDependencies.sh path/to/TarFile.tbz2 ```

### For non-Jetson systems (systems with a dGPU):

1.  Download the dGPU tar file for DeepStream from [this link](https://developer.nvidia.com/downloads/deepstream-sdk-v620-x86-64-tbz2)

2.  Download the NVIDIA driver version 525.85.12 from [this link](https://www.nvidia.com/Download/driverResults.aspx/198879/en-us/)

3.  Run the script with the tar file and driver file location as arguments: 

```./installDependencies.sh path/to/TarFile.tbz2 path/to/DriverFile.run```

> Alternatively, you can install DeepStream without the NVIDIA drivers if you already have them:
>
> ```./installDependencies.sh path/to/TarFile.tbz2```

## Build the plugin

To build the plugin, while within the directory of the cloned repository, execute the following commands:

```
mkdir build && cd build
cmake ..
cmake --build . 
sudo cmake --install .
```

Now, GStreamer pipelines have access to the element ```dgaccelerator``` for accelerating video processing tasks using NVIDIA DeepStream.

[DeepStream Plugin Guide]:<https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_plugin_Intro.html>
[DeepStream installation]:<https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_Quickstart.html>
[GStreamer installation]:<https://gstreamer.freedesktop.org/documentation/installing/index.html>
[OpenCV installation]:<https://github.com/DeGirum/dg_gstreamer_plugin/assets/126506976/acfb55c2-eb85-4552-9a6f-ef7af7352e0b>
