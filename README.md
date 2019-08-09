# play-with-docker
## nginx-beginner
Docker for nginx [beginner guide](http://nginx.org/en/docs/beginners_guide.html)
## nginx-node
Docker network practice. 2 upstream servers.
## my-docker
A simple version of docker from [creating your own container](http://cesarvr.github.io/post/2018-05-22-create-containers/). can only run alphine. have some error with dns.
### usage
```bash
> g++ main.cpp -o docker -l curl -std=c++11
> sudo ./docker
```
### trouble shooting
- if face error in apk update, may because of DNS error, try
```bash
echo "151.101.112.249 dl-cdn.alpinelinux.org" >> /etc/hosts
```

