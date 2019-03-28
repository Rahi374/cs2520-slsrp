FROM alpine:3.9

RUN apk update && apk add gcc make musl-dev libc-dev curl-dev

#EXPOSE 50500

WORKDIR /router
COPY ./src /router
COPY ./configs /router/configs
#COPY config.ini /router
RUN make clean && make router

#CMD ["./router", "config"]
CMD ["./router"]
