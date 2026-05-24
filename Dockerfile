FROM debian:stable-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        python3 \
        nasm \
        unzip \
	qemu-system-x86 \
        wget \
	websockify \
	novnc \
	ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /baos

RUN wget -O i686-elf-tools-linux.zip \
	https://github.com/lordmilko/i686-elf-tools/releases/download/13.2.0/i686-elf-tools-linux.zip && \
	unzip i686-elf-tools-linux.zip -d /opt/i686-elf && \
	rm i686-elf-tools-linux.zip

ENV PATH="/opt/i686-elf/bin:${PATH}"

COPY . .

EXPOSE 6080

ENTRYPOINT ["sh", "-c", "\
  ln -sf /usr/share/novnc/vnc.html /usr/share/novnc/index.html && \
  websockify --web=/usr/share/novnc 6080 localhost:5900 & \
  make run \
"]