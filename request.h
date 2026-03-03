// vim: sw=2 ts=2 expandtab smartindent

#ifndef request_IMPLEMENTATION

typedef enum RequestPhase {
  RequestPhase_Empty,
  RequestPhase_HttpRequesting,
  RequestPhase_HttpResponding,
} RequestPhase;

typedef struct RequestResponse {
  struct RequestResponse *next;

  /* response data goes in here */
  char *buf;
  size_t buf_len, progress;
} RequestResponse;

/**
 * If any HTTP message over this size,
 * we drop the request.
 **/
#define MAX_MESSAGE_SIZE (1 << 13)
typedef struct Request {
  struct Request *next;

  RequestPhase phase;

  int net_fd; /* net fd from accept() */

  time_t last_activity;

  /* requesting */
  struct {
    FILE *file; /* closed after requesting */
    bool seen_linefeed;
    size_t bytes_read;

    /* request data goes in here when file is closed */
    char *buf;
    size_t buf_len;
  } http_req;

  RequestResponse res;

} Request;

static void request_init(Request *c, int net_fd);

/**
 * Tells the server which events are worth waking up for.
 * Important not to subscribe to an event you don't handle,
 * otherwise the server will keep waking up and asking you
 * to handle it, which will burn a lot of CPU cycles.
 **/
static short request_events_subscription(Request *c);

typedef enum {
  RequestStepResult_Error,
  RequestStepResult_NoAction,
  RequestStepResult_Restart,
} RequestStepResult;
static RequestStepResult request_step(Request *c);

static void request_drop(Request *c);

static int request_http_respond_to_request(Request *c);

#endif



#ifdef request_IMPLEMENTATION

#include "request_http.h"

static void request_init(Request *c, int net_fd) {
  *c = (Request) {
    .last_activity = time(NULL),
    .phase = RequestPhase_HttpRequesting,
    .net_fd = net_fd,
  };
  c->http_req.file = open_memstream(
    &c->http_req.buf,
    &c->http_req.buf_len
  );
}

static void request_drop(Request *c) {
  if (c->phase == RequestPhase_HttpRequesting) {
    if (c->http_req.file != NULL)
      fclose(c->http_req.file);
    if (c->http_req.buf != NULL)
      free(c->http_req.buf);
  }

  c->phase = RequestPhase_Empty;

  /* free any lingering queued RequestResponses */
  if (c->res.next) {
    RequestResponse *last = c->res.next;
    for (
      RequestResponse *next = last->next;
      last;
      last = next
    ) {
      next = last->next;
      free(last->buf);
      free(last);
    }
  }

  if (c->     res.buf     != NULL) free(c->res.buf);

  close(c->net_fd);
}

static short request_events_subscription(Request *c) {
  short events = 0;
  short events_writes = POLLWRNORM | POLLWRBAND          ;
  short events_reads  = POLLRDNORM | POLLRDBAND | POLLPRI;

  switch (c->phase) {
    case RequestPhase_Empty: {
      /* this probably shouldn't happen */
      fprintf(stderr, "empty request!?\n");
    } break;
    case RequestPhase_HttpRequesting: {
      events = events_writes;
    } break;
    case RequestPhase_HttpResponding: {
      events = events_reads;
    } break;
  }

  return events;
}

static RequestStepResult request_step(Request *c) {

  if ((c->phase == RequestPhase_HttpRequesting) ||
      (c->phase == RequestPhase_HttpRequesting)) {
    long int time_since_io = time(NULL) - c->last_activity;

    /* timeout */
    if (time_since_io > 1)
      return RequestStepResult_Error;
  }

  switch (c->phase) {

    case RequestPhase_Empty:
      return RequestStepResult_NoAction;

    case RequestPhase_HttpRequesting:
      return request_http_read_request(c);

    case RequestPhase_HttpResponding:
      return request_http_write_response(c);

  }

  return RequestStepResult_NoAction;
}

#endif
