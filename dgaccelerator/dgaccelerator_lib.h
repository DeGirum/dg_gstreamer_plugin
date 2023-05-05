#ifndef __DGACCELERATOR_LIB__
#define __DGACCELERATOR_LIB__

#include <string>

constexpr int DG_MAX_LABEL_SIZE = 128;
constexpr int MAX_OBJ_PER_FRAME = 35;

class DgAcceleratorCtx;

// Init parameters structure as input, required for instantiating dgaccelerator_lib
struct DgAcceleratorInitParams
{
	// Width at which frame/object will be scaled
	int processingWidth;
	// height at which frame/object will be scaled
	int processingHeight;
	// model name
	char model_name[DG_MAX_LABEL_SIZE];
	// server ip
	char server_ip[DG_MAX_LABEL_SIZE];
	// Number of input streams
	int numInputStreams;
	// cloud token
	char cloud_token[DG_MAX_LABEL_SIZE];
	// drop frames toggle
	bool drop_frames;
};

// Detected/Labelled object bbox structure from Object Detection Model
struct DgAcceleratorObject
{
	float left;
	float top;
	float width;
	float height;
	char label[DG_MAX_LABEL_SIZE];
};

// Result from Pose Estimation Model
struct DgAcceleratorPose
{
	// A Landmark. Inference produces a set of these, together they define the pose
	struct Landmark
	{
		std::pair< double, double > point;	// Coordinate of landmark. [0] = x, [1] = y
		std::vector< int > connection;		// indices of landmarks this one connects to
		char label[ DG_MAX_LABEL_SIZE ];	// label of the landmark
		int landmark_class;					// index of the landmark
	};
	std::vector< Landmark > landmarks;		// vector of landmarks defining the pose
};

// Output data returned after processing
struct DgAcceleratorOutput
{
	// Detection models:
	int numObjects;
	DgAcceleratorObject object[MAX_OBJ_PER_FRAME]; // Allocates room for MAX_OBJ_PER_FRAME objects
	// Pose Estimation models:
	int numPoses;
	DgAcceleratorPose pose[MAX_OBJ_PER_FRAME];
};

// Initialize library
DgAcceleratorCtx *DgAcceleratorCtxInit(DgAcceleratorInitParams *init_params);

// Process output
DgAcceleratorOutput *DgAcceleratorProcess(DgAcceleratorCtx *ctx, unsigned char *data);

// Deinitialize our library context
void DgAcceleratorCtxDeinit(DgAcceleratorCtx *ctx);

#endif
