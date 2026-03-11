// vim: sw=2 ts=2 expandtab smartindent

#ifndef server_IMPLEMENTATION

typedef struct {

  int host_fd;
  /* The head of the linked list of requests */
  Request *last_request;

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

static char *svg_dark_ei =
#include "assets/Ei_DARK.svg"
;
static char *svg_light_ei =
#include "assets/Ei_LIGHT.svg"
;

static int server_http_respond(Server *server, Request *r) {

  char path[51] = {0};
  size_t sesh_cookie = 0;
  bool darkmode_cookie = false;
  {
    fclose(r->http_req.file);
    r->http_req.file = NULL;

    /* cannot fscanf a memstream, can fscanf an fmemopen */
    /* (can grow a memstream, cannot grow an fmemopen) */
    FILE *req = fmemopen(r->http_req.buf, r->http_req.buf_len, "r");

    if (fscanf(req, "GET %50s HTTP/1.1\r\n", path) == 0) {
      fclose(req);
      return -1;
    }

    while (true) {
      char cookie_name[20] = {0};

      if (fscanf(req, "cookie: __Http-%[^=]", cookie_name) > 0) {
        printf("cookie_name = %s\n", cookie_name);

        if (strcmp(cookie_name, "Sesh") == 0)
          fscanf(req, "=%lu\r\n", &sesh_cookie);

        char tmp[10] = {0};
        if (strcmp(cookie_name, "Darkmode") == 0)
          if (fscanf(req, "=%10s\r\n", tmp) > 0) {
            printf("tmp = %s\n", tmp);
            darkmode_cookie = strcmp(tmp, "dark") == 0;
          }
      }

      if (fscanf(req, "%*[^\n]\n") == EOF)
        break; /* no sesh_cookie found */
    }

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

      if (strcmp(asset_name, "Ei_DARK.svg" ) == 0) content = svg_dark_ei;
      if (strcmp(asset_name, "Ei_LIGHT.svg") == 0) content = svg_light_ei;
      printf("asset_name = %s\n", asset_name);

      if (sscanf(path, "/assets/darkmode_detector_%40s", asset_name) > 0) {
        fprintf(
          tmp,
          "Set-Cookie: __Http-Darkmode=%s; "
            "HttpOnly; Secure; "
            "SameSite=Strict;\r\n",
          asset_name
        );

        content =
          "\r\n<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\">"
          "\r\n  <text"
          "\r\n    x=\"50%\""
          "\r\n    y=\"50%\""
          "\r\n    dominant-baseline=\"middle\""
          "\r\n    text-anchor=\"middle\""
          "\r\n    font-size=\"80\""
          "\r\n  >🌑</text>"
          "\r\n</svg>"
        ;
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
    if (sesh_cookie == 0 || sesh_cookie > server->session_count) {
      size_t n = server->session_count++;
      server->sessions = reallocarray(
        server->sessions,
        server->session_count,
        sizeof(server->sessions[0])
      );
      session_init(server->sessions + n, server->session_count);
    }
    Session *sesh = server->sessions + (sesh_cookie - 1);

    char *page = NULL;
    size_t page_len = 0;
    sesh->darkmode = darkmode_cookie;
    session_render(sesh, &page, &page_len);

    FILE *tmp = open_memstream(&r->res.buf, &r->res.buf_len);
    fprintf(tmp, "HTTP/1.0 200 OK\r\n");
    fprintf(tmp, "Content-Length: %lu\r\n", page_len - 2);
    fprintf(tmp, "Connection: close\r\n");
    fprintf(tmp, "Content-Type: text/html; charset=utf-8\r\n");
    if (sesh_cookie == 0)
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
