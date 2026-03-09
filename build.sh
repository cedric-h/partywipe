mkdir -p assets

cd assets_raw
for f in *; do
  printf '"%s"' "\
          $(sed 's/"/\\"/g' "$f" | sed 's/#1d1d1d/var(--fg)/g')\
  " > "../assets/$f"
done
cd ..

gcc -g \
  -Wall \
  -Werror \
  -Wextra \
  -Wpedantic \
  -Wconversion \
  -Wformat=2 \
  -Wshadow \
  -Wno-overlength-strings \
  page.c
