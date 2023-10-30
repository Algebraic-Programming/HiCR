#include <acl/acl.h>
#include "runner.hpp"

int deviceId = 0;

bool RunOp();
void Destroy();
bool InitResource();


int main()
{
    if (!InitResource()) {
        ERROR_LOG("Init failed");
    } 
    INFO_LOG("Running op");
    if (!RunOp()) {
        Destroy();
        return 1;
    }
    INFO_LOG("Run op success");

    Destroy();
    return 0;
}

bool RunOp()
{
    Runner r = Runner();
    if (!r.Init()) {
        ERROR_LOG("error during runner init");
        return false;
    };
    INFO_LOG("runner init completed");

    r.PrepareInputs();
    INFO_LOG("runner input preparation completed");
    r.PrepareOutputs();
    INFO_LOG("runner output preparation completed");
    
    if (!r.Run()) {
        ERROR_LOG("error executing the kernel");
        return false;
    };
    r.PrintInputs();
    r.PrintOutput();
    // if (!r.Verify()) {
    //     ERROR_LOG("error verifying the result");
    //     return false;
    // }
    return true;
}

void Destroy()
{
    bool flag = false;
    if (aclrtResetDevice(deviceId) != ACL_SUCCESS) {
        ERROR_LOG("Reset device %d failed", deviceId);
        flag = true;
    }
    if (aclFinalize() != ACL_SUCCESS) {
        ERROR_LOG("Finalize acl failed");
        flag = true;
    }
    if (flag) {
        ERROR_LOG("Destory resource failed");
    } else {
        INFO_LOG("Destory resource success");
    }
}

bool InitResource() {
    if(aclInit(nullptr) != ACL_SUCCESS) {
        ERROR_LOG("init acl failed");
        return false;
    }

    if (aclrtSetDevice(deviceId) != ACL_SUCCESS) {
        ERROR_LOG("Set devide %d failed", deviceId);
        // do not care about error code
        (void)aclFinalize();
        return false;
    }

    INFO_LOG("set device %d success", deviceId);

    aclrtRunMode runMode;
    if(aclrtGetRunMode(&runMode) != ACL_SUCCESS) {
        ERROR_LOG("get run mode failed");
        Destroy();
        return false;

    }

    if (aclopSetModelDir("op_models") != ACL_SUCCESS) {
        ERROR_LOG("Load single op model failed");
        Destroy();
        return false;
    }

    return true;
}