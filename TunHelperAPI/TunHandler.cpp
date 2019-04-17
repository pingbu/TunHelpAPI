// TunHelperAPI.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "TunHandler.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include "../Public/Log.h"

#ifdef _WIN32
	#include "Winsock2.h"
	#include <iphlpapi.h>
	#include <winioctl.h>
	#include "Win32Helper.h"
	#include "tap-windows.h"
#else
	#include <string.h>
	#include <unistd.h>
	#include <net/if.h>
	#include <linux/if_tun.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <arpa/inet.h> 
	#include <sys/select.h>
	#include <sys/time.h>
	#include <errno.h>
	#include <stdarg.h>
	#include <net/route.h>  

	#define TUN_NAME "SANSEC"

	int  GMTunfd
#endif

typedef struct _TUN_DEVICE_INFO
{
	void *fd;
    unsigned short uMTU;
}TunDeviceInfo, *PTunDeviceInfo;

TunDeviceInfo g_DeviceInfo = {};

#ifdef _WIN32
	OVERLAPPED m_ReadOverlapped;
	OVERLAPPED m_WriteOverlapped;
#else
	int  GMTunfd
#endif

//打开设备，设置为TUN模式，激活网卡
void* OpenTun(const char *ip, const char *mask, const char *dhcpservice)
{
#ifdef _WIN32
	if(g_DeviceInfo.fd)
	{
		return &g_DeviceInfo;
	}

	char AdapterGUID[MAX_PATH] = {};
	if(0 != GetAdapterGUID(ADAPTER_NAME, AdapterGUID, sizeof(AdapterGUID)))
	{
		return NULL;
	}

	char DeviceName[MAX_PATH] = {};
	sprintf_s(DeviceName, sizeof(DeviceName), USERMODEDEVICEDIR "%s" TAP_WIN_SUFFIX, AdapterGUID);

	//Log(LOG_ERROR, "debug: device name = %s\n", DeviceName);

	//打开设备
	g_DeviceInfo.fd = CreateFileA(DeviceName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM|FILE_FLAG_OVERLAPPED, 0);
	//g_DeviceInfo.fd = CreateFileA(DeviceName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
	if (g_DeviceInfo.fd == INVALID_HANDLE_VALUE)
	{
		//Log(LOG_ERROR, "error: CreateFile failed   %s  0x%08llx\n", DeviceName, (unsigned long long)g_DeviceInfo.fd);
		g_DeviceInfo.fd = NULL;
		return NULL;
	}

	DWORD len = 0;

	//ep保存tun网卡的IP地址和掩码
	ULONG ep[3] = {};
	ep[0] = inet_addr(ip);
	ep[2] = inet_addr(mask);
	ep[1] = ep[0] & ep[2];

	//设置为 Tun 模式
	if(!DeviceIoControl(g_DeviceInfo.fd, TAP_WIN_IOCTL_CONFIG_TUN, ep, sizeof(ep), ep, sizeof ep, &len, NULL))
	{
		//Log(LOG_ERROR, "SET TUN ERR IP = %s mask = %s\n", ip, mask);
		CloseHandle(g_DeviceInfo.fd);
		g_DeviceInfo.fd = NULL;
		return NULL;
	}
	else
	{
		//DoDebug("SET TUN success IP = %s mask = %s\n", ip, mask);
	}

	//通过DHCP设置虚拟网卡IP地址
	if(!ConfigDHCP(ip, mask, dhcpservice))
	{
		//Log(LOG_ERROR, "ConfigDHCP error\n");
		CloseHandle(g_DeviceInfo.fd);
		g_DeviceInfo.fd = NULL;
		return NULL;
	}

	//AddTunIpaddr(ip, mask);

	//设置网卡启动状态
	ULONG MediaStatus = TRUE;
	if(!DeviceIoControl(g_DeviceInfo.fd, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &MediaStatus, sizeof(MediaStatus), &MediaStatus, sizeof(MediaStatus), &len, NULL))
	{
		//网卡启动出错
		//Log(LOG_ERROR, "TUN STATUS ERR\n");
		CloseHandle(g_DeviceInfo.fd);
		g_DeviceInfo.fd = NULL;
		return NULL;
	}

	//Debug
	ULONG uMTU = TRUE;
    DeviceIoControl(g_DeviceInfo.fd, TAP_WIN_IOCTL_GET_MTU, &uMTU, sizeof(uMTU), &uMTU, sizeof(uMTU), &len, NULL);

	g_DeviceInfo.uMTU = (unsigned short)uMTU;

	printf("Tun: get mtu %d.\n", g_DeviceInfo.uMTU);
	return &g_DeviceInfo;
#else
	//linux open tun
	struct ifreq ifr;  
    int   err;

	if ((GMTunfd = open("/dev/net/tun", O_RDWR)) < 0)  
    {  
        //printf("Error :%m/n", errno);  
        //打开tun设备出错
		return NULL;
    }  
	
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	
	strncpy(ifr.ifr_name, TUN_NAME, IFNAMSIZ);
	
	if((err = ioctl(GMTunfd, TUNSETIFF, (void *)&ifr)) < 0)
	{
		close(GMTunfd);
		return NULL;
	}
	
	int s;
	
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		//printf("Error create socket :%m/n", errno);  
        close(GMTunfd);
		return NULL;
	}
	
	short flag;  
	flag = IFF_UP;
	
	if(ioctl(s, SIOCGIFFLAGS, &ifr) < 0)  
    {  
		close(GMTunfd);
		return NULL;
    }  
	
	ifr.ifr_ifru.ifru_flags |= flag;  
	
	if(ioctl(s, SIOCSIFFLAGS, &ifr) < 0)  
    {  
       //激活tun出错
		close(GMTunfd);
		return NULL;
    }  
	
	//添加IP
	AddTunIpaddr(ip, mask)；

	return (void *)(&GMTunfd);

