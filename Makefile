CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
SAN_CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined
INC = -Imello
SRC = mello/mello_engine.c mello/mello_transport.c mello/mello_storage.c mello/mello_ui.c mello/mello_audio.c mello/mello_sample.c
OBJ = $(patsubst mello/%.c,build/%.o,$(SRC))
SAN_OBJ = $(patsubst mello/%.c,build/san_%.o,$(SRC))
TESTS = longform transport generative storage ui audio sample
.PHONY: all test sanitize clean
all: test
build:
	mkdir -p build
build/%.o: mello/%.c | build
	$(CC) $(CFLAGS) $(INC) -c $< -o $@
build/san_%.o: mello/%.c | build
	$(CC) $(SAN_CFLAGS) $(INC) -c $< -o $@
build/test_longform: $(OBJ) tests/test_mello_longform.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_mello_longform.c -o $@
build/test_transport: $(OBJ) tests/test_mello_transport_regression.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_mello_transport_regression.c -o $@
build/test_generative: $(OBJ) tests/test_mello_generative.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_mello_generative.c -o $@
build/test_storage: $(OBJ) tests/test_mello_storage.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_mello_storage.c -o $@
build/test_ui: $(OBJ) tests/test_mello_ui.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_mello_ui.c -o $@
build/test_audio: $(OBJ) tests/test_mello_audio.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_mello_audio.c -o $@
build/test_sample: $(OBJ) tests/test_mello_sample.c | build
	$(CC) $(CFLAGS) $(INC) $(OBJ) tests/test_mello_sample.c -o $@
test: $(addprefix build/test_,$(TESTS))
	@for t in $(addprefix build/test_,$(TESTS)); do $$t; done
build/san_%: $(SAN_OBJ) tests/test_mello_%.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_mello_$*.c -o $@
build/san_transport: $(SAN_OBJ) tests/test_mello_transport_regression.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_mello_transport_regression.c -o $@
build/san_generative: $(SAN_OBJ) tests/test_mello_generative.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_mello_generative.c -o $@
build/san_longform: $(SAN_OBJ) tests/test_mello_longform.c | build
	$(CC) $(SAN_CFLAGS) $(INC) $(SAN_OBJ) tests/test_mello_longform.c -o $@
sanitize: $(addprefix build/san_,$(TESTS))
	@for t in $(addprefix build/san_,$(TESTS)); do ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 $$t; done
clean:
	rm -rf build
