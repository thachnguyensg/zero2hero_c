TARGET_SRV = bin/dbserver
TARGET_CLI = bin/dbclient

SRC_SRV = $(wildcard src/srv/*.c)
OBJ_SRV = $(patsubst src/srv/%.c, obj/srv/%.o, $(SRC_SRV))

SRC_CLI = $(wildcard src/cli/*.c)
OBJ_CLI = $(patsubst src/cli/%.c, obj/cli/%.o, $(SRC_CLI))


CFLAGS = -g -Iinclude

run: clean default
	./$(TARGET_SRV) -f ./newdb.db -n -p 8080

default: $(TARGET_SRV) $(TARGET_CLI)

clean:
	rm -f obj/srv/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET_SRV): $(OBJ_SRV)
	gcc -o $@ $?

$(OBJ_SRV): obj/srv/%.o: src/srv/%.c
	@mkdir -p obj/srv
	gcc $(CFLAGS) -c $< -o $@

$(TARGET_CLI): $(OBJ_CLI)
	gcc -o $@ $?

$(OBJ_CLI): obj/cli/%.o: src/cli/%.c
	@mkdir -p obj/cli
	gcc $(CFLAGS) -c $< -o $@
