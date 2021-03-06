/*
 * WiFi_c.cpp
 *
 *  Created on: 12 ���. 2017 �.
 *      Author: ddemidov
 */

#include "WiFi_c.h"

#include <TCPSocketConnection.h>
#include <UDPSocket.h>
#if defined (USE_POE_SHIELD)
#include "W5100Interface.h"
W5100Interface eth;
#elif defined (USE_WIFI_SHIELD)
#include "WizFi250Interface.h"
#elif defined (USE_QUADRO_SHIELD)
#include "Quadro.h"
#endif



#if defined (USE_POE_SHIELD)
#define _udp_socket W5100_UDPSocket
#define _tcp_socket W5100_TCPSocketConnection
#define _socket W5100_Socket
#define _endpoint W5100_Endpoint
#elif defined (USE_WIFI_SHIELD)
#define _udp_socket UDPSocket
#define _tcp_socket TCPSocketConnection
#define _socket Socket
#define _endpoint Endpoint
#elif defined (USE_QUADRO_SHIELD)
extern Quadro eth;
#define _udp_socket QUADRO_UDPSocket
#define _tcp_socket QUADRO_TCPSocketConnection
#define _socket QUADRO_Socket
#define _endpoint QUADRO_Endpoint
#endif

char *eth_getMACAddress() {
    return eth.getMACAddress();
}

