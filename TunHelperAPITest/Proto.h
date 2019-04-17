#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

/**
* 网络协议结构
*/

/**
* IPv4 结构
*/
typedef struct {
#define IPH_GET_VER(v) (((v) >> 4) & 0x0F)
#define IPH_GET_LEN(v) (((v) & 0x0F) << 2)
	uint8_t version_len;

	uint8_t tos;
	uint16_t tot_len;
	uint16_t id;

#define IP_OFFMASK 0x1fff
	uint16_t frag_off;
	uint8_t ttl;

#define IP_PROTO_UDP  17  /* UDP protocol */
#define IP_PROTO_TCP   6  /* TCP protocol */
#define IP_PROTO_ICMP  1  /* ICMP protocol */
#define IP_PROTO_IGMP  2  /* IGMP protocol */
	uint8_t    protocol;

	uint16_t check_sum;
	uint32_t saddr;
	uint32_t daddr;
	/* The options start here. */
}  IPHDR;


/*
 * TCP header, per RFC 793.
 */
typedef struct {
  uint16_t      source;    /* source port */
  uint16_t      dest;      /* destination port */
  uint32_t      seq;       /* sequence number */
  uint32_t      ack_seq;   /* acknowledgement number */

# define TCPH_GET_DOFF(d) (((d) & 0xF0) >> 2)
  uint8_t       doff_res;

# define TCPH_FIN_MASK (1<<0)
# define TCPH_SYN_MASK (1<<1)
# define TCPH_RST_MASK (1<<2)
# define TCPH_PSH_MASK (1<<3)
# define TCPH_ACK_MASK (1<<4)
# define TCPH_URG_MASK (1<<5)
# define TCPH_ECE_MASK (1<<6)
# define TCPH_CWR_MASK (1<<7)
  uint8_t       flags;

  uint16_t      window;
  uint16_t      check_sum;
  uint16_t      urg_ptr;
} TCPHDR;

/*
 * UDP header
 */
typedef struct {
  uint16_t   source;
  uint16_t   dest;
  uint16_t   len;
  uint16_t   check;
} UDPHDR;

/*
 * 伪首部
*/
typedef struct {
	uint32_t saddr;
	uint32_t daddr;
	uint8_t zero;
	uint8_t protocol;
	uint16_t len;
}BOGUS;


/**
* ICMP 头结构
*/
typedef struct {
	IPHDR ip_hdr;
	uint8_t type;
	uint8_t code;
	uint16_t check_sum;
	/* data start here. */
}ICMPHDR;

#endif // PROTO_H
