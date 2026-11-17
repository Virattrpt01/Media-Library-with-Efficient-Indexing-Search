#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B 101
#define L 2048
#define DATA "media_library_data.csv"

typedef struct Media {
    int id, year;
    char *title, *artist, *genre, *path;
    struct Media *next;
} Media;

typedef struct IndexNode {
    Media *item;
    struct IndexNode *next;
} IndexNode;

typedef struct {
    Media *head;
    IndexNode *artist[B], *genre[B], *word[B];
    int count, nextId;
} Library;

static char *dupstr(const char *s) {
    char *p = malloc(strlen(s) + 1);
    if (!p) {
        perror("Memory error");
        exit(1);
    }
    return strcpy(p, s);
}

static void trim(char *s) {
    char *p = s, *e;

    while (isspace((unsigned char)*p)) p++;

    if (p != s)
        memmove(s, p, strlen(p) + 1);

    e = s + strlen(s);

    while (e > s && isspace((unsigned char)e[-1]))
        --e;

    *e = '\0';
}

static int equalIgnoreCase(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a++) !=
            tolower((unsigned char)*b++))
            return 0;
    }

    return *a == *b;
}

static int containsIgnoreCase(const char *text, const char *query) {
    size_t n = strlen(query), i;
    const char *p;

    if (!n) return 1;

    for (p = text; *p; p++) {
        for (i = 0;
             i < n && p[i] &&
             tolower((unsigned char)p[i]) ==
             tolower((unsigned char)query[i]);
             i++);

        if (i == n)
            return 1;
    }

    return 0;
}

static unsigned hash(const char *s) {
    unsigned h = 0;

    while (*s) {
        h = (h * 31 + (unsigned char)tolower((unsigned char)*s++)) % B;
    }

    return h;
}

static int validText(const char *s) {
    return *s && !strchr(s, ',') && !strchr(s, '"');
}

static int validYear(int year) {
    return year >= 1800 && year <= 2100;
}

static void addIndex(IndexNode **bucket, Media *media) {
    IndexNode *node = malloc(sizeof(*node));

    if (!node) {
        perror("Memory error");
        exit(1);
    }

    node->item = media;
    node->next = *bucket;
    *bucket = node;
}

static void clearIndexes(Library *library) {
    int i, j;

    for (i = 0; i < B; i++) {
        IndexNode *lists[3] = {
            library->artist[i],
            library->genre[i],
            library->word[i]
        };

        for (j = 0; j < 3; j++) {
            while (lists[j]) {
                IndexNode *old = lists[j];
                lists[j] = old->next;
                free(old);
            }
        }

        library->artist[i] = NULL;
        library->genre[i] = NULL;
        library->word[i] = NULL;
    }
}

static void indexTitleWords(Library *library, Media *media) {
    char word[128];
    int length = 0;
    const char *p;

    for (p = media->title;; p++) {
        if (isalnum((unsigned char)*p)) {
            if (length < 127) {
                word[length++] = (char)tolower((unsigned char)*p);
            }
        } else {
            if (length > 0) {
                word[length] = '\0';
                addIndex(&library->word[hash(word)], media);
                length = 0;
            }

            if (!*p)
                break;
        }
    }
}

static void rebuildIndexes(Library *library) {
    Media *media;

    clearIndexes(library);

    for (media = library->head; media; media = media->next) {
        addIndex(&library->artist[hash(media->artist)], media);
        addIndex(&library->genre[hash(media->genre)], media);
        indexTitleWords(library, media);
    }
}

static Media *findById(Library *library, int id) {
    Media *media;

    for (media = library->head; media; media = media->next) {
        if (media->id == id)
            return media;
    }

    return NULL;
}

static Media *appendMedia(
    Library *library,
    int id,
    const char *title,
    const char *artist,
    const char *genre,
    int year,
    const char *path
) {
    Media *media, **tail;

    if (!validText(title) || !validText(artist) ||
        !validText(genre) || !validText(path) ||
        !validYear(year)) {
        return NULL;
    }

    media = calloc(1, sizeof(*media));

    if (!media) {
        perror("Memory error");
        exit(1);
    }

    media->id = id;
    media->year = year;
    media->title = dupstr(title);
    media->artist = dupstr(artist);
    media->genre = dupstr(genre);
    media->path = dupstr(path);

    for (tail = &library->head; *tail; tail = &(*tail)->next);

    *tail = media;
    library->count++;

    if (id >= library->nextId)
        library->nextId = id + 1;

    return media;
}

static void input(const char *label, char *text, size_t size) {
    printf("%s", label);

    if (!fgets(text, (int)size, stdin))
        exit(0);

    text[strcspn(text, "\r\n")] = '\0';
    trim(text);
}

static int inputNumber(const char *label, int *number) {
    char text[80], extra;

    input(label, text, sizeof(text));

    return sscanf(text, "%d %c", number, &extra) == 1;
}

static void printMedia(const Media *media) {
    printf(
        "ID:%d | %s | %s | %s | %d | %s\n",
        media->id,
        media->title,
        media->artist,
        media->genre,
        media->year,
        media->path
    );
}

