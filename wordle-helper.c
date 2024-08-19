#define _GNU_SOURCE
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define return_defer(value) \
    do {                    \
        result = (value);   \
        goto defer;         \
    } while (0)

#define ALPHABET_SIZE 26

typedef struct {
    char* items;
    size_t count;
    size_t capacity;
} string;

typedef struct {
    char* data;
    size_t length;
} StringViewer;

typedef struct {
    StringViewer* items;
    size_t count;
    size_t capacity;
} StringViewers;

typedef struct {
    bool lenPresent;
    size_t len;
    bool isBest;
    bool isAlpha;
    bool withLettersPresent;
    const char* withLetters;
    bool withoutLettersPresent;
    const char* withoutLetters;
    bool patternPresent;
    const char* pattern;
} Command;

const int letterFrequency[26] = {
    8, // a
    1, // b
    3, // c
    4, // d
    13, // e
    2, // f
    2, // g
    6, // h
    7, // i
    0, // j
    1, // k
    4, // l
    2, // m
    7, // n
    8, // o
    2, // p
    0, // q
    6, // r
    6, // s
    9, // t
    3, // u
    1, // v
    2, // w
    0, // x
    2, // y
    0 // z
};

// Function to calculate a "score" for a word based on letter frequency and position weighting
int calculateWordScore(const char* word, size_t length, float** weightings)
{
    float score = 0;
    int letterCount[26] = {0}; // Array to count occurrences of each letter

    for (size_t i = 0; i < length; i++) {
        char lowerChar = tolower(word[i]);
        if (lowerChar >= 'a' && lowerChar <= 'z') {
            int index = lowerChar - 'a';
            letterCount[index]++;
            score += letterFrequency[index] * weightings[index][i];
        }
    }

    // Apply penalty for multiple occurrences of the same letter
    for (int i = 0; i < 26; i++) {
        if (letterCount[i] > 1) {
            // Apply penalty (e.g., subtract a fixed amount for each extra occurrence)
            score -= (letterCount[i] - 1) * 2; // Penalty of 2 per extra occurrence
        }
    }

    return score;
}

// Comparison function for qsort
int compareWords(const void* a, const void* b, void* weightings)
{
    const StringViewer* sv1 = (const StringViewer*)a;
    const StringViewer* sv2 = (const StringViewer*)b;

    // Calculate scores for both words
    int score1 = calculateWordScore(sv1->data, sv1->length, (float**)weightings);
    int score2 = calculateWordScore(sv2->data, sv2->length, (float**)weightings);

    // Sort in descending order (higher score comes first)
    return score2 - score1;
}

void usage_error()
{
    fprintf(stderr, "Usage: wordle-helper [-alpha|-best] [-len len] [-with letters] [-without letters] [pattern]\n");
    exit(1);
}

void pattern_error(Command* command)
{
    printf("wordle-helper: pattern must be of length %zu and only contain underscores and/or letters", command->len);
    exit(2);
}

bool is_command_valid(int argc, char** argv, Command* command)
{
    if (argc > 9) {
        return false;
    }

    if (argc == 1) {
        return true;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-alpha") == 0) {
            if (command->isBest == true || command->isAlpha == true) {
                return false;
            }
            command->isAlpha = true;
            continue;
        }

        if (strcmp(argv[i], "-best") == 0) {
            if (command->isAlpha == true || command->isBest == true) {
                return false;
            }
            command->isBest = true;
            continue;
        }

        if (strcmp(argv[i], "-len") == 0) {
            if (i + 1 >= argc || command->lenPresent == true) {
                return false;
            }
            command->lenPresent = true;

            char* lenArg = argv[i + 1];
            for (size_t j = 0; j < strlen(lenArg); j++) {
                if (!isdigit(lenArg[j])) {
                    return false;
                }
            }
            command->len = (size_t)atoi(lenArg);
            i++;
            continue;
        }

        if (strcmp(argv[i], "-with") == 0) {
            if (i + 1 >= argc || command->withLettersPresent == true) {
                return false;
            }
            command->withLettersPresent = true;

            char* withLettersArg = argv[i + 1];

            // check if withLetter arg is blank, if so set withLettersPresent to false
            if (!strcmp(withLettersArg, "")) {
                command->withLettersPresent = false;
                i++;
                continue;
            }
            for (size_t j = 0; j < strlen(withLettersArg); j++) {
                if (!isalpha(withLettersArg[j])) {
                    return false;
                }
            }
            command->withLettersPresent = true;
            command->withLetters = withLettersArg;
            i++;
            continue;
        }

        if (strcmp(argv[i], "-without") == 0) {
            if (i + 1 >= argc || command->withoutLettersPresent == true) {
                return false;
            }
            command->withoutLettersPresent = true;

            char* withoutLettersArg = argv[i + 1];

            // check if withoutLetter arg is blank, if so set withoutLettersPresent to false
            if (!strcmp(withoutLettersArg, "")) {
                command->withoutLettersPresent = false;
                i++;
                continue;
            }
            for (size_t j = 0; j < strlen(withoutLettersArg); j++) {
                if (!isalpha(withoutLettersArg[j])) {
                    return false;
                }
            }
            command->withoutLettersPresent = true;
            command->withoutLetters = withoutLettersArg;
            i++;
            continue;
        }

        // cmd arg starts with '-' and not one of the above command->
        if (command->patternPresent == false && argv[i][0] != '-') {
            command->patternPresent = true;
            command->pattern = argv[i];
            continue;
        } else {
            return false;
        }
    }

    return true;
}

