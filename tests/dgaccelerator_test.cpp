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
#define TEST_SERVER_IP "192.168.0.141"
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

// Convenience function to return a vector list of properties for a given element
std::vector<GParamSpec*> get_element_properties(GstElement *element) {
        GObjectClass *gobject_class = G_OBJECT_GET_CLASS(element);
        GParamSpec **property_specs;
        guint n_properties;
        property_specs = g_object_class_list_properties(gobject_class, &n_properties);
        std::vector<GParamSpec*> properties(property_specs, property_specs + n_properties);
        g_free(property_specs);
        return properties;
}

// Convenience function to create a dummy pipeline with specified properties
// nvurisrcbin -> nvstreammux -> dgaccelerator -> fakesink
static GstElement *create_dgaccelerator_pipeline( const gchar *model_name, const gchar *server_ip, const gchar *cloud_token, int procW, int procH )
{
  GError *error = nullptr;
  gchar *pipeline_description;
  if( cloud_token != "" )
  {
	pipeline_description = g_strdup_printf(
		"nvurisrcbin uri=file:///opt/nvidia/deepstream/deepstream-6.2/samples/streams/sample_1080p_h264.mp4 ! m.sink_0 "
		"nvstreammux name=m batch-size=1 width=1920 height=1080 ! "
		"dgaccelerator name=dgaccelerator model-name=%s server-ip=%s cloud-token=%s processing-width=%d processing-height=%d ! "
		"fakesink",
		model_name,
		server_ip,
		cloud_token,
		procW,
		procH );
  }
  else
  {
	pipeline_description = g_strdup_printf(
		"nvurisrcbin uri=file:///opt/nvidia/deepstream/deepstream-6.2/samples/streams/sample_1080p_h264.mp4 ! m.sink_0 "
		"nvstreammux name=m batch-size=1 width=1920 height=1080 ! "
		"dgaccelerator name=dgaccelerator model-name=%s server-ip=%s processing-width=%d processing-height=%d ! "
		"fakesink",
		model_name,
		server_ip,
		procW,
		procH );
  }
  GstElement *pipeline = gst_parse_launch( pipeline_description, &error );
  if( error )
  {
	g_printerr( "Failed to create the pipeline: %s\n", error->message );
	g_error_free( error );
	return nullptr;
  }
  std::cout << "\n\tTesting pipeline: \ngst-launch-1.0 " << pipeline_description << "\n";
  g_free( pipeline_description );
  return pipeline;
}
// Convenience function to set non-default values
static void set_non_default_value(GParamSpec *prop, GValue *value) {
    const gchar *default_str;
    gchar *non_default_str;
    switch (G_VALUE_TYPE(value)) {
        case G_TYPE_BOOLEAN:
            g_value_set_boolean(value, !g_param_spec_get_default_value(prop));
            std::cout << "Set the property " << prop->name << " to " << g_value_get_boolean(value) << std::endl;
            break;
        case G_TYPE_INT:
            g_value_set_int(value, g_value_get_int(g_param_spec_get_default_value(prop)) + 1);
            std::cout << "Set the property " << prop->name << " to " << g_value_get_int(value) << std::endl;
            break;
        case G_TYPE_UINT:
            g_value_set_uint(value, g_value_get_uint(g_param_spec_get_default_value(prop)) + 1);
            std::cout << "Set the property " << prop->name << " to " << g_value_get_uint(value) << std::endl;
            break;
        case G_TYPE_LONG:
            g_value_set_long(value, g_value_get_long(g_param_spec_get_default_value(prop) + 1));
            std::cout << "Set the property " << prop->name << " to " << g_value_get_long(value) << std::endl;
            break;
        case G_TYPE_ULONG:
            g_value_set_ulong(value, g_value_get_ulong(g_param_spec_get_default_value(prop) + 1));
            std::cout << "Set the property " << prop->name << " to " << g_value_get_ulong(value) << std::endl;
            break;
        case G_TYPE_INT64:
            g_value_set_int64(value, g_value_get_int64(g_param_spec_get_default_value(prop) + 1));
            std::cout << "Set the property " << prop->name << " to " << g_value_get_int64(value) << std::endl;
            break;
        case G_TYPE_UINT64:
            g_value_set_uint64(value, g_value_get_uint64(g_param_spec_get_default_value(prop) + 1));
            std::cout << "Set the property " << prop->name << " to " << g_value_get_uint64(value) << std::endl;
            break;
        case G_TYPE_STRING:
            default_str = g_value_get_string(g_param_spec_get_default_value(prop));
            non_default_str = g_strdup_printf("%s_modified", default_str);
            g_value_set_string(value, non_default_str);
            std::cout << "Set the property " << prop->name << " to " << g_value_get_string(value) << std::endl;
            g_free(non_default_str);
            break;
        default:
            g_param_value_set_default(prop, value);
            std::cout << "Set the property " << prop->name << " to the default value" << std::endl;
            break;
    }
}