static Media **makeArray(Library *library, int *count) {
    Media **array;
    Media *media;
    int i = 0;

    array = malloc((size_t)library->count * sizeof(*array));

    if (!array && library->count)
        exit(1);

    for (media = library->head; media; media = media->next)
        array[i++] = media;

    *count = i;
    return array;
}

static void paginatedDisplay(Media **items, int count) {
    int start = 0, i;
    char answer[8];

    if (!count) {
        puts("No matching media.");
        return;
    }

    while (start < count) {
        printf(
            "\n--- Results %d to %d of %d ---\n",
            start + 1,
            start + 5 < count ? start + 5 : count,
            count
        );

        for (i = start; i < count && i < start + 5; i++)
            printMedia(items[i]);

        start += 5;

        if (start < count) {
            input("Press Enter for next page (q stops): ",
                  answer, sizeof(answer));

            if (tolower((unsigned char)answer[0]) == 'q')
                break;
        }
    }
}

static void addMedia(Library *library) {
    char title[L], artist[L], genre[L], path[L];
    int year;

    input("Title: ", title, sizeof(title));
    input("Artist/creator: ", artist, sizeof(artist));
    input("Genre: ", genre, sizeof(genre));

    if (!inputNumber("Year (1800-2100): ", &year)) {
        puts("Invalid year.");
        return;
    }

    input("File path: ", path, sizeof(path));

    if (!appendMedia(
            library,
            library->nextId,
            title, artist, genre, year, path
        )) {
        puts("Invalid data. Fields cannot be blank or contain commas.");
        return;
    }

    rebuildIndexes(library);
    printf("Added media with ID %d.\n", library->nextId - 1);
}

static void editMedia(Library *library) {
    int id, year;
    Media *media;
    char text[L];

    if (!inputNumber("ID to edit: ", &id) ||
        !(media = findById(library, id))) {
        puts("Media not found.");
        return;
    }

    puts("Leave a field blank to keep its current value.");

    input("Title: ", text, sizeof(text));
    if (*text) {
        if (!validText(text)) {
            puts("Invalid title.");
            return;
        }
        free(media->title);
        media->title = dupstr(text);
    }

    input("Artist: ", text, sizeof(text));
    if (*text) {
        if (!validText(text)) {
            puts("Invalid artist.");
            return;
        }
        free(media->artist);
        media->artist = dupstr(text);
    }

    input("Genre: ", text, sizeof(text));
    if (*text) {
        if (!validText(text)) {
            puts("Invalid genre.");
            return;
        }
        free(media->genre);
        media->genre = dupstr(text);
    }

    input("Year: ", text, sizeof(text));
    if (*text) {
        if (sscanf(text, "%d", &year) != 1 ||
            !validYear(year)) {
            puts("Invalid year.");
            return;
        }
        media->year = year;
    }

    input("File path: ", text, sizeof(text));
    if (*text) {
        if (!validText(text)) {
            puts("Invalid path.");
            return;
        }
        free(media->path);
        media->path = dupstr(text);
    }

    rebuildIndexes(library);
    puts("Updated.");
}

static void deleteMedia(Library *library) {
    int id;
    Media **link, *media;

    if (!inputNumber("ID to delete: ", &id)) {
        puts("Invalid ID.");
        return;
    }

    for (link = &library->head;
         *link && (*link)->id != id;
         link = &(*link)->next);

    media = *link;

    if (!media) {
        puts("Media not found.");
        return;
    }

    *link = media->next;

    free(media->title);
    free(media->artist);
    free(media->genre);
    free(media->path);
    free(media);

    library->count--;
    rebuildIndexes(library);

    puts("Deleted.");
}

/* CSV format: id,title,artist,genre,year,path */
static int parseCSV(char *line, char **fields) {
    int count = 0;
    char *p = line;

    trim(p);

    while (count < 6) {
        fields[count++] = p;
        p = strchr(p, ',');

        if (!p)
            break;

        *p++ = '\0';
    }

    return count;
}

static int importFile(Library *library, const char *name, int report) {
    FILE *file = fopen(name, "r");
    char line[L], *fields[6];
    int first = 1, imported = 0, skipped = 0;

    if (!file) {
        if (report)
            perror("Cannot open CSV file");
        return -1;
    }

    while (fgets(line, sizeof(line), file)) {
        int count = parseCSV(line, fields);
        int i, id, year;

        for (i = 0; i < count; i++)
            trim(fields[i]);

        if (first && count && equalIgnoreCase(fields[0], "id")) {
            first = 0;
            continue;
        }

        first = 0;

        if (count != 6 ||
            sscanf(fields[0], "%d", &id) != 1 ||
            sscanf(fields[4], "%d", &year) != 1 ||
            findById(library, id) ||
            !appendMedia(
                library, id,
                fields[1], fields[2], fields[3],
                year, fields[5]
            )) {
            skipped++;
        } else {
            imported++;
        }
    }

    fclose(file);
    rebuildIndexes(library);

    if (report) {
        printf(
            "Imported %d item(s); skipped %d invalid/duplicate line(s).\n",
            imported, skipped
        );
    }

    return imported;
}

