// vim: sw=2 ts=2 expandtab smartindent

/* basics */
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/* networking */
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>

/* non-blocking io */
#include <fcntl.h>
#include <poll.h>

#define DEBUG 1

#include "socket.h"
#include "request.h"
#include "server.h"


static volatile bool killed = false;
void interrupt_handler(int _) { (void)_; killed = true; }

int main() {

  /* I turned this off before adding support for poll,
   * but after seeing how running the server killed my laptop battery
   * and pegged out my server CPU, I kept this turned off, because
   * anytime we're writing to a pipe that isn't ready is a time we
   * probably weren't aggressive enough with poll */
  signal(SIGPIPE, SIG_IGN);

  /* I just like freeing all the memory at the end
   * so that valgrind says I'm a good boy 😇 */
  signal(SIGINT, interrupt_handler);

  Server server = {0};
  if (server_init(&server) < 0) return 1;

  while (!killed) {
    /**
     * This blocks indefinitely until there's something that needs doing.
     **/
    server_poll(&server);

    /* first see if any requests need responding to */
    for (Request *next = NULL, *c = server.last_request; c; c = next) {
      next = c->next;

      short revents = server_request_get_revents(&server, c);
      if (revents & (POLLHUP | POLLERR)) {
        server_drop_request(&server, c);
      } else {
        server_step_request(&server, c);
      }
    }

#if DEBUG
    printf("\nREQUEST COUNT: %zu\n", server_request_count(&server));
    for (Request *c = server.last_request; c; c = c->next) {
      // printf("request! id: %zu phase: ", c->id);

      switch (c->phase) {
        case RequestPhase_Empty         : printf("RequestPhase_Empty         \n"); continue;
        case RequestPhase_HttpResponding: printf("RequestPhase_HttpResponding\n"); continue;
        case RequestPhase_HttpRequesting:
          printf(
            "RequestPhase_HttpRequesting"
              "(bytes_read: %zu, buf_len: %zu)\n",
            c->http_req.bytes_read,
            c->http_req.buf_len
          );
          continue;
        default: printf("Unknown phase!\n"); continue;
      }
    }
#endif

    /* now poll for new requests */
    if (server_new_request_revent(&server))
      for (;;) {
        int fd = socket_accept_request(server.host_fd);
        if (fd < 0) {
          if (errno == EWOULDBLOCK || errno == EAGAIN)
            break;
          else
            continue;
        }

        server_add_request(&server, fd);
      }

  }

  server_free(&server);
}

#define socket_IMPLEMENTATION
#include "socket.h"
#define server_IMPLEMENTATION
#include "server.h"
#define request_IMPLEMENTATION
#include "request.h"
