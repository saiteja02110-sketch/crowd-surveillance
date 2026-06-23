FROM gcc:latest

WORKDIR /app

COPY index.html .
COPY server.c .

RUN gcc -o crowd_server server.c

EXPOSE 8080

CMD ["./crowd_server"]