#endif
}

int GetTunMTU(void *pHandle, unsigned short &uMTU)
{
	if(NULL == pHandle)
	{
		return FALSE;
	}
	
	TunDeviceInfo *pDeviceInfo = (TunDeviceInfo *)pHandle;

	uMTU = pDeviceInfo->uMTU;
	return TRUE;
}

void *GetTunFD(void *pHandle)
{
	if(NULL == pHandle)
	{
		return FALSE;
	}
	
	TunDeviceInfo *pDeviceInfo = (TunDeviceInfo *)pHandle;

	return pDeviceInfo->fd;
}

void CloseTun(void **pHandle)
{
	if(NULL == pHandle || NULL == *pHandle)
	{
		return;
	}

	TunDeviceInfo *pDeviceInfo = (TunDeviceInfo *)*pHandle;
	if(pDeviceInfo->fd)
	{
#ifdef _WIN32
		CloseHandle(pDeviceInfo->fd);
#else
		close(*((int*)pDeviceInfo->fd));
#endif
		pDeviceInfo->fd = NULL;
	}

	*pHandle = NULL;
}

//添加IP地址
int AddTunIpaddr(const char *ip, const char *mask)
{
#ifdef _WIN32
	ULONG NTEContext = 0;
	ULONG NTEInstance = 0;
	UINT IPAddress = inet_addr(ip);
	UINT IPMask = inet_addr(mask);

	int Index = 0;
	Index = MyGetAdapterIndex(ADAPTER_NAME);
	if(Index >= 0)
	{
		DWORD dwRet;
		dwRet = AddIPAddress(IPAddress, IPMask, Index, &NTEContext, &NTEInstance);   //windows help api 添加IP
		if(NO_ERROR == dwRet || ERROR_OBJECT_ALREADY_EXISTS == dwRet)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	return -1; //获取index无效
#else
	// linux add ip
	int s;
    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)  
    {
        return -1;
    }
	
	struct ifreq ifr;  
    strcpy(ifr.ifr_name, TUN_NAME);
	
	struct sockaddr_in addr;  
    bzero(&addr, sizeof(struct sockaddr_in));  
    addr.sin_family = PF_INET;  
    inet_aton(ip, &addr.sin_addr);  
  
    memcpy(&ifr.ifr_ifru.ifru_addr, &addr, sizeof(struct sockaddr_in));  
	if(ioctl(s, SIOCSIFADDR, &ifr) < 0)  
    {  
        //printf("Error set %s ip :%m/n",TUN_NAME, errno);  
        return -1;  
    } 
	
	inet_aton( mask, &addr.sin_addr);
	memcpy(&ifr.ifr_ifru.ifru_netmask, &addr, sizeof(struct sockaddr_in));
    if(ioctl( s, SIOCSIFNETMASK, &ifr) < 0)
	{  
        return -1;  
	}
    return 0;  
#endif
}

