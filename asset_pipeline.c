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

    typedef enum {
      AssetMode_None,
      AssetMode_Dark,
      AssetMode_Light,
    } AssetMode;
    AssetMode asset_mode_in = AssetMode_None;
    {
      char asset_mode[40] = {0};
      if (sscanf(d->d_name, "%*[^._]_%[^._].svg", asset_mode) > 0) {
        if (strcmp(asset_mode, "DARK" ) == 0)
          asset_mode_in = AssetMode_Dark;
        else if (strcmp(asset_mode, "LIGHT") == 0)
          asset_mode_in = AssetMode_Light;
        else {
          fprintf(stderr, "No such asset mode: %s", asset_mode);
          exit(1);
        }
      }
    }

    for (int i = 0; i < 2; i++) {

      bool dark = i == 0;

      /* we only need to run this once if we know what asset mode */
      if (asset_mode_in != AssetMode_None) {
        if ( dark && asset_mode_in != AssetMode_Dark) continue;
        if (!dark && asset_mode_in != AssetMode_Light) continue;
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
      if (asset_mode_in == AssetMode_None) {
        char *suffix = dark ? "DARK" : "LIGHT";
        snprintf(
          out_path,
          sizeof(out_path),
          "./assets/%s_%s.svg",
          asset_name,
          suffix
        );
      } else {
        snprintf(out_path, sizeof(out_path), "./assets/%s.svg", asset_name);
      }
      FILE *out = fopen(out_path, "w+");

      fprintf(out, "\""); // wrap the output in quotes so we can #include it in C
      while (true) {
        int n = 0;

        /* we only need to do these automatic translations if the artist
         * hasn't already done them for us. */
        if (asset_mode_in == AssetMode_None) {
          /* replace the default tldraw.app off-black with off-white */
          if (dark) {
            if (sscanf(in, "#1d1d1d%n", &n), n == strlen("#1d1d1d")) {
              in += n;
              fprintf(out, "#e2e2e2");
              continue;
            }
          }
        }

        if (
          sscanf(in, "background-color: rgb(249, 250, 251);%n", &n),
          n == strlen("background-color: rgb(249, 250, 251);")
        ) {
          in += n;
          continue;
        }

        char c = *in++;
        if (c == 0) break;

        /* escape quotes */
        if (c == '"') {
          fprintf(out, "\\\"");
          continue;
        }

        /* no newlines. */
        if (c == '\n')
          continue;

        fprintf(out, "%c", c);
      }
      fprintf(out, "\""); // wrap the output in quotes so we can #include it in C

      fclose(out);
      free(og_in);
    }

  }
  return 0;
}
