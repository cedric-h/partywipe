// vim: sw=2 ts=2 expandtab smartindent

#define HTML_RES \
"<!DOCTYPE html>\r\n" \
"<html lang='en'>\r\n" \
"  <head>\r\n" \
"    <meta charset='utf-8'/>\r\n" \
"    <title>Partywipe</title>\r\n" \
"    <style>\r\n" \
"      document, body { width: 100vw; height: 100vh; margin: 0px; padding: 0px; }\r\n" \
"      :root {\r\n" \
"        color-scheme: light dark;\r\n" \
"        font-size: calc(6 * min(1vw, 1vh * (9/16)));\r\n" \
"      }\r\n" \
"      @media (prefers-color-scheme: dark) {\r\n" \
"        :root {\r\n" \
"          --fg: white;\r\n" \
"          --bg: black;\r\n" \
"        }\r\n" \
"      }\r\n" \
"      @media (prefers-color-scheme: light) {\r\n" \
"        :root {\r\n" \
"          --fg: black;\r\n" \
"          --bg: white;\r\n" \
"        }\r\n" \
"      }\r\n" \
"      main {\r\n" \
"        aspect-ratio: 9/16;\r\n" \
"        border: 1px solid var(--fg);\r\n" \
"\r\n" \
"        overflow: hidden;\r\n" \
"        position: absolute;\r\n" \
"        inset: 0;\r\n" \
"        margin: auto;\r\n" \
"        min-height: 0;\r\n" \
"        max-height: calc(100% - 2px);\r\n" \
"\r\n" \
"        .main-content {\r\n" \
"          position: relative;\r\n" \
"          margin: 1rem;\r\n" \
"          width: stretch;\r\n" \
"          height: stretch;\r\n" \
"        }\r\n" \
"      }\r\n" \
"    </style>\r\n" \
"  </head>\r\n" \
"\r\n" \
"  <body>\r\n" \
"    <main>\r\n" \
"      <div class=\"main-content\">\r\n" \
"        <div style=\"position:absolute;bottom:0px;\">\r\n" \
"          At first, there was nothing ...\r\n" \
"        </div>\r\n" \
"      </div>\r\n" \
"    </main>\r\n" \
"  </body>\r\n" \
"</html>\r\n"

static int request_http_respond_to_request(Request *c) {

  char path[31] = {0};
  size_t cookie = 0;
  {
    fclose(c->http_req.file);
    c->http_req.file = NULL;

    /* cannot fscanf a memstream, can fscanf an fmemopen */
    /* (can grow a memstream, cannot grow an fmemopen) */
    FILE *req = fmemopen(c->http_req.buf, c->http_req.buf_len, "r");

    if (fscanf(req, "GET %30s HTTP/1.1\r\n", path) == 0) {
      fclose(req);
      return -1;
    }

    while (fscanf(req, "cookie: __Http-Sesh=%lu\r\n", &cookie) <= 0)
      if (fscanf(req, "%*[^\n]\n") == EOF)
        break; /* no cookie found */

    fclose(req);
    free(c->http_req.buf);
    c->http_req.buf = NULL;
  }

  printf("cookie = \"%lu\"\n", cookie);

  c->phase = RequestPhase_HttpResponding;

  if (strcmp(path, "/") == 0) {
    FILE *tmp = open_memstream(&c->res.buf, &c->res.buf_len);
    fprintf(tmp, "HTTP/1.0 200 OK\r\n");
    fprintf(tmp, "Content-Length: %lu\r\n", strlen(HTML_RES) - 2);
    fprintf(tmp, "Connection: close\r\n");
    fprintf(tmp, "Content-Type: text/html; charset=utf-8\r\n");
    if (cookie == 0) {
      cookie = 1;
      fprintf(
        tmp,
        "Set-Cookie: __Http-Sesh=%lu; "
          "HttpOnly; "
          "Secure; "
          "SameSite=Strict;"
          "\r\n",
        cookie
      );
    }
    fprintf(tmp, "\r\n");

    fprintf(tmp, "%s", HTML_RES);

    fclose(tmp);
  } else {
    FILE *tmp = open_memstream(&c->res.buf, &c->res.buf_len);
    fprintf(tmp, "HTTP/1.1 404 Not Found\r\n\r\n");
    fclose(tmp);
  }

  return 0;
}

static RequestStepResult request_http_read_request(Request *c) {
  for (;;) {
    char byte;
    ssize_t read_ret = read(c->net_fd, &byte, 1);
    if (read_ret < 1) {
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        perror("request read()");
        return RequestStepResult_Error;
      }
      break;
    }

    fwrite(&byte, 1, 1, c->http_req.file);
    c->http_req.bytes_read++;

    if (c->http_req.bytes_read > MAX_MESSAGE_SIZE)
      return RequestStepResult_Error;

    /* ignore carriage return */
    if (byte != 0x0D) {
      /* track line feeds */
      if (byte == 0x0A) {
        if (c->http_req.seen_linefeed) {
          if (request_http_respond_to_request(c) < 0)
            return RequestStepResult_Error;
          return RequestStepResult_Restart;
        }
        c->http_req.seen_linefeed = 1;
      } else {
        c->http_req.seen_linefeed = 0;
      }
    }
  }

  return RequestStepResult_NoAction;
}

static RequestStepResult request_http_write_response(Request *c) {
  while (c->res.progress < c->res.buf_len) {
    char byte = c->res.buf[c->res.progress];
    ssize_t wlen = write(c->net_fd, &byte, 1);

    if (wlen < 1) {
      if (errno != EWOULDBLOCK && errno != EAGAIN) {
        perror("request write()");
        return RequestStepResult_Error;
      }
      return RequestStepResult_NoAction;
    }

    /* important to only increase this if write succeeds */
    c->res.progress += 1;
  }

  if (c->res.progress == c->res.buf_len) {
    return RequestStepResult_Error;
  }

  return RequestStepResult_NoAction;
}
