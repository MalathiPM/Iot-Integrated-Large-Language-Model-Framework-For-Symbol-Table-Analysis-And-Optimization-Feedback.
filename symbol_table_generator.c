#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Symbol {
    char name[30];
    char type[20];
    char scope[10];
    char additionalInfo[50];
    int size;
};

int isDataType(char* word) {
    return (strcmp(word, "int") == 0 || strcmp(word, "float") == 0 ||
            strcmp(word, "char") == 0 || strcmp(word, "double") == 0);
}

int getTypeSize(char* type) {
    if (strcmp(type, "int") == 0) return sizeof(int);
    if (strcmp(type, "float") == 0) return sizeof(float);
    if (strcmp(type, "double") == 0) return sizeof(double);
    if (strcmp(type, "char") == 0) return sizeof(char);
    return 0;
}

int skipBlockComments(FILE *fp, char *word) {
    if (strstr(word, "/*")) {
        while (fscanf(fp, "%s", word) != EOF) {
            if (strstr(word, "*/")) break;
        }
        return 1;
    }
    return 0;
}

int skipLineComments(FILE *fp, char *word) {
    if (strstr(word, "//")) {
        fscanf(fp, "%*[^\n]");
        return 1;
    }
    return 0;
}

int detectConstant(char* word) {
    return strcmp(word, "const") == 0;
}

void addSymbol(struct Symbol *table, int *count, char *name, char *type, char *scope, int isConst, const char* addInfo) {
    strcpy(table[*count].name, name);
    strcpy(table[*count].type, type);
    strcpy(table[*count].scope, scope);
    table[*count].size = getTypeSize(type);
    if (isConst)
        strcpy(table[*count].additionalInfo, "const variable");
    else if (addInfo != NULL)
        strcpy(table[*count].additionalInfo, addInfo);
    else
        strcpy(table[*count].additionalInfo, "variable");
    (*count)++;
}

void writeJSON(struct Symbol *table, int count, const char *jsonFilename) {
    FILE *jf = fopen(jsonFilename, "w");
    if (!jf) {
        printf("Failed to open JSON file for writing.\n");
        return;
    }
    fprintf(jf, "[\n");
    for (int i = 0; i < count; i++) {
        fprintf(jf, "  {\n");
        fprintf(jf, "    \"name\": \"%s\",\n", table[i].name);
        fprintf(jf, "    \"type\": \"%s\",\n", table[i].type);
        fprintf(jf, "    \"scope\": \"%s\",\n", table[i].scope);
        fprintf(jf, "    \"additionalInfo\": \"%s\",\n", table[i].additionalInfo);
        fprintf(jf, "    \"size\": %d\n", table[i].size);
        fprintf(jf, "  }%s\n", (i == count - 1) ? "" : ",");
    }
    fprintf(jf, "]\n");
    fclose(jf);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <source_code_file>\n", argv[0]);
        return 1;
    }
    char *filename = argv[1];
    char word[200], prevType[20] = "";
    struct Symbol table[100];
    int count = 0, isConst = 0;
    int braceDepth = 0, inFunction = 0;

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("File not found: %s\n", filename);
        return 1;
    }

    while (fscanf(fp, "%s", word) != EOF) {
        if (skipBlockComments(fp, word)) continue;
        if (skipLineComments(fp, word)) continue;

        if (detectConstant(word)) {
            isConst = 1;
            continue;
        }

        if (strchr(word, '{')) {
            braceDepth++;
            continue;
        }
        if (strchr(word, '}')) {
            braceDepth--;
            if (braceDepth == 0)
                inFunction = 0;
            continue;
        }

        if (isDataType(prevType) && strchr(word, '(')) {
            char funcName[50] = "";
            char params[200] = "";
            char *openParen = strchr(word, '(');
            int pos = openParen - word;
            strncpy(funcName, word, pos);
            funcName[pos] = '\0';
            strcpy(params, openParen + 1);

            char temp[200];
            while (strchr(params, ')') == NULL && fscanf(fp, "%s", temp) != EOF) {
                strcat(params, " ");
                strcat(params, temp);
            }

            char *closeParen = strchr(params, ')');
            if (closeParen) *closeParen = '\0';

            addSymbol(table, &count, funcName, prevType, "global", 0, "function");

            char *paramToken = strtok(params, ",");
            while (paramToken) {
                char paramType[20] = "", paramName[30] = "";
                sscanf(paramToken, "%s %s", paramType, paramName);
                if (isDataType(paramType) && strlen(paramName) > 0) {
                    addSymbol(table, &count, paramName, paramType, "local", 0, NULL);
                }
                paramToken = strtok(NULL, ",");
            }
            inFunction = 1;
            prevType[0] = '\0';
            isConst = 0;
            continue;
        }

        if (isDataType(word)) {
            strcpy(prevType, word);
            continue;
        }

        if (strlen(prevType) > 0) {
            char *token = strtok(word, ",;=");
            while (token) {
                char scope[10];
                if (inFunction && braceDepth > 0)
                    strcpy(scope, "local");
                else
                    strcpy(scope, "global");
                addSymbol(table, &count, token, prevType, scope, isConst, NULL);
                token = strtok(NULL, ",;=");
            }
            prevType[0] = '\0';
            isConst = 0;
        }
    }

    fclose(fp);

    writeJSON(table, count, "symbol_table.json");
    return 0;
}