#ifdef __cplusplus
extern "C" {
#endif
#include <ctype.h>
#include <time/time.h>
#include <debug.h>
    
#define MAX_SOCKETS 8

typedef struct {
_socket *s;
int cid;
int type;
void *additional;
} sock_info_t;

static sock_info_t sockets_stack[MAX_SOCKETS];

class SocketStarter {
public:
  SocketStarter() {
    for ( int i = 0; i < MAX_SOCKETS; i++ )
      sockets_stack[i].s = NULL;
  }
};

static int find_free_sock(void) {
  for(int i=0; i<MAX_SOCKETS; i++) if ( !sockets_stack[i].s ) return i;
  return -1;
}

static SocketStarter _sock_start;

static int inet_ntop(uint32_t *src, char *dst) {
	char *tmp = (char *)src;
	sprintf(dst, "%u.%u.%u.%u", tmp[0], tmp[1], tmp[2], tmp[3]);
	return 0;
}

static int inet_pton(char *src, uint32_t *dst) {
	char *tmp = (char *)dst;
	int ret = sscanf(src, "%u.%u.%u.%u",
			(unsigned int*)(tmp+0),
			(unsigned int*)(tmp+1),
			(unsigned int*)(tmp+2),
			(unsigned int*)(tmp+3));
	if ( ret != 4 ) return -1;
	return 0;
}

int get_wifi_mac_address(char *mac) {
	char mac_str[20];
	int ret = 0;
#if defined(USE_POE_SHIELD)
	strncpy(mac_str, eth_getMACAddress(), sizeof(mac_str));
#elif defined(USE_WIFI_SHIELD)
	ret =  WizFi250::getInstance()->getMacAddress(mac_str);
#elif defined(USE_QUADRO_SHIELD)
	return eth.getmacaddress(mac);
#endif
	if ( ret < 0 ) return ret;

	uint32_t mac32[6];
#if GCC_VERSION <= 50201
	for (int i = 0; i < 6; i++) mac[i] = tolower(mac[i]);
	ret = sscanf(mac_str, "%x:%x:%x:%x:%x:%x",
#else
	ret = sscanf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
#endif
			(unsigned int *)mac32,
			(unsigned int *)mac32+1,
			(unsigned int *)mac32+2,
			(unsigned int *)mac32+3,
			(unsigned int *)mac32+4,
			(unsigned int *)mac32+5);
	if ( ret != 6 ) return -1;
	for (int i = 0; i < 6; i++) mac[i] = mac32[i]&0x00ff;
	return 0;
}

int wifi_gethostbyname(const char *addr, uint32_t *_ip) {
	char ip[16];
	memset(ip, 0, 16);
    int ret = -1;
#if defined(USE_POE_SHIELD)
	ret = eth.gethostbyname(addr, _ip);
//	DBG("get host [%d] %s - %08x", ret, addr, *_ip);
#elif defined(USE_WIFI_SHIELD)
	ret = WizFi250::getInstance()->getHostByName((char*)addr, (char *)ip);
	inet_pton(ip, _ip);
	DBG("dns ret %d", ret);
#elif defined(USE_QUADRO_SHIELD)
	ret = eth.gethostbyname(addr, _ip);
#endif
	return ret;
}

int wifi_socket(int protocol_family, int socket_type, int protocol) {
  if ( protocol_family != AF_INET )
    return -1;
  if ( protocol == IPPROTO_IP
       || protocol == IPPROTO_TCP
       || protocol == IPPROTO_UDP );
  else return -1;

  int sock = -1;
  sock = find_free_sock();
  if ( sock < 0 ) return sock;
  sockets_stack[sock].additional = NULL;
  switch(socket_type) {
    case SOCK_DGRAM: {
    	_udp_socket *udp = new _udp_socket();
    	sockets_stack[sock].s = udp;
    	sockets_stack[sock].type = socket_type;
    	udp->init();
    	udp->bind(rand()&0x7fff);
    } break;
    case SOCK_STREAM: {
      _tcp_socket *tcp = new _tcp_socket();
      sockets_stack[sock].s = tcp;
      sockets_stack[sock].type = socket_type;
    } break;
    default:
      return -1;
  }
  return sock;
}

int wifi_socket_close(int socket) {
  if ( socket < 0 || socket > MAX_SOCKETS ) return -1;
  if ( !sockets_stack[socket].s ) return -1;
  delete sockets_stack[socket].s;
  memset(&sockets_stack[socket], 0x00, sizeof(sock_info_t));
  sockets_stack[socket].cid = -1;
  return 0;
}

int wifi_socket_send(int socket, const void *buf, size_t len) {
  if ( socket < 0 || socket > MAX_SOCKETS ) return -1;
  if ( !sockets_stack[socket].s ) return -1;
  _tcp_socket *tcps = static_cast<_tcp_socket*>(sockets_stack[socket].s);
  return tcps->send((char *)buf, len);
}

int wifi_socket_recv(int socket, void *buf, size_t len) {
  if ( socket < 0 || socket > MAX_SOCKETS ) return -1;
  if ( !sockets_stack[socket].s ) return -1;
  _tcp_socket *tcps = static_cast<_tcp_socket*>(sockets_stack[socket].s);
  int ret = tcps->receive_all((char *)buf, len);
  return ret;
}

int wifi_socket_sendto(int socket, const void *buf, size_t len,
                       const struct sockaddr *dest_addr, socklen_t addrlen) {
  if ( socket < 0 || socket > MAX_SOCKETS ) return -1;
  if ( !sockets_stack[socket].s ) return -1;
  struct sockaddr_in *dest_addr_in = (struct sockaddr_in*)dest_addr;
  uint32_t haddr = htonl(dest_addr_in->sin_addr.s_addr);
  char ip[16];
  inet_ntop(&haddr, ip);
  _endpoint addr;
  int ret = addr.set_address(ip, htons(dest_addr_in->sin_port));
  _udp_socket *udp = static_cast<_udp_socket*>(sockets_stack[socket].s);
  ret = udp->sendTo(addr, (char *)buf, len);
//  DBG("send to %s:%d [%d]", addr.get_address(), addr.get_port(), ret);
  return ret;
}

int wifi_socket_recvfrom(int socket, void *buf, size_t len,
                         sockaddr *src_addr, socklen_t *addrlen) {
  if ( socket < 0 || socket > MAX_SOCKETS ) return -1;
  if ( !sockets_stack[socket].s ) return -1;
  DBG("recv from [%d] %d", socket, len);
  _endpoint addr;
  _udp_socket *udp = static_cast<_udp_socket*>(sockets_stack[socket].s);
  int ret = udp->receiveFrom(addr, (char *)buf, len);
  DBG("recv from [%d] %d", socket, ret);
  if ( ret < 0 ) return -1;
  struct sockaddr_in *saddr_in = (struct sockaddr_in*)src_addr;
  uint32_t _ip;
  inet_pton(addr.get_address(), &_ip);
  memcpy(&saddr_in->sin_addr.s_addr, &_ip, 4);
  saddr_in->sin_port = addr.get_port();
  return ret;
}

int wifi_socket_connect(int socket, const sockaddr *saddr, socklen_t addrlen) {
  if ( socket < 0 || socket > MAX_SOCKETS ) return -1;
  if ( !sockets_stack[socket].s ) return -1;
  struct sockaddr_in *dest_addr_in = (struct sockaddr_in*)saddr;
  uint32_t haddr = htonl(dest_addr_in->sin_addr.s_addr);
  _tcp_socket *tcps = static_cast<_tcp_socket*>(sockets_stack[socket].s);
  char ip[16];
  inet_ntop(&haddr, ip);
  return tcps->connect(ip, htons(dest_addr_in->sin_port));
}

int wifi_socket_setopt(int socket, int level, int optname,
                       const void *optval, socklen_t optlen) {
  if ( socket < 0 || socket > MAX_SOCKETS ) return -1;
  if ( level == SOL_SOCKET ) {
    switch(optname) {
      case SO_SNDTIMEO: // FIXME separate timeout for the send
      case SO_RCVTIMEO: {
        if ( optlen == sizeof(struct timeval) ) {
          if ( optval ) {
            struct timeval *tv = ((struct timeval *)(optval));
            int timeout = tv->tv_sec*1000 + (tv->tv_usec/1000);
            if ( timeout > 0 ) {
//            	DBG("to {%d}", timeout);
              sockets_stack[socket].s->set_blocking(false, timeout);
            }
          }
          return 0;
        }
        return -3;
      } break;
      default: return -2;
    }
    return 0;
  }

  return -1;
}

#if defined(USE_QUADRO_SHIELD)

int wifi_ssl_socket_connect(int sock) {
	if ( sock < 0 || sock > MAX_SOCKETS ) return -1;
	if ( !sockets_stack[sock].s ) return -1;
	_tcp_socket *tcps = static_cast<_tcp_socket*>(sockets_stack[sock].s);
	return tcps->ssl_connect();
}

int wifi_ssl_socket_read(int sock, char *data, int len) {
	if ( sock < 0 || sock > MAX_SOCKETS ) return -1;
	if ( !sockets_stack[sock].s ) return -1;
	_tcp_socket *tcps = static_cast<_tcp_socket*>(sockets_stack[sock].s);
	return tcps->ssl_read_all(data, len);
}

int wifi_ssl_socket_write(int sock, char *data, int len) {
	if ( sock < 0 || sock > MAX_SOCKETS ) return -1;
	if ( !sockets_stack[sock].s ) return -1;
	_tcp_socket *tcps = static_cast<_tcp_socket*>(sockets_stack[sock].s);
	return tcps->ssl_send(data, len);
}

int wifi_ssl_socket_close(int sock) {
	if ( sock < 0 || sock > MAX_SOCKETS ) return -1;
	if ( !sockets_stack[sock].s ) return -1;
	_tcp_socket *tcps = static_cast<_tcp_socket*>(sockets_stack[sock].s);
	return tcps->ssl_close();
}

#endif // USE_QUADRO_SHIELD

#ifdef __cplusplus
}
#endif
