#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BLOCK_SIZE 1400

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <broadcast_addr>\n", argv[0]);
    return 0;
  }

  if(lseek(0, 0, SEEK_END) == -1) {
    perror("lseek end");
    return 1;
  }

  long file_size = lseek(0, 0, SEEK_CUR);
  if(file_size == -1) {
    perror("lseek cur");
    return 1;
  }

  if(lseek(0, 0, SEEK_SET) == -1) {
    perror("lseek set");
    return 1;
  }

  int udp_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_sock_fd == -1) {
    perror("socket");
    return 1;
  }

  int one = 1;
  if (setsockopt(udp_sock_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) ==
      -1) {
    perror("setsockopt reuseaddr");
    return 1;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7854);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(udp_sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 1;
  }

  if (setsockopt(udp_sock_fd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) ==
      -1) {
    perror("setsockopt broadcast");
    return 1;
  }

  addr.sin_port = htons(7855);
  addr.sin_addr.s_addr = inet_addr(argv[1]);

  time_t last_update = time(NULL);

  long current_file_offset = 0;
  for (;;) {
    if(time(NULL) != last_update) {
      last_update = time(NULL);
      fprintf(stderr, "%ld%%\n", 100 * current_file_offset / file_size);
    }

    long buf_to_recv;
    if(recv(udp_sock_fd, &buf_to_recv, sizeof(buf_to_recv), MSG_DONTWAIT) == -1) {
      if(errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("recv");
        return 1;
      }
    } else {
      if(buf_to_recv < current_file_offset) {
        if(lseek(0, buf_to_recv, SEEK_SET) == -1) {
          perror("lseek set");
          return 1;
        }
        current_file_offset = buf_to_recv;
      }
    }

    char buf_to_send[sizeof(long) + BLOCK_SIZE];
    *(long *)buf_to_send = current_file_offset;

    int n_read = read(0, buf_to_send + sizeof(long), BLOCK_SIZE);
    if (n_read == -1) {
      perror("read");
      return 1;
    }

    current_file_offset += n_read;
    if (sendto(udp_sock_fd, buf_to_send, sizeof(long) + n_read, 0, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      perror("send");
      return 1;
    }
  }

  return 0;
}
