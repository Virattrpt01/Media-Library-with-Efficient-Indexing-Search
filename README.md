# Media Library with Efficient Indexing and Search

A PPS practical project written in C.

## Project features

- Stores media ID, title, artist/creator, genre, year, and file path
- Uses structures and dynamic memory
- Uses linked lists
- Uses hash-table-style indexes for artist, genre, and title keywords
- Add, edit, and delete media items
- Import media records from a CSV file
- Exact artist and genre search
- Title keyword and substring search
- Sort media by title or year
- Paginated display of search results
- Saves data in `media_library_data.csv`

## Files

- `media_library.c` — Main C program
- `sample_media.csv` — Sample records for CSV import

## How to run

Compile:

```bash
gcc -std=c11 -Wall -Wextra media_library.c -o media_library