static void importCSV(Library *library) {
    char fileName[L];

    input("CSV file name/path: ", fileName, sizeof(fileName));
    importFile(library, fileName, 1);
}

static int compareTitle(const void *x, const void *y) {
    return strcmp((*(Media **)x)->title, (*(Media **)y)->title);
}

static int compareYear(const void *x, const void *y) {
    Media *a = *(Media **)x;
    Media *b = *(Media **)y;

    if (a->year != b->year)
        return a->year - b->year;

    return strcmp(a->title, b->title);
}

static void sortedView(Library *library, int byYear) {
    int count;
    Media **items = makeArray(library, &count);

    qsort(
        items,
        (size_t)count,
        sizeof(*items),
        byYear ? compareYear : compareTitle
    );

    paginatedDisplay(items, count);
    free(items);
}

static void indexedSearch(Library *library, int type) {
    char query[L];
    IndexNode *node;
    Media **matches;
    int used = 0, i;

    if (type == 0)
        input("Exact artist: ", query, sizeof(query));
    else if (type == 1)
        input("Exact genre: ", query, sizeof(query));
    else
        input("Title keyword: ", query, sizeof(query));

    if (!*query) {
        puts("Search cannot be blank.");
        return;
    }

    matches = malloc((size_t)library->count * sizeof(*matches));

    if (type == 0)
        node = library->artist[hash(query)];
    else if (type == 1)
        node = library->genre[hash(query)];
    else
        node = library->word[hash(query)];

    while (node) {
        Media *media = node->item;
        int match;

        if (type == 0)
            match = equalIgnoreCase(media->artist, query);
        else if (type == 1)
            match = equalIgnoreCase(media->genre, query);
        else
            match = containsIgnoreCase(media->title, query);

        for (i = 0; i < used && matches[i] != media; i++);

        if (match && i == used)
            matches[used++] = media;

        node = node->next;
    }

    paginatedDisplay(matches, used);
    free(matches);
}

static void substringSearch(Library *library) {
    char query[L];
    Media *media, **matches;
    int count = 0;

    input("Substring (title, artist, or genre): ",
          query, sizeof(query));

    matches = malloc((size_t)library->count * sizeof(*matches));

    for (media = library->head; media; media = media->next) {
        if (containsIgnoreCase(media->title, query) ||
            containsIgnoreCase(media->artist, query) ||
            containsIgnoreCase(media->genre, query)) {
            matches[count++] = media;
        }
    }

    paginatedDisplay(matches, count);
    free(matches);
}

static void saveData(Library *library) {
    FILE *file = fopen(DATA, "w");
    Media *media;

    if (!file) {
        perror("Cannot save data");
        return;
    }

    fputs("id,title,artist,genre,year,path\n", file);

    for (media = library->head; media; media = media->next) {
        fprintf(
            file, "%d,%s,%s,%s,%d,%s\n",
            media->id,
            media->title,
            media->artist,
            media->genre,
            media->year,
            media->path
        );
    }

    fclose(file);
    puts("Saved successfully.");
}

static void freeLibrary(Library *library) {
    Media *media = library->head;

    clearIndexes(library);

    while (media) {
        Media *old = media;
        media = media->next;

        free(old->title);
        free(old->artist);
        free(old->genre);
        free(old->path);
        free(old);
    }
}

static void showMenu(void) {
    puts("\n===== MEDIA LIBRARY =====");
    puts("1. Add media");
    puts("2. Edit media");
    puts("3. Delete media");
    puts("4. Import CSV");
    puts("5. Exact artist search");
    puts("6. Exact genre search");
    puts("7. Title keyword search");
    puts("8. Substring search");
    puts("9. View sorted by title");
    puts("10. View sorted by year");
    puts("11. Save data");
    puts("0. Exit");
}

int main(void) {
    Library library = {0};
    int choice;

    library.nextId = 1;

    /* Automatically loads saved data if the file exists. */
    importFile(&library, DATA, 0);

    puts("Media Library with Efficient Indexing and Search");

    while (1) {
        showMenu();

        if (!inputNumber("Choose an option: ", &choice)) {
            puts("Please enter a number.");
            continue;
        }

        switch (choice) {
            case 1:
                addMedia(&library);
                break;

            case 2:
                editMedia(&library);
                break;

            case 3:
                deleteMedia(&library);
                break;

            case 4:
                importCSV(&library);
                break;

            case 5:
                indexedSearch(&library, 0);
                break;

            case 6:
                indexedSearch(&library, 1);
                break;

            case 7:
                indexedSearch(&library, 2);
                break;

            case 8:
                substringSearch(&library);
                break;

            case 9:
                sortedView(&library, 0);
                break;

            case 10:
                sortedView(&library, 1);
                break;

            case 11:
                saveData(&library);
                break;

            case 0:
                saveData(&library);
                freeLibrary(&library);
                puts("Goodbye.");
                return 0;

            default:
                puts("Invalid choice.");
        }
    }
}
