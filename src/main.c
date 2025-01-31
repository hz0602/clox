#include <stdio.h>
#include <stdlib.h>

#include "vm.h"
#include "hint.h"

// Wiriten by Roberts in clox.
static char* readFile(const char* path) {
	FILE* file = fopen(path, "rb");
	if(file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char* buffer = (char*)malloc(fileSize + 1);

	if(buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}

	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if(bytesRead < fileSize) {
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	}

    // If read '\0', which means reaching the end of buffer.
	buffer[bytesRead] = '\0';

	fclose(file);
	return buffer;
}

static void errorHint(PROCESS_RESULT res) {
    if(res == RUNTIME_ERROR) {
        redHint("Runtime Error.\n");
    } else if(res == COMPILE_ERROR) {
        redHint("Compile Error.\n");
    }
}

int main(int argc, char* argv[]) {
    if(argc == 1 || argc > 2) {
        return 1;
    }
    char* source = readFile(argv[1]);
    // char* source = NULL;
    initVM();
    PROCESS_RESULT res = interpret(source);
    freeVM();
    errorHint(res);
    return 0;
}
