/**
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
 * Copyright (c) 2023 Stephan Sokolov < stephan@degirum.com >
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "dgfilternv_lib.h"
#include <stdio.h>
#include <stdlib.h>

/* Open CV headers */
#pragma GCC diagnostic push
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

#pragma GCC diagnostic pop
#include <memory>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "../include/Utilities/dg_file_utilities.h"
#include "../include/DglibInterface/dg_model_api.h"
#include "../include/client/dg_client.h"
#include "../include/json.hpp"

using json_ld = nlohmann::basic_json<std::map, std::vector, std::string, bool,
                                     std::int64_t, std::uint64_t, long double>;

struct DgFilternvCtx
{
    DgFilternvInitParams initParams;

    std::unique_ptr<DG::AIModel> model;
};

// Initialize our library
DgFilternvCtx *
DgFilternvCtxInit (DgFilternvInitParams * initParams)
{
    DgFilternvCtx *ctx = (DgFilternvCtx *) calloc (1, sizeof (DgFilternvCtx));
    ctx->initParams = *initParams;

    // TODO: change server ip to be a property? or modelparams
    const std::string server_ip = "100.122.112.76";
    std::string modelNameStr = ctx->initParams.model_name;

    std::cout << "\n\nv1.3\n\nINITIALIZING MODEL with IP ";
    std::cout << server_ip << " and name ";
    std::cout << modelNameStr << "\n";

    // TODO: validate model name?
    ctx->model.reset(new DG::AIModel( server_ip, modelNameStr, DG::ModelParamsWriter().InputImgFmt_set( "JPEG" )));

    std::cout << "\nMODEL SUCCESSFULLY INITIALIZED\n\n";

    return ctx;
}

// Main process function. Converts input to a cv::Mat and passes jpeg info to the model.
// Outputs objects in a DgFilternvOutput
DgFilternvOutput *
DgFilternvProcess (DgFilternvCtx * ctx, unsigned char *data)
{

    DgFilternvOutput *out = (DgFilternvOutput*)calloc (1, sizeof (DgFilternvOutput));

    json response;

    if (data != NULL) // Process the data here
    {
        // Data is a pointer to a cv::Mat.
        // Extract the mat: rows, cols
        cv::Mat frameMat(ctx->initParams.processingHeight, ctx->initParams.processingWidth, CV_8UC3, data );
        // (Usually is square) We can now pass it to the AI Model.
        
        // Decode this mat into a jpeg buffer vector.
        std::vector<int> param(2);
        param[0] = cv::IMWRITE_JPEG_QUALITY;
        param[1] = 90;
        std::vector<unsigned char> ubuff = {};
        // The function imencode compresses the image and stores it in the memory buffer that is resized to fit the result.
        cv::imencode(".jpeg", frameMat, ubuff, param);
        // Pass to the model.
        std::vector<char> buff (ubuff.begin(),ubuff.end());
        std::vector<std::vector<char>> frameVect = { buff };
        ctx->model->predict(frameVect, response);
        
        // Debug: can output an image of the frame
        // cv::imwrite("../../../../../../../../home/degirum/Documents/testoutput.jpeg", frameMat);
        frameMat.release();
    }
    // Parse the json output, fill output structure using processed output
    out->numObjects = 0; // not necessary due to calloc? 
    json_ld resp = response;
    if (strcmp(resp.type_name(),"array") == 0 && response.dump() != "[]"){
        // Iterate over all of the detected objects
        for(int i = 0; i < resp.size(); i++){
            out->numObjects++;
            json_ld newresp = resp[i]; // Output from model is a json array, so convert to single element
            std::vector<long double> bbox = newresp["bbox"].get<std::vector<long double>>();
            std::string label = newresp["label"];
            int category_id = newresp["category_id"].get<int>();
            long double score = newresp["score"].get<long double>();

            // std::cout << label << " " << category_id << " " << score;
            // std::cout << " " << bbox[0]<< " "<< bbox[1]<< " "<< bbox[2]<< " "<< bbox[3] << "\n";

            out->object[i] = (DgFilternvObject)
            {
                std::roundf(bbox[0]),           // left
                std::roundf(bbox[1]),           // top
                std::roundf(bbox[2] - bbox[0]), // width
                std::roundf(bbox[3] - bbox[1]), // height
                ""                              // label, must be of type char[]
            };
            snprintf(out->object[i].label, 64, label.c_str()); // Sets the label
        }
    }
    else if (strcmp(resp.type_name(),"object") == 0) { // Model gave a bad result
      std::cout << response.dump() << "\n\n";
    }
    // Objects not found... do nothing
    return out;
}

void
DgFilternvCtxDeinit (DgFilternvCtx * ctx)
{
    std::cout << "\nD E I N I T I A L I Z I N G \n\n\n";
    ctx->model.reset();
    free (ctx);
}
