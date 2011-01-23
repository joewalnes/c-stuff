#include <alloca.h>
#include <arpa/inet.h>
#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

int main(int argc, const char* argv[]) {
  
  // Command line args.
  if (argc != 4) {
    printf("usage: %s group port message\n", argv[0]);
    return -2;
  }
  const char* multicast_group = argv[1];
  int port = atoi(argv[2]);
  const char* message = argv[3];

  // Create UDP socket.
  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP); 
  if (fd < 0) {
    perror("could not open socket()");
    return -1;
  }
  
  // SO_REUSADDR: Allow multiple binds per host.
	int on = 1;
	if ((setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
    perror("could not setsockopt()");
    close(fd);
    return -1;
  }
  
  // Bind to multicast port.
  struct sockaddr_in multicast_addr;
  memset(&multicast_addr, 0, sizeof(multicast_addr));
  multicast_addr.sin_family      = AF_INET;
  multicast_addr.sin_addr.s_addr = htonl(INADDR_ANY); // TODO: Allow binding to specific address
  multicast_addr.sin_port        = htons(port);
  if ((bind(fd, (struct sockaddr *) &multicast_addr, sizeof(multicast_addr))) < 0) {
    perror("could not bind()");
    close(fd);
    return -1;
  }

  // Send an IGMP 'add membership' message to join multicast group.
	struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(multicast_group);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if ((setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*) &mreq, sizeof(mreq))) < 0) {
    perror("could not add membership");
    close(fd);
    return -1;
	}

  // Turn on loopback
  u_char loopback = 1;
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) == -1) {
    perror("failed to set IP_MULTICAST_LOOP socket option"); 
    close(fd);
    return -1;
  }
  
  // Set the multicast TTL.
  u_char ttl = 2;
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1) {
    perror("failed to set IP_MULTICAST_TTL socket option\n"); 
    close(fd);
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(multicast_group);
  addr.sin_port = htons(port);

  // Main send loop.
  for (;;) {
    int length = strlen(message);
    sendto(fd, message, length, 0, (struct sockaddr*)&addr, sizeof(addr));
    printf("Sent %d bytes to %s:%d: <%s>\n", length, multicast_group, port, message);
    fflush(stdout);
    sleep(1);
  }

}