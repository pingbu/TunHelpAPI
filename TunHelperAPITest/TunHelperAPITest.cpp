// TunHelperAPITest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../TunHelperAPI/TunHandler.h"
#include "../TunHelperAPI/Win32Helper.h"
#include "Proto.h"
#include <iostream>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "TunHelperAPI.lib")
#pragma comment(lib,"WS2_32.LIB") 

#define MAX_PACKET_BUFFER 1024 * 8
#define TUN_ADDR	"10.251.251.34"
#define TUN_GW		"10.251.251.253"
#define TUN_MASK	"255.255.255.0"
#define TUN_DHCP	"10.251.251.33"

int _tmain(int argc, _TCHAR* argv[])
{
	void *pTunDevice;

	pTunDevice = OpenTun(TUN_ADDR, TUN_MASK, TUN_DHCP);

	DelTunRouteAll();

	AddTunRoute("192.168.65.0", "255.255.255.0", TUN_GW);

	unsigned char buffer[MAX_PACKET_BUFFER] = {0};

	WinCreateEvent();

	while(1)
	{
		ReadTun(buffer, MAX_PACKET_BUFFER);
		// 交换 IP 头源目的地址
		IPHDR * IpHdr = reinterpret_cast<IPHDR*>(buffer);

		// 输出源目的地址
		in_addr s;
		in_addr d;
		s.S_un.S_addr = IpHdr->saddr;
		d.S_un.S_addr = IpHdr->daddr;
		std::cout << inet_ntoa(s) << " -> ";
		std::cout << inet_ntoa(d) << std::endl;

		uint32_t Address = IpHdr->saddr;
		IpHdr->saddr = IpHdr->daddr;
		IpHdr->daddr = Address;
		
		// 修改 ICMP 为应答包
		ICMPHDR * IcmpHdr = reinterpret_cast<ICMPHDR*>(buffer);
		IcmpHdr->type = 0;
		IcmpHdr->check_sum += 8;

		WriteTun(buffer, MAX_PACKET_BUFFER);
	}
	return 0;
}

