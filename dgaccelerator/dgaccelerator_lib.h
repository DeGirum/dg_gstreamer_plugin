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

// Detected/Labelled object bbox structure (Detection Models)
struct DgAcceleratorObject
{
	float left;
	float top;
	float width;
	float height;
	char label[DG_MAX_LABEL_SIZE];
};

// Output data returned after processing
struct DgAcceleratorOutput
{
	int numObjects;
	DgAcceleratorObject object[MAX_OBJ_PER_FRAME]; // Allocates room for MAX_OBJ_PER_FRAME objects
};

// Initialize library
DgAcceleratorCtx *DgAcceleratorCtxInit(DgAcceleratorInitParams *init_params);

// Process output
DgAcceleratorOutput *DgAcceleratorProcess(DgAcceleratorCtx *ctx, unsigned char *data);

// Deinitialize our library context
void DgAcceleratorCtxDeinit(DgAcceleratorCtx *ctx);

#endif
