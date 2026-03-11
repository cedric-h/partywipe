// vim: sw=2 ts=2 expandtab smartindent

#ifndef server_IMPLEMENTATION

typedef struct {
  char *name;
  char *content;
} ServerAsset;

typedef struct {

  int host_fd;
  /* The head of the linked list of requests */
  Request *last_request;

  ServerAsset *assets;
  size_t asset_count;

  struct pollfd *pollfds;
  nfds_t pollfd_count;

  Session *sessions;
  size_t session_count;
} Server;

static int server_init(Server *server);
static void server_free(Server *server);

/**
 * wait indefinitely until there is work to be done!
 **/
static void server_poll(Server *server);

/* these functions help you process the output of the poll */
static short server_new_request_revent(Server *server);
static short server_request_get_revents(Server *server, Request *r);

static size_t server_request_count(Server *server);
static void server_add_request(Server *server, int net_fd);

static int server_step_request(Server *server, Request *r);

static void server_drop_request(Server *server, Request *r);

#endif


#ifdef server_IMPLEMENTATION

static int server_init(Server *server) {
  server->host_fd = socket_host_bind(NULL, "8083");

  {
    DIR *fd = opendir("./assets");

    for (struct dirent *d; (d = readdir(fd));) {

      if (strcmp(d->d_name, "." ) == 0) continue;
      if (strcmp(d->d_name, "..") == 0) continue;

      char *content;
      {
        char in_path[269] = {0};
        snprintf(in_path, sizeof(in_path), "./assets/%s", d->d_name);
        FILE *in_f = fopen(in_path, "r");

        fseek(in_f, 0, SEEK_END);
        size_t size = (size_t)ftell(in_f);
        rewind(in_f);

        content = malloc(size + 1);
        fread(content, 1, size, in_f);
        content[size] = '\0';
        fclose(in_f);
      }

      size_t n = server->asset_count++;
      server->assets = reallocarray(
        server->assets,
        server->asset_count,
        sizeof(server->assets[0])
      );

      server->assets[n] = (ServerAsset) {
        .name = strdup(d->d_name),
        .content = content,
      };
    }
  }

  if (server->host_fd < 0) {
    return -1;
  }

  return 0;
}

static void server_free(Server *server) {

  /* free all the requests */
  for (Request *next = NULL, *r = server->last_request; r; r = next) {
    next = r->next;
    server_drop_request(server, r);
  }

  for (
    ServerAsset *asset = server->assets;
    (server->assets - asset) < (ssize_t)server->asset_count;
    asset++
  ) {
    free(asset->name);
    free(asset->content);
  }
  free(server->assets);

  for (
    Session *sesh = server->sessions;
    (server->sessions - sesh) < (ssize_t)server->session_count;
    sesh++
  ) {
    session_free(sesh);
  }
  free(server->sessions);

  close(server->host_fd);

  free(server->pollfds);
}

static short server_new_request_revent(Server *server) {
  /* first fd is always the socket */
  return server->pollfds[0].revents;
}

static short server_request_get_revents(Server *server, Request *r) {
  for (nfds_t i = 0; i < server->pollfd_count; i++)
    if (server->pollfds[i].fd == r->net_fd)
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
  for (Request *r = server->last_request; r; r = r->next)
    *fd_w++ = (struct pollfd) {
      .events = request_events_subscription(r),
      .fd = r->net_fd
    };

#if DEBUG
  printf("polling ... %lu\n", time(NULL));
#endif

  ssize_t updated = poll(server->pollfds, server->pollfd_count, -1);
  if (updated < 0) {

#if DEBUG
    if (errno == EINTR) {
      exit(0);
    }
#endif

    perror("poll():");
    goto restart;
  }
}

static size_t server_request_count(Server *server) {
  size_t ret = 0;
  for (Request *r = server->last_request; r; r = r->next)
    ret++;
  return ret;
}

static void server_add_request(Server *server, int net_fd) {
  Request *r = calloc(sizeof(Request), 1);
  request_init(r, net_fd);
  r->next = server->last_request;
  server->last_request = r;
}

