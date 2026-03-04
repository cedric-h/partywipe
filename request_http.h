// vim: sw=2 ts=2 expandtab smartindent

static RequestStepResult request_http_read_request(Request *r) {
  for (;;) {
    char byte;
    ssize_t read_ret = read(r->net_fd, &byte, 1);
    if (read_ret < 1) {
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        perror("request read()");
        return RequestStepResult_Error;
      }
      break;
    }

    fwrite(&byte, 1, 1, r->http_req.file);
    r->http_req.bytes_read++;

    if (r->http_req.bytes_read > MAX_MESSAGE_SIZE)
      return RequestStepResult_Error;

    /* ignore carriage return */
    if (byte != 0x0D) {
      /* track line feeds */
      if (byte == 0x0A) {
        if (r->http_req.seen_linefeed) {
          return RequestStepResult_DoneReading;
        }
        r->http_req.seen_linefeed = 1;
      } else {
        r->http_req.seen_linefeed = 0;
      }
    }
  }

  return RequestStepResult_NoAction;
}

static RequestStepResult request_http_write_response(Request *r) {
  while (r->res.progress < r->res.buf_len) {
    char byte = r->res.buf[r->res.progress];
    ssize_t wlen = write(r->net_fd, &byte, 1);

    if (wlen < 1) {
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        perror("request write()");
        return RequestStepResult_Error;
      }
      return RequestStepResult_NoAction;
    }

    /* important to only increase this if write succeeds */
    r->res.progress += 1;
  }

  if (r->res.progress == r->res.buf_len) {
    return RequestStepResult_Error;
  }

  return RequestStepResult_NoAction;
}
