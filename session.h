// vim: sw=2 ts=2 expandtab smartindent

#ifndef session_IMPLEMENTATION

typedef struct Session {
  size_t id;
} Session;

static void session_init(Session *s, size_t id);
static void session_render(Session *s, char **page, size_t *page_len);
static void session_free(Session *s);

#endif



#ifdef session_IMPLEMENTATION

static void session_init(Session *s, size_t id) {
  *s = (Session) {
    .id = id,
  };
}

static void session_free(Session *s) {
  (void)s;
}

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

static void session_render(Session *s, char **page, size_t *page_len) {
  (void)s;

  FILE *tmp = open_memstream(page, page_len);

  fprintf(tmp, "%s", HTML_RES);

  fclose(tmp);
}

#endif
