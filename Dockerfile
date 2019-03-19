FROM archlinux/base

RUN pacman -Syu --noconfirm && pacman -S --noconfirm gcc base-devel

#EXPOSE 50500

WORKDIR /router
COPY ./src /router

WORKDIR /router/lib/igraph-0.7.1
RUN ./configure && make && make install

#COPY config.ini /router
WORKDIR /router
RUN make clean && make router

#CMD ["./router", "config"]
CMD ["./router"]
