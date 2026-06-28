# Rileviamo automaticamente il sistema operativo
UNAME_S := $(shell uname -s)

# Configurazione dei flag tramite pkg-config (valido per Linux e Mac Intel)
CFLAGS = -W -Wall -O2 $(shell pkg-config --cflags openssl sdl3 libavcodec libavformat libavutil)
LDFLAGS = $(shell pkg-config --libs openssl sdl3 libavcodec libavformat libavutil)

# Correzione specifica per Mac Apple Silicon (M1/M2/M3)
ifeq ($(UNAME_S),Darwin)
    ifneq ($(wildcard /opt/homebrew/bin/brew),)
        # Se siamo su M1, forziamo pkg-config a guardare nella cartella di Homebrew ARM64
        PKG_CONFIG_PATH = /opt/homebrew/opt/openssl@3/lib/pkgconfig:/opt/homebrew/lib/pkgconfig
        CFLAGS = -W -Wall -O2 $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags openssl sdl3 libavcodec libavformat libavutil)
        LDFLAGS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs openssl sdl3 libavcodec libavformat libavutil)
    endif
endif

# Nome dell'eseguibile finale
TARGET = underground_radio

all: $(TARGET)

$(TARGET): client_http.c
	cc $(CFLAGS) client_http.c $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)