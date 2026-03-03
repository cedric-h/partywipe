// vim: sw=2 ts=2 expandtab smartindent

#ifndef server_IMPLEMENTATION

typedef struct {

  int host_fd;
  /* The head of the linked list of requests */
  Request *last_request;

  struct pollfd *pollfds;
  nfds_t pollfd_count;
} Server;

static int server_init(Server *server);
static void server_free(Server *server);

/**
 * wait indefinitely until there is work to be done!
 **/
static void server_poll(Server *server);

/* these functions help you process the output of the poll */
static short server_new_request_revent(Server *server);
static short server_request_get_revents(Server *server, Request *c);

static size_t server_request_count(Server *server);
static void server_add_request(Server *server, int net_fd);

static int server_step_request(Server *server, Request *c);

static void server_drop_request(Server *server, Request *c);

#endif


#ifdef server_IMPLEMENTATION

static int server_init(Server *server) {
  server->host_fd = socket_host_bind(NULL, "8083");

  if (server->host_fd < 0) {
    return -1;
  }

  return 0;
}

static void server_free(Server *server) {

  /* free all the requests */
  for (Request *next = NULL, *c = server->last_request; c; c = next) {
    next = c->next;
    server_drop_request(server, c);
  }

  close(server->host_fd);

  free(server->pollfds);
}

static short server_new_request_revent(Server *server) {
  /* first fd is always the socket */
  return server->pollfds[0].revents;
}

static short server_request_get_revents(Server *server, Request *c) {
  for (nfds_t i = 0; i < server->pollfd_count; i++)
    if (server->pollfds[i].fd == c->net_fd)
      return server->pollfds[i].revents;
  return 0;
}

static void server_poll(Server *server) {
restart:
  server->pollfd_count = 1 + server_request_count(server);
  server->pollfds = reallocarray(
    server->pollfds,
    server->pollfd_count,
    sizeof(struct pollfd)
  );

  struct pollfd *fd_w = server->pollfds;

  /* first fd is always the socket */
  *fd_w++ = (struct pollfd) { .events = POLLIN, .fd = server->host_fd };

  /* create pollfds for our requests */
  for (Request *c = server->last_request; c; c = c->next)
    *fd_w++ = (struct pollfd) {
      .events = request_events_subscription(c),
      .fd = c->net_fd
    };

#if DEBUG
  printf("polling ... %lu\n", time(NULL));
#endif
  ssize_t updated = poll(server->pollfds, server->pollfd_count, -1);
  if (updated < 0) {
    perror("poll():");
    goto restart;
  }
}

static size_t server_request_count(Server *server) {
  size_t ret = 0;
  for (Request *c = server->last_request; c; c = c->next)
    ret++;
  return ret;
}

static void server_add_request(Server *server, int net_fd) {
  Request *c = calloc(sizeof(Request), 1);
  request_init(c, net_fd);
  c->next = server->last_request;
  server->last_request = c;
}

static void server_drop_request(Server *server, Request *c) {
  request_drop(c);

  if (server->last_request == c) {
    server->last_request = c->next;
  } else
    for (Request *o = server->last_request; o; o = o->next)
      if (o->next == c) {
        o->next = c->next;
        break;
      }

  free(c);
}

static int server_step_request(Server *server, Request *request) {
  restart:
  switch (request_step(request)) {

    case RequestStepResult_Error: {
      server_drop_request(server, request);
      return -1;
    } break;

    case RequestStepResult_NoAction: {
    } break;

    case RequestStepResult_Restart: {
      goto restart;
    } break;

  }

  return 0;
}

#endif
