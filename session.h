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

typedef struct Rcx {
  FILE *body;
  FILE *css;
} Rcx;

static void session_render_fight(Session *sesh, Rcx *rcx) {
  (void) sesh;

#define ATTACK_DIST "8.5rem"

  /* combatants */
  {
    fprintf(
      rcx->body,
      "\r\n<div class=\"combatant-board\">"
      "\r\n  <div class=\"enemy-row\">"
      "\r\n    <img class=\"combatant-image active\" src=\"assets/Ei.svg\"/>"
      "\r\n  </div>"
      "\r\n  <div class=\"player-row\">"
      "\r\n    <img class=\"combatant-image active\" src=\"assets/brotchen.svg\"/>"
      "\r\n  </div>"
      "\r\n</div>"
    );
    fprintf(rcx->css,
      "\r\n.combatant-board {"
      "\r\n  width: 100%%;"
      "\r\n  aspect-ratio: 1;"
      "\r\n  display: flex;"
      "\r\n  flex-direction: column;"
      "\r\n  justify-content: space-between;"
      "\r\n  .player-row,.enemy-row {"
      "\r\n    width: 100%%;"
      "\r\n    display: flex;"
      "\r\n  }"
      "\r\n  .enemy-row {"
      "\r\n    flex-direction: row-reverse;"
      "\r\n  }"
      "\r\n  --attack-duration: 0.5s;"
      "\r\n  --attack-timing: cubic-bezier(0.42, 0, 1, 1);"
      "\r\n  .player-row {"
      "\r\n    --attack-delay: 0.2s;"
      "\r\n    --attack-frames: player-attack;"
      "\r\n    --ouch-delay: 2.5s;"
      "\r\n    --ouch-frames: player-ouch;"
      "\r\n  }"
      "\r\n  .enemy-row {"
      "\r\n    --attack-delay: 2.0s;"
      "\r\n    --attack-frames: enemy-attack;"
      "\r\n    --ouch-delay: 0.7s;"
      "\r\n    --ouch-frames: enemy-ouch;"
      "\r\n  }"
      "\r\n  .active {" /*                               attack, ouch */
      "\r\n    animation-duration:       var(--attack-duration), 0.2s;"
      "\r\n    animation-delay:             var(--attack-delay), var(--ouch-delay);"
      "\r\n    animation-timing-function:  var(--attack-timing), ease-out;"
      "\r\n    animation-iteration-count:                     2, 2;"
      "\r\n    animation-direction:                   alternate, alternate;"
      "\r\n    animation-fill-mode:                        both, both;"
      "\r\n    animation-name:             var(--attack-frames), var(--ouch-frames);"
      "\r\n    animation-composition: accumulate;"
      "\r\n  }"
      "\r\n  .combatant-image {"
      "\r\n    width: 5rem;"
      "\r\n  }"
      "\r\n}"

      "\r\n@keyframes player-attack {"
      "\r\n  from { translate: 0; }"
      "\r\n  40%% { translate: -1rem 1rem; }"
      "\r\n  to { translate: "ATTACK_DIST" -"ATTACK_DIST"; }"
      "\r\n}"
      "\r\n@keyframes enemy-attack {"
      "\r\n  from { translate: 0; }"
      "\r\n  40%% { translate: 1rem -1rem; }"
      "\r\n  to { translate: -"ATTACK_DIST" "ATTACK_DIST"; }"
      "\r\n}"

      "\r\n@keyframes enemy-ouch {"
      "\r\n  from { translate: 0; }"
      "\r\n  to { translate: 0.65rem -0.6175rem; }"
      "\r\n}"
      "\r\n@keyframes player-ouch {"
      "\r\n  from { translate: 0; }"
      "\r\n  to { translate: -0.65rem 0.6175rem; }"
      "\r\n}"

      "\r\n@media (prefers-color-scheme: dark) {"
      "\r\n  .combatant-image {"
      "\r\n    filter: invert(1);"
      // Ei looks good either way
      "\r\n    &[src=\"assets/Ei.svg\"] { filter: revert; }"
      "\r\n  }"
      "\r\n}"
    );
  }
#undef ATTACK_DIST

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