// Test if the plugin is properly registered
TEST_F(GStreamerPluginTest, PluginRegistered) {
	GstPlugin *plugin = gst_plugin_load_by_name(PLUGIN_NAME);
	ASSERT_TRUE(plugin != NULL);
	gst_object_unref(plugin);
}

// Test setting/getting all properties to some value.
TEST_F (GStreamerPluginTest, TestSettingGettingProperties) {
    GstElement* element = gst_element_factory_make("dgaccelerator", "test_dgaccelerator");
  	ASSERT_NE(element, nullptr);

    auto properties = get_element_properties(element);

    for (GParamSpec *prop : properties) {
		if (strcmp(prop->name, "parent") && strcmp(prop->name, "name")) // You can't set name or parent
		{
			// Set the property with some value
			GValue some_value = G_VALUE_INIT;
			g_value_init(&some_value, prop->value_type);
			set_non_default_value(prop, &some_value);
			g_object_set_property(G_OBJECT(element), prop->name, &some_value);

			// Get the property value
			GValue actual_value = G_VALUE_INIT;
			g_value_init(&actual_value, prop->value_type);
			g_object_get_property(G_OBJECT(element), prop->name, &actual_value);

			// Check if the retrieved value matches the expected value
			EXPECT_TRUE(g_param_values_cmp(prop, &some_value, &actual_value) == 0);

			g_value_unset(&some_value);
			g_value_unset(&actual_value);
		}
    }

    gst_object_unref(element);
}

// Test running several pipelines with the element
TEST_F(GStreamerPluginTest, RunTestPipelines) {
	// List of pipelines
	// 0 : gstreamer elements control check
	// 1 : nvidia + gstreamer control check
	// 2 : dgaccelerator on videotestsrc
	// 3 : dgaccelerator on mp4 video with box drawing
	const gchar *pipelines[] = {
		"fakesrc ! fakesink",
		"videotestsrc ! nvvideoconvert ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! identity ! fakesink enable-last-sample=0",
		"videotestsrc ! nvvideoconvert ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! queue ! dgaccelerator processing-width=300 processing-height=300 server_ip=" TEST_SERVER_IP " model-name=mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1 drop-frames=false ! fakesink enable-last-sample=0",
		"nvurisrcbin uri=file:///opt/nvidia/deepstream/deepstream-6.2/samples/streams/sample_1080p_h264.mp4 ! m.sink_0 nvstreammux name=m batch-size=1 width=1920 height=1080 ! dgaccelerator processing-width=300 processing-height=300 server_ip=" TEST_SERVER_IP " model-name=mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1 drop-frames=false ! nvvideoconvert ! nvdsosd ! fakesink enable-last-sample=0",
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
	g_usleep(1 * G_USEC_PER_SEC); 													// Wait for 1 seconds
	ret = gst_element_set_state(pipeline, GST_STATE_NULL); 							// Stop the pipeline
	EXPECT_EQ(GST_STATE_CHANGE_SUCCESS, ret); 										// Expect that the pipeline state change is successful
	gst_object_unref(pipeline); 													// Free the pipeline's resources
	}
}

// Test the plugin's robustness (invalid/unexpected property input)
TEST_F(GStreamerPluginTest, Robustness) {
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

	gst_object_unref(dgaccelerator);
}

TEST_F(GStreamerPluginTest, PropertyValidationPipelines){

	// Test handling of non-existing model name
	std::cout << "\n------=======Test handling of non-existing model name=======================================------\n";
	GstElement *pipeline1 = create_dgaccelerator_pipeline("non_existing_model", TEST_SERVER_IP, "", 300, 300);
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
	std::cout << "\n------=======Test handling of incorrect server IP===========================================------\n";
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

	// Test handling of model and processing-width / processing-height mismatch
	std::cout << "\n------=======Test handling of model and processing-width / processing-height mismatch=======------\n";
	GstElement *pipeline4 = create_dgaccelerator_pipeline("mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1", TEST_SERVER_IP, "", 450, 300);
	EXPECT_THROW(
		{
			try {
				gst_element_set_state(pipeline4, GST_STATE_PLAYING);
			} catch (const std::runtime_error& e) {
				std::cerr << "Caught runtime error: " << e.what() << std::endl;
				throw;
			}
		}, std::runtime_error);
	gst_object_unref(pipeline4);

	// Test handling of empty cloud-token input
	std::cout << "\n------=======Test handling of empty cloud-token input=====================================------\n";
	GstElement *pipeline5 = create_dgaccelerator_pipeline("degirum/public/mobilenet_v2_ssd_coco--300x300_quant_n2x_orca_1", TEST_SERVER_IP, "", 300, 300);
	EXPECT_THROW(
		{
			try {
				gst_element_set_state(pipeline5, GST_STATE_PLAYING);
			} catch (const std::runtime_error& e) {
				std::cerr << "Caught runtime error: " << e.what() << std::endl;
				throw;
			}
		}, std::runtime_error);
	gst_object_unref(pipeline5);

}

int main(int argc, char **argv) {
	// Initialize GStreamer
	gst_init(&argc, &argv);
	
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
