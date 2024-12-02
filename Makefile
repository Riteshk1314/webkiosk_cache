CC = gcc
CFLAGS = -Wall -Wextra -I./includes
LIBS = -lcurl
SRC = main.c includes/dotenv.c
OUT = main
all: $(OUT)
$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LIBS)
clean:
	rm -f $(OUT)
