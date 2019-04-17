#pragma once

#ifdef _WIN32
	#define ADAPTER_NAME		"TAP-Windows Adapter V9"
#endif

#ifdef _WIN32
#ifdef __cplusplus
extern "C"{
#endif 
#endif

#ifdef _WIN32

	int DelTunRoute(const char *ip, const char *mask, const char *gateway);
	int DelTunRouteAll();
	int ConfigDHCP(const char *ip, const char *mask, const char *DHCPServer);
	int ConfigTun();
	void WinCreateEvent();
#endif

	void* OpenTun(const char *ip, const char *mask, const char *dhcpservice);	
	int ReadTun(unsigned char* buffer, int size);
	int WriteTun(unsigned char* buffer, int size);
	int GetTunMTU(void *pHandle, unsigned short &uMTU);
	void *GetTunFD(void *pHandle);
	void CloseTun(void **pHandle);
	int AddTunIpaddr(const char *ip, const char *mask);
	int AddTunRoute(const char *ip, const char *mask, const char *gateway);

#ifdef _WIN32
#ifdef __cplusplus
}
#endif
#endif
