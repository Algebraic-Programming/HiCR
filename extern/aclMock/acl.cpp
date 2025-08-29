#include "acl/acl.h"

ACL_FUNC_VISIBILITY aclError       aclInit(const char *configPath) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclFinalize() { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtSetCurrentContext(aclrtContext context) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtFree(void *devPtr) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtMallocHost(void **hostPtr, size_t size) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtFreeHost(void *hostPtr) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtMemcpy(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind, aclrtStream stream) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtCreateContext(aclrtContext *context, int32_t deviceId) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtGetCurrentContext(aclrtContext *context) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtDestroyContext(aclrtContext context) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclDestroyDataBuffer(const aclDataBuffer *dataBuffer) { return 0; }
ACL_FUNC_VISIBILITY aclDataBuffer *aclCreateDataBuffer(void *data, size_t size) { return nullptr; }
ACL_FUNC_VISIBILITY aclError       aclrtGetMemInfo(aclrtMemAttr attr, size_t *free, size_t *total) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtDeviceEnablePeerAccess(int32_t peerDeviceId, uint32_t flags) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtDeviceCanAccessPeer(int32_t *canAccessPeer, int32_t deviceId, int32_t peerDeviceId) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtGetDeviceCount(uint32_t *count) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtSetDevice(int32_t deviceId) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtCreateEvent(aclrtEvent *event) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtDestroyEvent(aclrtEvent event) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtRecordEvent(aclrtEvent event, aclrtStream stream) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtCreateStreamWithConfig(aclrtStream *stream, uint32_t priority, uint32_t flag) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtSynchronizeStream(aclrtStream stream) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtQueryEventStatus(aclrtEvent event, aclrtEventRecordedStatus *status) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclrtDestroyStream(aclrtStream stream) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclopSetModelDir(const char *modelDir) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclopLoad(const void *model, size_t modelSize) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclopExecuteV2(const char    *opType,
                                                  int            numInputs,
                                                  aclTensorDesc *inputDesc[],
                                                  aclDataBuffer *inputs[],
                                                  int            numOutputs,
                                                  aclTensorDesc *outputDesc[],
                                                  aclDataBuffer *outputs[],
                                                  aclopAttr     *attr,
                                                  aclrtStream    stream)
{
  return 0;
}
ACL_FUNC_VISIBILITY aclopAttr     *aclopCreateAttr() { return nullptr; }
ACL_FUNC_VISIBILITY void           aclopDestroyAttr(const aclopAttr *attr) {}
ACL_FUNC_VISIBILITY aclTensorDesc *aclCreateTensorDesc(aclDataType dataType, int numDims, const int64_t *dims, aclFormat format) { return nullptr; }
ACL_FUNC_VISIBILITY void           aclDestroyTensorDesc(const aclTensorDesc *desc) {}
ACL_FUNC_VISIBILITY float          aclFloat16ToFloat(aclFloat16 value) { return 0.0f; }
ACL_FUNC_VISIBILITY aclFloat16     aclFloatToFloat16(float value) { return 0; }
ACL_FUNC_VISIBILITY aclError       aclopSetAttrBool(aclopAttr *attr, const char *attrName, uint8_t attrValue) { return 0; }