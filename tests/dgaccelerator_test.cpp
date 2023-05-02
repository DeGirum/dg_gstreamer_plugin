//////////////////////////////////////////////////////////////////////
/// \file  tests/dgaccelerator_test.cpp
/// \brief Degirum Gstreamer plugin unit tests
///
/// Copyright 2023 DeGirum Corporation
///
/// This file contains implementation of unit tests 
/// for testing dgaccelerator plugin in DeepStream pipelines
///
#include <iostream>
#include <thread>
#include "gtest/gtest.h"
#include "../dgaccelerator/gstdgaccelerator.h"
#include "../dgaccelerator/dgaccelerator_lib.h"

// Define constants and data structures for the test cases
const gchar *PLUGIN_NAME = "nvdsgst_dgaccelerator";

class GStreamerPluginTest : public ::testing::Test {
protected:
  GStreamerPluginTest() {
    // gst_init(NULL, NULL);
  }

  ~GStreamerPluginTest() override {
    // gst_deinit();
  }
  // debug function, prints list of installed gstreamer plugins on system
  void printPlugins(){
	GList* plugin_list = gst_registry_get_plugin_list(gst_registry_get());
	std::cout << "Available plugin list:\n";
	for (GList* it = plugin_list; it != NULL; it = it->next) {
		std::cout << gst_plugin_get_name(GST_PLUGIN(it->data)) << std::endl;
	}
	gst_plugin_list_free(plugin_list); // Free the list
  }
};
// creates a simple pipeline for debug purposes fakesrc -> dgaccelerator -> fakesink
static GstElement *create_dgaccelerator_pipeline( const gchar *model_name, const gchar *server_ip, const gchar* cloud_token, int procW, int procH ) {
  GstElement *pipeline = gst_pipeline_new("test-pipeline");
  GstElement *source = gst_element_factory_make("fakesrc", "source");
  GstElement *dgaccelerator = gst_element_factory_make("dgaccelerator", "dgaccelerator");
  GstElement *sink = gst_element_factory_make("fakesink", "sink");

  gst_bin_add_many(GST_BIN(pipeline), source, dgaccelerator, sink, nullptr);
  gst_element_link_many(source, dgaccelerator, sink, nullptr);

  g_object_set(G_OBJECT(dgaccelerator), "model-name", model_name, "server-ip", server_ip, "cloud-token", cloud_token, "processing-width", procW, "processing-height", procH, nullptr);

  return pipeline;
}

// Test if the plugin is properly registered
TEST_F(GStreamerPluginTest, PluginRegistered) {
	GstPlugin *plugin = gst_plugin_load_by_name(PLUGIN_NAME);
	ASSERT_TRUE(plugin != NULL);
	gst_object_unref(plugin);
}

// Test running several pipelines with the element
TEST_F(GStreamerPluginTest, RunTestPipelines) {
	// gst_update_registry();
	// List of pipelines
	// 0 : gstreamer elements control check
	// 1 : nvidia + gstreamer control check
	// 2 : dgaccelerator on videotestsrc
	// 3 : dgaccelerator on mp4 video with box drawing
	const gchar *pipelines[] = {
		"fakesrc ! fakesink",
		"videotestsrc ! nvvideoconvert ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! identity ! fakesink enable-last-sample=0",
		"videotestsrc ! nvvideoconvert ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator processing-width=300 processing-height=300 server_ip=192.168.0.141 model-name=mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1 drop-frames=false ! fakesink enable-last-sample=0",
		"nvurisrcbin uri=file:///opt/nvidia/deepstream/deepstream-6.2/samples/streams/sample_1080p_h264.mp4 ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! dgaccelerator processing-width=300 processing-height=300 server_ip=192.168.0.141 model-name=mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1 drop-frames=false ! nvvideoconvert ! nvdsosd ! fakesink enable-last-sample=0",
		NULL
	};
	for (gint i = 0; pipelines[i] != NULL; i++) {
	GError *err = NULL;
	std::cout << "\n\tTesting pipeline " << i << ": " << pipelines[i] << std::endl;
	GstElement *pipeline = gst_parse_launch(pipelines[i], &err); 					// Parse and launch the GStreamer pipeline
	if (!pipeline){
		std::cerr << "Failed to launch pipeline " << err->message << std::endl; 	// Print the error message
		g_error_free(err); 															// Free the error variable
	}
	ASSERT_TRUE(pipeline != NULL); 													// Assert that the pipeline was launched correctly
	GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); 	// Start the pipeline
	EXPECT_EQ(GST_STATE_CHANGE_ASYNC, ret); 										// Expect that the pipeline state change is asynchronous
	g_usleep(2 * G_USEC_PER_SEC); 													// Wait for 2 seconds
	ret = gst_element_set_state(pipeline, GST_STATE_NULL); 							// Stop the pipeline
	EXPECT_EQ(GST_STATE_CHANGE_SUCCESS, ret); 										// Expect that the pipeline state change is successful
	gst_object_unref(pipeline); 													// Free the pipeline's resources
	}
}

