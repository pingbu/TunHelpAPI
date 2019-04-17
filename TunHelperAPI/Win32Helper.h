#pragma once

#ifdef __cplusplus
extern "C"{
#endif 

	int GetAdapterGUID(const char *AdapterName, char *pBuf, int nLen);
	int MyGetAdapterIndex(const char *AdapterName);
	unsigned long GetMetric1(int Index);
	int ClearIP(const char* AdapterName);
	int CmdSetTun(const char* ip, const char* mask, const char* gate);
	bool RenewAddress(int Index);
#ifdef __cplusplus
}
#endif