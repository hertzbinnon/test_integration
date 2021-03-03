#include <cuda.h>
#include <nvEncodeAPI.h>
int main(int argc , char** argv){
cuInit(0);
CUdevice cuDevice = 0;
cuDeviceGet(&cuDevice, 0);
CUcontext cuContext = NULL;
cuCtxCreate(&cuContext, CU_CTX_SCHED_BLOCKING_SYNC, cuDevice);

NV_ENCODE_API_FUNCTION_LIST nvenc = { NV_ENCODE_API_FUNCTION_LIST_VER };
NVENCSTATUS nvStatus = NvEncodeAPICreateInstance(&nvenc);

NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encodeSessionExParams = {         
NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
encodeSessionExParams.device     = cuContext;
encodeSessionExParams.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
encodeSessionExParams.apiVersion = NVENCAPI_VERSION;

printf("OpenEncodeSesionEx #1\n");
void *hEncoder1 = NULL;
nvStatus = nvenc.nvEncOpenEncodeSessionEx(&encodeSessionExParams, &hEncoder1);

printf("OpenEncodeSesionEx #2\n");
void *hEncoder2 = NULL;
nvStatus = nvenc.nvEncOpenEncodeSessionEx(&encodeSessionExParams, &hEncoder2);

printf("OpenEncodeSesionEx #3\n");
void *hEncoder3 = NULL;
nvStatus = nvenc.nvEncOpenEncodeSessionEx(&encodeSessionExParams, &hEncoder3);
return 0;
}
