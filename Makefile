CC = gcc
CFLAGS = -Wall -Wextra -I./includes -I/usr/include/libmongoc-1.0 -I/usr/include/libbson-1.0
LIBS = -lcurl -lmongoc-1.0 -lbson-1.0
SRC = main.c db.c fetch_data.c includes/dotenv.c
OUT = main

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LIBS)

clean:
	rm -f $(OUT)
