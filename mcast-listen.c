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
  if (argc != 3) {
    printf("usage: %s group port\n", argv[0]);
    return -2;
  }
  const char* multicast_group = argv[1];
  int port = atoi(argv[2]);

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

  // Buffer to receive messages in.
  char buffer[BUFFER_SIZE];
  memset(&buffer, 0, BUFFER_SIZE);

  // Main receive loop.
  printf("Listening on multicast group %s:%d...\n", multicast_group, port);
  fflush(stdout);
  int i;
  for (;;) {
    int length = (int)recv(fd, buffer, BUFFER_SIZE - 1, MSG_WAITALL);
    if (length < 0) {
      perror("could not recv()");
      close(fd);
      return -1;
    }
    buffer[length] = '\0';
    printf("Received %d bytes: <%s>\n", length, buffer);
    fflush(stdout);
  }

}