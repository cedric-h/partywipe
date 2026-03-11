// vim: sw=2 ts=2 expandtab smartindent

#ifndef session_IMPLEMENTATION

typedef struct Session {
  size_t id;
  bool darkmode;
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

typedef struct Rcx {
  FILE *body;
  FILE *css;
} Rcx;

static void session_render_darkmode_detector(Session *sesh, Rcx *rcx) {
  (void)sesh;

  fprintf(rcx->body, "<div id=\"darkmode_detector\">");
  fprintf(rcx->css,
    "\r\n#darkmode_detector {"
    "\r\n  width: 0;"
    "\r\n  height: 0;"
    "\r\n}"
    "\r\n@media (prefers-color-scheme: dark) {"
    "\r\n  #darkmode_detector {"
    "\r\n    background-image: url(\"assets/darkmode_detector_dark\");"
    "\r\n  }"
    "\r\n}"
    "\r\n@media (prefers-color-scheme: light) {"
    "\r\n  #darkmode_detector {"
    "\r\n    background-image: url(\"assets/darkmode_detector_light\");"
    "\r\n  }"
    "\r\n}"
  );
}

static void session_render_fight(Session *sesh, Rcx *rcx) {

  /* combatants */
  {
    fprintf(
      rcx->body,
      "%s",
      (sesh->darkmode)
        ? "<img style=\"width:5rem\" src=\"assets/Ei_DARK.svg\"/>"
        : "<img style=\"width:5rem\" src=\"assets/Ei_LIGHT.svg\"/>"
    );
    fprintf(rcx->css,
      "\r\n"
    );
  }

  /* action bar */
  {
    fprintf(rcx->css,
      "\r\n.action-bar {"
      "\r\n  position: absolute;"
      "\r\n  bottom: 0px;"
      "\r\n  display: flex;"
      "\r\n  flex-wrap: wrap;"
      "\r\n  gap: 0.75rem;"
      "\r\n  width: 100%%;"

      "\r\n  .action-button {"
      "\r\n    &.disabled { opacity: 0.2; }"
      "\r\n    border: 0.1rem solid var(--fg);"
      "\r\n    padding: 0.75rem;"
      "\r\n    flex-grow: 1;"
      "\r\n    flex-shrink: 1;"
      "\r\n    flex-basis: 0;"
      "\r\n    text-align: center;"
      "\r\n  }"
      "\r\n}"
    );

    fprintf(rcx->body,
      "\r\n<div class=\"action-bar\">"
      "\r\n  <a href=\"attack0\" class=\"action-button\">"
      "\r\n    headbutt"
      "\r\n  </a>"
      "\r\n  <a class=\"action-button disabled\">"
      "\r\n    swipe"
      "\r\n  </a>"
      "\r\n  <a href=\"inventory\" class=\"action-button\">"
      "\r\n    inventory"
      "\r\n  </a>"
      "\r\n</div>"
    );
  }

}

static void session_render(Session *sesh, char **page, size_t *page_len) {

  char *css_buf = NULL;
  size_t css_buf_len = 0;
  FILE *css = open_memstream(&css_buf, &css_buf_len);
  fprintf(css,
    "\r\ndocument, body {"
    "\r\n  width: 100vw; height: 100vh;"
    "\r\n  margin: 0px; padding: 0px;"
    "\r\n  font-family: monospace;"
    "\r\n}"
    "\r\n:root {"
    "\r\n  color-scheme: light dark;"
    "\r\n  font-size: calc(6 * min(1vw, 1vh * (9/16)));"
    "\r\n}"
    "\r\n@media (prefers-color-scheme: dark) {"
    "\r\n  :root {"
    "\r\n    --fg: white;"
    "\r\n    --bg: black;"
    "\r\n  }"
    "\r\n}"
    "\r\n@media (prefers-color-scheme: light) {"
    "\r\n  :root {"
    "\r\n    --fg: black;"
    "\r\n    --bg: white;"
    "\r\n  }"
    "\r\n}"
    "\r\nmain {"
    "\r\n  aspect-ratio: 9/16;"
    "\r\n  border: 0.05rem solid var(--fg);"
    "\r\n"
    "\r\n  overflow: hidden;"
    "\r\n  position: absolute;"
    "\r\n  inset: 0;"
    "\r\n  margin: auto;"
    "\r\n  min-height: 0;"
    "\r\n  max-height: calc(100%% - 0.2rem);"
    "\r\n"
    "\r\n  .main-content {"
    "\r\n    position: relative;"
    "\r\n    margin: 1rem;"
    "\r\n    width: stretch;"
    "\r\n    height: stretch;"
    "\r\n  }"
    "\r\n}"
  );

  char *body_buf = NULL;
  size_t body_buf_len = 0;
  FILE *body = open_memstream(&body_buf, &body_buf_len);

  Rcx rcx = { .body = body, .css = css };
  session_render_darkmode_detector(sesh, &rcx);
  session_render_fight(sesh, &rcx);

  fclose(css), fclose(body);
  FILE *out = open_memstream(page, page_len);
  /* actual post-fix newlines on this one since the ones at the end
   * and beginning are actually important */
  fprintf(out,
    "<!DOCTYPE html>\r\n"
    "<html lang='en'>\r\n"
    "  <head>\r\n"
    "    <meta charset='utf-8'/>\r\n"
    "    <link rel=\"icon\" href=\"data:image/svg+xml,<svg xmlns=%%22http://www.w3.org/2000/svg%%22 viewBox=%%220 0 100 100%%22><text y=%%22.9em%%22 font-size=%%2290%%22>⚔️</text></svg>\">\r\n"
    "    <title>Partywipe</title>\r\n"
    "    <style>\r\n"
    "%s\r\n"
    "    </style>\r\n"
    "  </head>\r\n"
    "\r\n"
    "  <body>\r\n"
    "    <main>\r\n"
    "      <div class=\"main-content\">\r\n"
    "%s\r\n"
    "      </div>\r\n"
    "    </main>\r\n"
    "  </body>\r\n"
    "</html>\r\n",
    css_buf,
    body_buf
  );
  free(css_buf);
  free(body_buf);

  fclose(out);
}

#endif