bool isunderscore(char character)
{
    if (character != '_') {
        return false;
    }
    return true;
}

bool is_valid_pattern(Command* command)
{
    if (command->len != strlen(command->pattern)) {
        return false;
    }

    if (!strcmp(command->pattern, "")) {
        command->patternPresent = false;
        return true;
    }

    for (size_t j = 0; j < strlen(command->pattern); j++) {
        if (!isalpha(command->pattern[j]) && !isunderscore(command->pattern[j])) {
            return false;
        }
    }

    return true;
}

void init_command(Command* command)
{
    command->len = 5;
    command->patternPresent = false;
    command->withoutLettersPresent = false;
    command->withLettersPresent = false;
    command->isAlpha = false;
    command->isBest = false;
    command->lenPresent = false;
    return;
}

size_t read_full_file(const char* file_path, string* content)
{
    FILE* file_ptr;
    bool result = true;

    file_ptr = fopen(file_path, "rb");
    if (file_ptr == NULL) {
        return_defer(false);
    }
    if (fseek(file_ptr, 0, SEEK_END) < 0) {
        return_defer(false);
    }
    long file_length = ftell(file_ptr);

    if (fseek(file_ptr, 0, SEEK_SET) < 0) {
        return_defer(false);
    }

    size_t new_count = content->count + file_length;
    if (new_count > content->capacity) {
        content->items = realloc(content->items, new_count);
        if (content->items == NULL) {
            printf("Error: Ran out of memory");
            return_defer(false);
        }
    }

    fread(content->items + content->count, file_length, 1, file_ptr);
    if (ferror(file_ptr)) {
        return_defer(false);
    }
    content->count = new_count;

defer:
    if (!result) {
        printf("Error: Could not read file %s:", file_path);
    }
    if (file_ptr) {
        fclose(file_ptr);
    }
    return result;
}

void trim_left(StringViewer* input)
{
    size_t i = 0;
    while (i < input->length && isspace(input->data[i])) {
        i++;
    }

    input->length -= i;
    input->data += i;

    return;
}

StringViewer split_word(StringViewer* input)
{
    volatile size_t i = 0;
    while (i < input->length && !isspace(input->data[i])) {
        i++;
    }

    StringViewer result = {
        .length = i,
        .data = input->data,
    };

    if (i < input->length) {
        input->length -= i + 1;
        input->data += i + 1;
    } else {
        input->length -= i;
        input->data += i;
    }

    return result;
}

void append_token(StringViewers* List, StringViewer* item)
{
    size_t new_capacity = List->capacity;

    if (List->count == 0) {
        new_capacity = 1;
        List->items = malloc(new_capacity * sizeof(*item));
        if (List->items == NULL) {
            printf("ERROR");
            exit(4);
        }
    } else if (List->count >= List->capacity) {
        new_capacity *= 2;
        List->items = realloc(List->items, new_capacity * sizeof(*item));
        if (List->items == NULL) {
            printf("ERR");
            exit(4);
        }
    }

    List->items[List->count] = *item;
    List->count += 1;
    List->capacity = new_capacity;

    return;
}

bool contains_withletters(StringViewer* token, const char* withLetters)
{
    int withLettersFreq[256] = { 0 };
    int stringLettersFreq[256] = { 0 };

    for (size_t i = 0; i < strlen(withLetters); i++) {
        withLettersFreq[(unsigned char)withLetters[i]]++;
    }

    for (size_t j = 0; j < token->length; j++) {
        stringLettersFreq[(unsigned char)token->data[j]]++;
    }

    for (size_t k = 0; k < 256; k++) {
        if (stringLettersFreq[k] < withLettersFreq[k]) {
            return false;
        }
    }

    return true;
}

