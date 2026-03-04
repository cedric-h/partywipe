// vim: sw=2 ts=2 expandtab smartindent

#ifndef request_IMPLEMENTATION

typedef enum RequestPhase {
  RequestPhase_HttpRequesting,
  RequestPhase_HttpResponding,
} RequestPhase;

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

  struct RequestResponse {
    /* response data goes in here */
    char *buf;
    size_t buf_len, progress;
  } res;

} Request;

static void request_init(Request *r, int net_fd);

/**
 * Tells the server which events are worth waking up for.
 * Important not to subscribe to an event you don't handle,
 * otherwise the server will keep waking up and asking you
 * to handle it, which will burn a lot of CPU cycles.
 **/
static short request_events_subscription(Request *r);

typedef enum {
  RequestStepResult_Error,
  RequestStepResult_NoAction,
  RequestStepResult_DoneReading,
  /* this just means "it's probably meaningful to step more,"
   * e.g. because you finished reading and can now start writing */
  RequestStepResult_Restart,
} RequestStepResult;
static RequestStepResult request_step(Request *r);

static void request_drop(Request *r);

#endif



#ifdef request_IMPLEMENTATION

#include "request_http.h"

static void request_init(Request *r, int net_fd) {
  *r = (Request) {
    .last_activity = time(NULL),
    .phase = RequestPhase_HttpRequesting,
    .net_fd = net_fd,
  };
  r->http_req.file = open_memstream(
    &r->http_req.buf,
    &r->http_req.buf_len
  );
}

static void request_drop(Request *r) {
  if (r->phase == RequestPhase_HttpRequesting) {
    if (r->http_req.file != NULL)
      fclose(r->http_req.file);
    if (r->http_req.buf != NULL)
      free(r->http_req.buf);
  }

  if (r->res.buf != NULL) free(r->res.buf);

  close(r->net_fd);
}

static short request_events_subscription(Request *r) {
  short events = 0;
  short events_writes = POLLWRNORM | POLLWRBAND          ;
  short events_reads  = POLLRDNORM | POLLRDBAND | POLLPRI;

  switch (r->phase) {
    case RequestPhase_HttpRequesting: {
      events = events_writes;
    } break;
    case RequestPhase_HttpResponding: {
      events = events_reads;
    } break;
  }

  return events;
}

static RequestStepResult request_step(Request *r) {

  if ((r->phase == RequestPhase_HttpRequesting) ||
      (r->phase == RequestPhase_HttpRequesting)) {
    long int time_since_io = time(NULL) - r->last_activity;

    /* timeout */
    if (time_since_io > 1)
      return RequestStepResult_Error;
  }

  switch (r->phase) {

    case RequestPhase_HttpRequesting:
      return request_http_read_request(r);

    case RequestPhase_HttpResponding:
      return request_http_write_response(r);

  }

  return RequestStepResult_NoAction;
}

#endif