// Test the plugin's robustness
TEST_F(GStreamerPluginTest, Robustness) {
	// gst_update_registry();
	GstElement* dgaccelerator = gst_element_factory_make("dgaccelerator", "test_dgaccelerator");
  	ASSERT_NE(dgaccelerator, nullptr);

	// Test handling of unexpected input format
	g_object_set(G_OBJECT(dgaccelerator), "processing-width", 512, "processing-height", 512, nullptr);
	GstCaps* invalid_caps = gst_caps_new_simple("video/x-raw",
												"format", G_TYPE_STRING, "SOME_INVALID_FORMAT",
												"width", G_TYPE_INT, 512,
												"height", G_TYPE_INT, 512,
												"framerate", GST_TYPE_FRACTION, 30, 1,
												nullptr);
	ASSERT_NE(invalid_caps, nullptr);
	GstPad* sinkpad = gst_element_get_static_pad(dgaccelerator, "sink");
	ASSERT_NE(sinkpad, nullptr);
	gboolean link_return = gst_pad_set_caps(sinkpad, invalid_caps);
	ASSERT_EQ(link_return, 0);
	gst_caps_unref(invalid_caps);
	gst_object_unref(sinkpad);

	// Test handling of incorrect GPU device ID
	g_object_set(G_OBJECT(dgaccelerator), "gpu-id", 4294967296, nullptr);
	guint gpu_id;
	g_object_get(G_OBJECT(dgaccelerator), "gpu-id", &gpu_id, nullptr);
	ASSERT_NE(gpu_id, 4294967296);

	// Test handling of incorrect processing width and height
	g_object_set(G_OBJECT(dgaccelerator), "processing-width", 0, "processing-height", 0, nullptr);
	gint processing_width, processing_height;
	g_object_get(G_OBJECT(dgaccelerator), "processing-width", &processing_width, "processing-height", &processing_height, nullptr);
	ASSERT_NE(processing_width, 0);
	ASSERT_NE(processing_height, 0);


	// Test properties that have to be validated during runtime:

	// Test handling of non-existing model name
	GstElement *pipeline1 = create_dgaccelerator_pipeline("non_existing_model", "192.168.0.141", "", 300, 300);
	EXPECT_THROW(
		{
			try {
				gst_element_set_state(pipeline1, GST_STATE_PLAYING);
			} catch (const std::runtime_error& e) {
				std::cerr << "Caught runtime error: " << e.what() << std::endl;
				throw;
			}
		}, std::runtime_error);
	gst_object_unref(pipeline1);

	// Test handling of incorrect server IP
	GstElement *pipeline2 = create_dgaccelerator_pipeline("mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1", "999.999.999.999", "", 300, 300);
	EXPECT_THROW(
		{
			try {
				gst_element_set_state(pipeline2, GST_STATE_PLAYING);
			} catch (const std::runtime_error& e) {
				std::cerr << "Caught runtime error: " << e.what() << std::endl;
				throw;
			}
		}, std::runtime_error);
	gst_object_unref(pipeline2);

	// Test handling of invalid cloud-token input (runtime validation)
	GstElement *pipeline3 = create_dgaccelerator_pipeline("degirum/public/mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1", "192.168.0.141", "fake_cloud_token", 300, 300);
	EXPECT_THROW(
		{
			try {
				gst_element_set_state(pipeline3, GST_STATE_PLAYING);
				std::this_thread::sleep_for(std::chrono::seconds(1)); // Add a one-second wait
			} catch (const std::runtime_error& e) {
				std::cerr << "Caught runtime error: " << e.what() << std::endl;
				throw;
			}
		}, std::runtime_error);
	gst_object_unref(pipeline3);

	// Test handling of model and processing-width / processing-height mismatch (runtime validation)
	GstElement *pipeline4 = create_dgaccelerator_pipeline("mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1", "192.168.0.141", "", 450, 300);
	EXPECT_THROW(
		{
			try {
				gst_element_set_state(pipeline4, GST_STATE_PLAYING);
				std::this_thread::sleep_for(std::chrono::seconds(1)); // Add a one-second wait
			} catch (const std::runtime_error& e) {
				std::cerr << "Caught runtime error: " << e.what() << std::endl;
				throw;
			}
		}, std::runtime_error);
	gst_object_unref(pipeline4);


	gst_object_unref(dgaccelerator);
}

int main(int argc, char **argv) {
	// Initialize GStreamer
	gst_init(&argc, &argv);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}