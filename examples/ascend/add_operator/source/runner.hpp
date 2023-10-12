#include <acl/acl.h>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cmath>

#define INFO_LOG(fmt, args...) fprintf(stdout, "[INFO]  " fmt "\n", ##args)
#define WARN_LOG(fmt, args...) fprintf(stdout, "[WARN]  " fmt "\n", ##args)
#define ERROR_LOG(fmt, args...) fprintf(stderr, "[ERROR]  " fmt "\n", ##args)


class Runner 
{
private:
    aclFloat16 *hostInputVector1;
    aclFloat16 *hostInputVector2;
    aclFloat16 *hostOutputVector;

    size_t sizeInput1;
    size_t sizeInput2;
    size_t sizeOutput;

    aclFloat16 *devInputVector1;
    aclFloat16 *devInputVector2;
    aclFloat16 *devOutputVector;

    aclDataType inputType = ACL_FLOAT16;
    aclDataType outputType = ACL_FLOAT16;

bool copyDataToAscend()
{
    aclError return_code = ACL_SUCCESS;
    return_code = aclrtMemcpy(devInputVector1, sizeInput1, hostInputVector1, sizeInput1, ACL_MEMCPY_HOST_TO_DEVICE);
    if(return_code != ACL_SUCCESS) {
        ERROR_LOG("failed to copy matrix 1 data on ascend error code: %d", return_code);
        return false;
    }

    return_code = aclrtMemcpy(devInputVector2, sizeInput2, hostInputVector2, sizeInput2, ACL_MEMCPY_HOST_TO_DEVICE);
    if(return_code != ACL_SUCCESS) {
        ERROR_LOG("failed to copy matrix 2 data on ascend error code: %d", return_code);
        return false;
    }

    INFO_LOG("data copy on ascend succeed");
    return true;
}

bool copyResultFromAscend()
{
    aclError return_code = ACL_SUCCESS;
    return_code = aclrtMemcpy(hostOutputVector, sizeOutput, devOutputVector, sizeOutput, ACL_MEMCPY_DEVICE_TO_HOST);
    if(return_code != ACL_SUCCESS) {
        ERROR_LOG("failed to copy result back from the ascend code: %d", return_code);
        return false;
    }
    return true;
}

void doPrintMatrix(const aclFloat16 *matrix, uint32_t numRows, uint32_t numCols)
{
    uint32_t rows = numRows;

    for (uint32_t i = 0; i < rows; ++i) {
        for (uint32_t j = 0; j < numCols; ++j) {
            std::cout << std::setw(10) << aclFloat16ToFloat(matrix[i * numCols + j]);
        }
        std::cout << std::endl;
    }

    if (rows < numRows) {
        std::cout << std::setw(10) << "......" << std::endl;
    }
}
public:

Runner() {
    sizeInput1 = 192 * aclDataTypeSize(inputType);
    sizeInput2 = 192 * aclDataTypeSize(inputType);
    sizeOutput = 192 * aclDataTypeSize(outputType);
}

virtual ~Runner(){
    if (devInputVector1 != nullptr) {
        (void) aclrtFree(devInputVector1);
    }
    if (devInputVector2 != nullptr) {
        (void) aclrtFree(devInputVector2);
    }
    if (devOutputVector != nullptr) {
        (void) aclrtFree(devOutputVector);
    }

    INFO_LOG("device buffers deallocated");

    if (hostInputVector1 != nullptr) {
        (void) aclrtFreeHost(hostInputVector1);
    }
    if (hostInputVector2 != nullptr) {
        (void) aclrtFreeHost(hostInputVector2);
    }
    if (hostOutputVector != nullptr) {
        (void) aclrtFreeHost(hostOutputVector);
    }

    INFO_LOG("host buffers deallocated");
}

bool Init(){
    if (aclrtMalloc((void **) &devInputVector1, sizeInput1, ACL_MEM_MALLOC_HUGE_FIRST) != ACL_SUCCESS) {
        ERROR_LOG("malloc for input 1 failed");
        return false;
    }
    if (aclrtMalloc((void **) &devInputVector2, sizeInput2, ACL_MEM_MALLOC_HUGE_FIRST) != ACL_SUCCESS) {
        ERROR_LOG("malloc for input 2 failed");
        return false;
    }
    if (aclrtMalloc((void **) &devOutputVector, sizeOutput, ACL_MEM_MALLOC_HUGE_FIRST) != ACL_SUCCESS) {
        ERROR_LOG("malloc for output failed");
        return false;
    }

    if (aclrtMallocHost((void **) &hostInputVector1, sizeInput1) != ACL_SUCCESS) {
        ERROR_LOG("malloc host memory for host input 1 failed");
        return false;
    }

    if (aclrtMallocHost((void **) &hostInputVector2, sizeInput2) != ACL_SUCCESS) {
        ERROR_LOG("malloc host memory for host input 1 failed");
        return false;
    }

    if (aclrtMallocHost((void **) &hostOutputVector, sizeOutput) != ACL_SUCCESS) {
        ERROR_LOG("malloc host memory for host output 1 failed");
        return false;
    }
    return true;    
}

void PrepareInputs(){
    for (int i = 0; (size_t)i < sizeInput1; i++) {
        hostInputVector1[i] = aclFloatToFloat16(2.0);
        hostInputVector2[i] = aclFloatToFloat16(4.0);
    }
}

void PrepareOutputs(){
    for (int i = 0; (size_t)i < sizeOutput; i++) {
        hostOutputVector[i] = aclFloatToFloat16(0.0);
    }
}

void PrintOutput(){
    INFO_LOG("Print output matrix ");
    doPrintMatrix(hostOutputVector, 1, 1);
    
}

void PrintInputs(){
    INFO_LOG("Print input matrix 1");
    doPrintMatrix(hostInputVector1, 1, 1);
    INFO_LOG("Print input matrix 2");
    doPrintMatrix(hostInputVector2, 1, 1);
}

bool Run()
{
    aclrtStream stream = nullptr;
    if (aclrtCreateStream(&stream) != ACL_SUCCESS) {
        ERROR_LOG("create stream failed");
        return false;
    }
    INFO_LOG("stream created");


    if(!copyDataToAscend()) {
        ERROR_LOG("error copying data to ascend");
        return false;
    }

    aclDataBuffer *buff1 = aclCreateDataBuffer(devInputVector1, sizeInput1);
    aclDataBuffer *buff2 = aclCreateDataBuffer(devInputVector2, sizeInput2);
    aclDataBuffer *buff3 = aclCreateDataBuffer(devOutputVector, sizeOutput);

    aclDataBuffer *inputs[] = {buff1, buff2};
    aclDataBuffer *outputs[] = {buff3};

    INFO_LOG("data buffers created");

    int64_t dims[] = {(int64_t)192, (int64_t)1};
    aclTensorDesc *inputDesc1 = aclCreateTensorDesc(ACL_FLOAT16, 2, dims, ACL_FORMAT_ND);
    aclTensorDesc *inputDesc2 = aclCreateTensorDesc(ACL_FLOAT16, 2, dims, ACL_FORMAT_ND);
    aclTensorDesc *outputDesc = aclCreateTensorDesc(ACL_FLOAT16, 2, dims, ACL_FORMAT_ND);
    aclTensorDesc *inputDescs[] = {inputDesc1, inputDesc2};
    aclTensorDesc *outputDescs[] = {outputDesc};
    aclopAttr *attrs = aclopCreateAttr();
    // inputDesc[0] = aclCreateTensorDesc(ACL_FLOAT16, 1, [192], ACL_FORMAT_ND);
    aclError return_code = ACL_SUCCESS;
    return_code = aclopExecuteV2("Add", 2, inputDescs, inputs, 1, outputDescs, outputs, attrs , stream);

    if (return_code != ACL_SUCCESS) {
        ERROR_LOG("error in executing kernel %d", return_code);
        (void)aclDestroyDataBuffer(buff1);
        (void)aclDestroyDataBuffer(buff2);
        (void)aclDestroyDataBuffer(buff3);
        (void)aclrtDestroyStream(stream);
        return false;
    }
    return_code = aclrtSynchronizeStream(stream);

    if (return_code != ACL_SUCCESS) {
        ERROR_LOG("error in synchronizing stream %d", return_code);
        (void)aclDestroyDataBuffer(buff1);
        (void)aclDestroyDataBuffer(buff2);
        (void)aclDestroyDataBuffer(buff3);
        (void)aclrtDestroyStream(stream);
        return false;
    } 
    
    (void)copyResultFromAscend();
    INFO_LOG("results copied from ascend");
    (void)aclDestroyDataBuffer(buff1);
    (void)aclDestroyDataBuffer(buff2);
    (void)aclDestroyDataBuffer(buff3);
    INFO_LOG("data buffers destroyed");
    (void)aclrtDestroyStream(stream);
    INFO_LOG("stream destroyed");
    return true;
}

// bool Verify()
// {

//     aclFloat16 data1;
//     aclFloat16 data2;
//     aclFloat16 data3;

//     for (int i = 0; (size_t)i < sizeInput1; i++) {
//         data1 = hostInputVector1[0];
//         data2 = hostInputVector2[0];
//         data3 = hostOutputVector[0];
//         if (std::abs(data1+data2-data3) >= 1.0e-05){
            
//             return false;
//         }
//     }
//     return true;
// }

};