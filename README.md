# dev

Rebuild whenever any of the code changes
- [`find *.c *.h | entr -rs 'clear && gcc -g -Wall -Werror -Wextra -Wpedantic -Wconversion -Wformat=2 -Wshadow page.c && ./a.out'`](https://github.com/eradman/entr)

Run with leak/memory checking:
- [`gcc -Wall -Werror -O0 -g page.c && valgrind --leak-check=yes ./a.out`](https://valgrind.org/docs/manual/quick-start.html)

# deployment

## Example nginx reverse proxy:
in `/etc/nginx/sites-available/default`, in a `server` block

```nginx
	location /partywipe/ {
		proxy_pass http://localhost:8083/;
		proxy_http_version 1.1;
	}
```

## Example systemctl file
in: `/etc/systemd/system/partywipe.service`
```
[Unit]
Description=Partywipe!

[Service]
ExecStart=/home/ubuntu/partywipe/a.out

[Install]
WantedBy=multi-user.target
```
systemd commands:
- See the status:
`sudo systemctl status partywipe`
- Configure whether or not it runs on startup:
`sudo systemctl enable/disable partywipe`
- Configure whether or not it's currently running:
`sudo systemctl start/stop partywipe`