//添加路由
int AddTunRoute(const char *ip, const char *mask, const char *gateway)
{

#ifdef _WIN32
	DWORD dwStatus = 0;
	int Index = MyGetAdapterIndex(ADAPTER_NAME);
	if(Index == -1)
	{
		//DoDebug("AddTunRoute MyGetAdapterIndex failure\n");
		return -1;
	}

	DWORD Metric1 = GetMetric1(Index);

	//填充 MIB_IPFORWARDROW 信息
	MIB_IPFORWARDROW fr = {};
	fr.dwForwardDest = inet_addr(ip);
	fr.dwForwardMask = inet_addr(mask);
	fr.dwForwardNextHop = inet_addr(gateway);
	fr.dwForwardPolicy = 0;
	fr.dwForwardIfIndex = Index;  //dwForwardIfIndex值要 >= 网卡Index值
	fr.dwForwardType = 4;  /* the next hop is not the final dest */
	fr.dwForwardProto = 3; /* PROTO_IP_NETMGMT */
	fr.dwForwardAge = 0;
	fr.dwForwardNextHopAS = 0;
	fr.dwForwardMetric1 = Metric1;
	fr.dwForwardMetric2 = ~0;
	fr.dwForwardMetric3 = ~0;
	fr.dwForwardMetric4 = ~0;
	fr.dwForwardMetric5 = ~0;

	dwStatus = CreateIpForwardEntry(&fr);
	if (NO_ERROR == dwStatus || ERROR_OBJECT_ALREADY_EXISTS == dwStatus)
	{
		//DoDebug("AddTunRoute success\n");
		return 0;
	}
	else 
	{
		//如果GetMetric1()没有得到正确的Metric1值，采用遍历的方法获取Metric1
		//MyFormatMessage(dwStatus);
		for(Metric1 = 0; Metric1 < 9999; Metric1++)
		{
			fr.dwForwardMetric1 = Metric1;
			dwStatus = CreateIpForwardEntry(&fr);
			if (NO_ERROR == dwStatus || ERROR_OBJECT_ALREADY_EXISTS == dwStatus)
			{
				//DoDebug("AddTunRoute success  %d  %d\n", Index, Metric1);
				return 0;
			}
		}
	}
	//DoDebug("AddTunRoute failure\n");
	return -1;

#else
	// linux add route
	int skfd;  
    struct rtentry rt;  
  
    struct sockaddr_in dst;  
    struct sockaddr_in gw;  
    struct sockaddr_in genmask;
	
	memset(&rt, 0, sizeof(rt)); 
	
	bzero(&genmask,sizeof(struct sockaddr_in)); 
	inet_aton(mask, &genmask.sin_addr);  
	genmask.sin_family = PF_INET;
	
	bzero(&gw,sizeof(struct sockaddr_in));
	inet_aton(gateway, &gw.sin_addr); 
	gw.sin_family = PF_INET;	
	
	bzero(&dst,sizeof(struct sockaddr_in)); 
	inet_aton(destination, &dst.sin_addr);
	dst.sin_family = PF_INET;  
	
	rt.rt_metric = 0;  
    rt.rt_dst = *(struct sockaddr*) &dst;  
    rt.rt_genmask = *(struct sockaddr*) &genmask; 
	rt.rt_gateway = *(struct sockaddr*) &gw;	
    rt.rt_dev = TUN_NAME;  
	
	skfd = socket(PF_INET, SOCK_STREAM, 0);  
    if(ioctl(skfd, SIOCADDRT, &rt) < 0)   
    {  
        //printf("Error route add :%m /n", errno);  
        return -1;  
    }  
	
	return 0;
#endif
}