bool contains_withoutLetters(StringViewer* token, const char* withoutLetters)
{
    for (size_t i = 0; i < token->length; i++) {
        for (size_t j = 0; j < strlen(withoutLetters); j++) {
            if ((char)tolower(token->data[i]) == (char)tolower(withoutLetters[j])) {
                return true;
            }
        }
    }
    return false;
}

bool matches_pattern(StringViewer* token, const char* pattern)
{
    if (token->length != strlen(pattern)) {
        return false;
    }

    for (size_t i = 0; i < strlen(pattern); i++) {
        if (pattern[i] == '_') {
            continue;
        } else if (pattern[i] != token->data[i]) {
            return false;
        }
    }
    return true;
}

void update_weightings(float** weightings, StringViewer* token)
{
    for (size_t i = 0; i < token->length; i++) {
        weightings[tolower(token->data[i]) - 'a'][i] += 1;
    }
}

void normalise_weightings(float** weightings, float count, size_t length)
{
    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
        for (size_t j = 0; j < length; j++) {
            weightings[i][j] /= count;
        }
    }
}

void free_weightings(float** weightings)
{
    if (weightings == NULL) {
        return;
    }
    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
        if (weightings[i] != NULL) {
            free(weightings[i]);
        }
    }
    free(weightings);

    return;
}

void populate_wordlist(StringViewers* wordList, StringViewer* content, Command* command, float** weightings)
{
    int weighting_words = 0;
    while (content->length > 0) {
        trim_left(content);
        StringViewer token = split_word(content);

        if (token.length != command->len) {
            continue;
        }

        weighting_words += 1;
        update_weightings(weightings, &token);

        if (command->withLettersPresent == true) {
            if (!contains_withletters(&token, command->withLetters)) {
                continue;
            }
        }

        if (command->withoutLettersPresent == true) {
            if (contains_withoutLetters(&token, command->withoutLetters)) {
                continue;
            }
        }

        if (command->patternPresent == true) {
            if (!matches_pattern(&token, command->pattern)) {
                continue;
            }
        }

        append_token(wordList, &token);
    }

    normalise_weightings(weightings, weighting_words, command->len);
}

static int alphabet_compare(const void* lhs, const void* rhs)
{
    const StringViewer* sv1 = (const StringViewer*)lhs;
    const StringViewer* sv2 = (const StringViewer*)rhs;

    for (size_t i = 0; i < sv1->length; i++) {
        char c1 = tolower(sv1->data[i]);
        char c2 = tolower(sv2->data[i]);

        if (c1 != c2) {
            return c1 - c2;
        }
    }
    return 0;
}

void sort_wordlist(StringViewers* wordList, Command* command, float** weightings)
{
    if (command->isBest) {
        qsort_r(wordList->items, wordList->count, sizeof(StringViewer), (int (*)(const void*, const void*, void*))compareWords, weightings);
    }

    if (command->isAlpha) {
        qsort(wordList->items, wordList->count, sizeof(StringViewer), alphabet_compare);
    }

    // If -alpha or -best are not specified, return dictionary order

    return;
}

void print_wordlist(StringViewers* wordList)
{
    for (size_t i = 0; i < wordList->count; i++) {
        if (i == 10) {
            exit(0);
        }
        for (size_t j = 0; j < wordList->items[i].length; j++) {
            printf("%c", wordList->items[i].data[j]);
        }
        printf("\n");
    }
}

float** init_weightings(size_t length)
{
    float** weightings = (float**)malloc(ALPHABET_SIZE * sizeof(float*));
    for (size_t i = 0; i < ALPHABET_SIZE; i++) {
        weightings[i] = (float*)calloc(length, sizeof(float));
    }
    return weightings;
}

void run_command(Command* command)
{
    char* file_path = "words.txt";
    string buffer = { 0 };

    if (!read_full_file(file_path, &buffer)) {
        exit(1);
    }

    StringViewer content = {
        .data = buffer.items,
        .length = buffer.count,
    };

    StringViewers wordList = { 0 };

    float** weightings = init_weightings(command->len);

    populate_wordlist(&wordList, &content, command, weightings);

    sort_wordlist(&wordList, command, weightings);
    print_wordlist(&wordList);

    free(buffer.items);
    free_weightings(weightings);
}

int main(int argc, char** argv)
{
    Command command;
    init_command(&command);

    if (!is_command_valid(argc, argv, &command)) {
        usage_error();
    }
    if (command.patternPresent == true) {
        if (!is_valid_pattern(&command)) {
            pattern_error(&command);
        }
    }

    run_command(&command);

    exit(0);

    /*return 0;*/
}
