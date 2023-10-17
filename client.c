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
  addr.sin_port = htons(7855);
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

  addr.sin_port = htons(7854);
  addr.sin_addr.s_addr = inet_addr(argv[1]);
  // if (connect(udp_sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
  //   perror("connect");
  //   return 1;
  // }

  long current_file_offset = 0;
  for (;;) {
    char buf_to_recv[sizeof(long) + BLOCK_SIZE];
    int n_recv = recv(udp_sock_fd, buf_to_recv, sizeof(buf_to_recv), 0);
    if (n_recv == -1) {
      perror("recv");
      return 1;
    }

    int block_size = n_recv - sizeof(long);

    long buf_start = *(long *)buf_to_recv;
    long buf_end = buf_start + block_size;
    if (buf_end <= current_file_offset) {
      if (block_size == 0) {
        break;
      }
      continue;
    }

    if (buf_start <= current_file_offset) {
      char *data_start =
          buf_to_recv + sizeof(long) + current_file_offset - buf_start;
      int count = buf_end - current_file_offset;
      while (count > 0) {
        int n_written = write(1, data_start, count);
        if (n_written == -1) {
          perror("write");
          return 1;
        }
        data_start += n_written;
        count -= n_written;
      }
      current_file_offset = buf_end;
    } else {
      long buf_to_send = current_file_offset;
      if (sendto(udp_sock_fd, &buf_to_send, sizeof(buf_to_send), 0,
                 (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("send");
        return 1;
      }
    }
  }

  return 0;
}