//删除路由
int DelTunRoute(const char *ip, const char *mask, const char *gateway)
{

#ifdef _WIN32
	DWORD status = 0;
	MIB_IPFORWARDROW fr = {};
	fr.dwForwardDest = inet_addr(ip);
	fr.dwForwardMask = inet_addr(mask);
	fr.dwForwardNextHop = inet_addr(gateway);
	fr.dwForwardIfIndex = MyGetAdapterIndex(ADAPTER_NAME);

	if((DWORD)-1 != fr.dwForwardIfIndex)
	{
		status = DeleteIpForwardEntry(&fr);
		if (status == NO_ERROR)
		{
			//DoDebug("DeleteIpForwardEntry success\n");
			return 0;
		}
		else
		{
			//MyFormatMessage(status);
			return -1;
		}
	}
	else 
	{
		//DoDebug("DelTunRoute MyGetAdapterIndex failure\n");
	}
	//DoDebug("DelTunRoute failure\n");
	return -1;

#else
	// linux del route
	//linux 关闭虚拟网卡 路由自动删除
#endif

}

//删除tun网卡的所有路由
#ifdef _WIN32
int DelTunRouteAll()
{
	DWORD status;
	DWORD dwActualSize = 0;
	MIB_IPFORWARDROW fr;
	PMIB_IPFORWARDTABLE pIpRouteTab = NULL;
	memset(&fr, 0, sizeof(fr));

	if(GetIpForwardTable(pIpRouteTab, &dwActualSize, TRUE) == ERROR_INSUFFICIENT_BUFFER)
	{
		pIpRouteTab = (PMIB_IPFORWARDTABLE)malloc(dwActualSize);

		if(GetIpForwardTable(pIpRouteTab, &dwActualSize, TRUE) == NO_ERROR)
		{
			DWORD i = 0;
			for (i = 0; i < pIpRouteTab->dwNumEntries; i++)
			{
				if(pIpRouteTab ->table[i].dwForwardIfIndex == MyGetAdapterIndex(ADAPTER_NAME))
				{
					fr = pIpRouteTab->table[i];

					status = DeleteIpForwardEntry(&fr);
					if (status == NO_ERROR)
					{
						//DoDebug("DelTunRouteAll success\n");
					}
					else
					{
						//MyFormatMessage(status);
					}
				}
			}
		}
		if(pIpRouteTab)
		{
			//free(pIpRouteTab);
		}
	}
	return 0;
}

BOOL ConfigDHCP(const char *ip, const char *mask, const char *DHCPServer)
{
	ULONG DhcpConfig[4];
	DWORD BytesReturned; 
	DhcpConfig[0] = inet_addr(ip);
	DhcpConfig[1] = inet_addr(mask);
	DhcpConfig[2] = inet_addr(DHCPServer);
	DhcpConfig[3] = 50000; // 租约，秒

	BOOL Success = DeviceIoControl (g_DeviceInfo.fd, TAP_WIN_IOCTL_CONFIG_DHCP_MASQ,
			DhcpConfig, sizeof DhcpConfig,
			DhcpConfig, sizeof DhcpConfig, &BytesReturned, NULL);

	/*int Index = 0;
	Index = MyGetAdapterIndex(ADAPTER_NAME);
	if(Index >= 0)
	{
		RenewAddress(Index);
	}
	*/
	return (BOOL)Success;
}

