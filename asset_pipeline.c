// vim: sw=2 ts=2 expandtab smartindent

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

int main() {
  DIR *fd = opendir("./assets_raw");
  for (struct dirent *d; (d = readdir(fd));) {

    char asset_name[40] = {0};
    if (sscanf(d->d_name, "%[^.].svg", asset_name) < 1) {
      printf("ignoring %s\n", d->d_name);
      continue;
    }

    char *og_in = NULL;
    {
      char in_path[269] = {0};
      snprintf(in_path, sizeof(in_path), "./assets_raw/%s", d->d_name);
      FILE *in_f = fopen(in_path, "r");

      fseek(in_f, 0, SEEK_END);
      size_t size = (size_t)ftell(in_f);
      rewind(in_f);

      og_in = malloc(size + 1);
      fread(og_in, 1, size, in_f);
      og_in[size] = '\0';
      fclose(in_f);
    }
    char *in = og_in;

    char out_path[269] = {0};
    snprintf(out_path, sizeof(out_path), "./assets/%s.svg", asset_name);
    FILE *out = fopen(out_path, "w+");

    while (true) {
      int n = 0;

      /* strip out the background */
      if (
        sscanf(in, "background-color: rgb(249, 250, 251);%n", &n),
        n == strlen("background-color: rgb(249, 250, 251);")
      ) {
        in += n;
        continue;
      }

      char c = *in++;
      if (c == 0) break;

      fprintf(out, "%c", c);
    }

    fclose(out);
    free(og_in);

  }
  return 0;
}