static void server_drop_request(Server *server, Request *r) {
  request_drop(r);

  if (server->last_request == r) {
    server->last_request = r->next;
  } else
    for (Request *o = server->last_request; o; o = o->next)
      if (o->next == r) {
        o->next = r->next;
        break;
      }

  free(r);
}

static int server_http_respond(Server *server, Request *r) {

  char path[31] = {0};
  size_t cookie = 0;
  {
    fclose(r->http_req.file);
    r->http_req.file = NULL;

    /* cannot fscanf a memstream, can fscanf an fmemopen */
    /* (can grow a memstream, cannot grow an fmemopen) */
    FILE *req = fmemopen(r->http_req.buf, r->http_req.buf_len, "r");

    if (fscanf(req, "GET %30s HTTP/1.1\r\n", path) == 0) {
      fclose(req);
      return -1;
    }

    while (fscanf(req, "cookie: __Http-Sesh=%lu\r\n", &cookie) <= 0)
      if (fscanf(req, "%*[^\n]\n") == EOF)
        break; /* no cookie found */

    fclose(req);
    free(r->http_req.buf);
    r->http_req.buf = NULL;
  }

  r->phase = RequestPhase_HttpResponding;

  {
    char asset_name[40] = {0};
    if (sscanf(path, "/assets/%40s", asset_name) > 0) {
      FILE *tmp = open_memstream(&r->res.buf, &r->res.buf_len);
      fprintf(tmp, "HTTP/1.0 200 OK\r\n");
      fprintf(tmp, "Connection: close\r\n");
      fprintf(tmp, "Cache-Control: no-cache\r\n");
      fprintf(tmp, "Content-Type: image/svg+xml; charset=utf-8\r\n");

      char *content = 
          "\r\n<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\">"
          "\r\n  <text"
          "\r\n    x=\"50%\""
          "\r\n    y=\"50%\""
          "\r\n    dominant-baseline=\"middle\""
          "\r\n    text-anchor=\"middle\""
          "\r\n    font-size=\"80\""
          "\r\n  >⁉️</text>"
          "\r\n</svg>"
        ;

      for (size_t i = 0; i < server->asset_count; i++) {
        ServerAsset *sa = server->assets + i;
        if (strcmp(sa->name, asset_name) == 0) {
          content = sa->content;
          break;
        }
      }

      fprintf(tmp, "Content-Length: %lu\r\n", strlen(content));

      fprintf(tmp, "\r\n");
      fprintf(tmp, "%s", content);
      fprintf(tmp, "\r\n");
      fclose(tmp);

      return 0;
    }
  }

  {
    if (cookie == 0 || cookie > server->session_count) {
      size_t n = server->session_count++;
      server->sessions = reallocarray(
        server->sessions,
        server->session_count,
        sizeof(server->sessions[0])
      );
      session_init(server->sessions + n, server->session_count);
    }
    Session *sesh = server->sessions + (cookie - 1);

    char *page = NULL;
    size_t page_len = 0;
    session_render(sesh, &page, &page_len);

    FILE *tmp = open_memstream(&r->res.buf, &r->res.buf_len);
    fprintf(tmp, "HTTP/1.0 200 OK\r\n");
    fprintf(tmp, "Content-Length: %lu\r\n", page_len - 2);
    fprintf(tmp, "Connection: close\r\n");
    fprintf(tmp, "Content-Type: text/html; charset=utf-8\r\n");
    if (cookie == 0)
      fprintf(
        tmp,
        "Set-Cookie: __Http-Sesh=%lu; "
          "HttpOnly; Secure; "
          "SameSite=Strict;\r\n",
        sesh->id
      );

    fprintf(tmp, "\r\n");

    fprintf(tmp, "%s", page);
    free(page);

    fclose(tmp);
  }

  return 0;
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

    case RequestStepResult_DoneReading: {
      server_http_respond(server, request);
      goto restart;
    } break;

    case RequestStepResult_Restart: {
      goto restart;
    } break;

  }

  return 0;
}

#endif
