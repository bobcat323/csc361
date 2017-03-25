(echo -e -n "GET /index.html HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &)  && \
(echo -e -n "GET / HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /gnu/main.html HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "get / http/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GeT / HtTp/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET http.request HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /index.html HTTP/1.1 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /gnu/ HTTP/1.1 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "got / http/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /nofile HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /gnu/nofile HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /Index.html HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /cantRead HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /../ HTTP/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &) && \
(echo -e -n "GET /a http/1.0 \r\n\r\n" | nc -u -s 192.168.1.100 10.10.1.100 8082 &)

