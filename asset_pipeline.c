// vim: sw=2 ts=2 expandtab smartindent

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int main() {
  DIR *fd = opendir("./assets_raw");
  for (struct dirent *d; (d = readdir(fd));) {

    if (strcmp(d->d_name, ".") == 0)
      continue;

    if (strcmp(d->d_name, "..") == 0)
      continue;

    char in_path[269] = {0};
    snprintf(in_path, sizeof(in_path), "./assets_raw/%s", d->d_name);

    char out_path[269] = {0};
    snprintf(out_path, sizeof(out_path), "./assets/%s", d->d_name);

    FILE *in = fopen(in_path, "r");
    FILE *out = fopen(out_path, "w+");

    fprintf(out, "\""); // wrap the output in quotes so we can #include it in C
    while (true) {
      ssize_t start = ftell(in);

      /* replace the default tldrawa.app color with
       * the foreground CSS variable */
      if (fscanf(in, "#1d1d1d"), ftell(in) != start) {
        fprintf(out, "--var(fg)");
        continue;
      }

      char c = 0;
      if (fscanf(in, "%c", &c) == EOF)
        break;

      /* escape quotes */
      if (c == '"') {
        fprintf(out, "\\\"");
        continue;
      }

      fprintf(out, "%c", c);
    }
    fprintf(out, "\""); // wrap the output in quotes so we can #include it in C

    fclose(in);
    fclose(out);
  }
  return 0;
}
