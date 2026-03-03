// vim: sw=2 ts=2 expandtab smartindent

#ifndef server_IMPLEMENTATION

typedef struct {

  int host_fd;
  size_t client_id_i;
  /* The head of the linked list of clients */
  Client *last_client;

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
static short server_new_client_revent(Server *server);
static short server_client_get_revents(Server *server, Client *c);

static size_t server_client_count(Server *server);
static void server_add_client(Server *server, int net_fd);

static int server_step_client(Server *server, Client *c);

static void server_drop_client(Server *server, Client *c);

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

  /* free all the clients */
  for (Client *next = NULL, *c = server->last_client; c; c = next) {
    next = c->next;
    server_drop_client(server, c);
  }

  close(server->host_fd);

  free(server->pollfds);
}

static short server_new_client_revent(Server *server) {
  /* first fd is always the socket */
  return server->pollfds[0].revents;
}

static short server_client_get_revents(Server *server, Client *c) {
  for (nfds_t i = 0; i < server->pollfd_count; i++)
    if (server->pollfds[i].fd == c->net_fd)
      return server->pollfds[i].revents;
  return 0;
}

static void server_poll(Server *server) {
restart:
  server->pollfd_count = 1 + server_client_count(server);
  server->pollfds = reallocarray(
    server->pollfds,
    server->pollfd_count,
    sizeof(struct pollfd)
  );

  struct pollfd *fd_w = server->pollfds;

  /* first fd is always the socket */
  *fd_w++ = (struct pollfd) { .events = POLLIN, .fd = server->host_fd };

  /* create pollfds for our clients */
  for (Client *c = server->last_client; c; c = c->next)
    *fd_w++ = (struct pollfd) {
      .events = client_events_subscription(c),
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

static size_t server_client_count(Server *server) {
  size_t ret = 0;
  for (Client *c = server->last_client; c; c = c->next)
    ret++;
  return ret;
}

static void server_add_client(Server *server, int net_fd) {
  Client *c = calloc(sizeof(Client), 1);
  client_init(c, net_fd, server->client_id_i++);
  c->next = server->last_client;
  server->last_client = c;
}

static void server_drop_client(Server *server, Client *c) {
  client_drop(c);

  if (server->last_client == c) {
    server->last_client = c->next;
  } else
    for (Client *o = server->last_client; o; o = o->next)
      if (o->next == c) {
        o->next = c->next;
        break;
      }

  free(c);
}

static int server_step_client(Server *server, Client *client) {
  restart:
  switch (client_step(client)) {

    case ClientStepResult_Error: {
      server_drop_client(server, client);
      return -1;
    } break;

    case ClientStepResult_NoAction: {
    } break;

    case ClientStepResult_Restart: {
      goto restart;
    } break;

  }

  return 0;
}

#endif