BOOL ConfigTun()
{
	ULONG ulOutBufLen = 0;
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO pAdapter = NULL;

	in_addr ip;
	in_addr mask;
	//in_addr gw;
	if (GetAdaptersInfo (NULL, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);

		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
		{
			pAdapter = pAdapterInfo;

			while(pAdapter)
			{
				if (strncmp(ADAPTER_NAME, pAdapter->Description, strlen(ADAPTER_NAME)) == 0)
				{
					ip.S_un.S_addr = inet_addr(pAdapter->IpAddressList.IpAddress.String);
					mask.S_un.S_addr = inet_addr(pAdapter->IpAddressList.IpMask.String);

					if(ip.S_un.S_addr == 0 || mask.S_un.S_addr == 0)
					{
						//Log(LOG_ERROR, "get_tun_ip error.\n");
						return FALSE;
					}	
				}
				pAdapter = pAdapter->Next;
			}
		}
	}
	if(pAdapterInfo)
	{
		free(pAdapterInfo);
		pAdapter = NULL;
	}

	//ep保存tun网卡的IP地址和掩码
	ULONG ep[3] = {};
	ep[0] = ip.S_un.S_addr;
	ep[2] = mask.S_un.S_addr;
	ep[1] = ep[0] & ep[2];

	//设置为 Tun 模式
	DWORD len = 0;
	if(!DeviceIoControl(g_DeviceInfo.fd, TAP_WIN_IOCTL_CONFIG_TUN, ep, sizeof(ep), ep, sizeof ep, &len, NULL))
	{
		//Log(LOG_ERROR, "SET TUN ERR IP = %s mask = %s\n", ip, mask);
		CloseHandle(g_DeviceInfo.fd);
		g_DeviceInfo.fd = NULL;
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void WinCreateEvent()
{
	memset(&m_ReadOverlapped, 0, sizeof m_ReadOverlapped);
	m_ReadOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	memset(&m_WriteOverlapped, 0, sizeof m_WriteOverlapped);
	m_WriteOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

}
#endif

int ReadTun(unsigned char *buffer, int size)
{
	if(NULL == buffer || 0 == size)
	{
		return FALSE;
	}

#ifdef _WIN32
	DWORD dwNumberOfBytesRead = 0;

	if(ReadFile(g_DeviceInfo.fd, buffer, size, &dwNumberOfBytesRead, &m_ReadOverlapped)
		|| GetLastError() == ERROR_IO_PENDING)
	/*if(ReadFile(g_DeviceInfo.fd, buffer, size, &dwNumberOfBytesRead, NULL)
		|| GetLastError() == ERROR_IO_PENDING)*/
	{
		WaitForSingleObject(m_ReadOverlapped.hEvent, INFINITE);
		GetOverlappedResult(g_DeviceInfo.fd, &m_ReadOverlapped, &dwNumberOfBytesRead, 0);
		return dwNumberOfBytesRead;
	}
	else
		return -1;
#else
	// linux 
	int nread;
	int fd;
	fd = *(int *)Handle;

	if((nread = read(fd, buffer, size)) < 0)
	{
		
	}
	return nread;
#endif
}

int WriteTun(unsigned char *buffer, int size)
{
	if(NULL == buffer || 0 == size)
	{
		return FALSE;
	}
#ifdef _WIN32
	DWORD dwNumberOfBytesWritten = 0;
	if (WriteFile(g_DeviceInfo.fd, buffer, size, &dwNumberOfBytesWritten, &m_WriteOverlapped)
		|| GetLastError() == ERROR_IO_PENDING)
	/*if (WriteFile(g_DeviceInfo.fd, buffer, size, &dwNumberOfBytesWritten, NULL)
		|| GetLastError() == ERROR_IO_PENDING)*/
	{
		WaitForSingleObject(m_WriteOverlapped.hEvent, INFINITE);
		GetOverlappedResult(g_DeviceInfo.fd, &m_WriteOverlapped, &dwNumberOfBytesWritten, 0);
		return dwNumberOfBytesWritten;
	}
	else
		return -1;
#else
	// linux 
	int nwrite;
	int fd;
	fd = *(int *)Handle;

	if((nwrite = write(fd, buffer, size)) < 0)
	{
	}	
	return nwrite;
#endif
}