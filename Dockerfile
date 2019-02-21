FROM alpine:3.9

RUN apk update && apk add gcc make musl-dev libc-dev

EXPOSE 50500

WORKDIR /router
COPY ./src /router
RUN make router

#CMD ["./router", "config"]
CMD ["./router"]
