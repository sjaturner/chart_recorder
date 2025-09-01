#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
    MAX_COLS = 2048,
    BRAILLE_CELL_HEIGHT = 4,
    BRAILLE_CELL_WIDTH = 2,

    BRAILLE_STORED_LINES = 2,
    DEPTH = BRAILLE_STORED_LINES * BRAILLE_CELL_HEIGHT,
    MAX_TRACES = 10,
#define NAME_LENGTH_FMT "10" /* At most that should be NAME_LENGTH - 1 */
    NAME_LENGTH = 20, /* Bigger than NAME_LENGTH_FMT. */
    BRAILLE_CHARS = 3,
};

enum
{
    BRAILLE_MODE,
    CHARACTER_MODE,
};

int mode = BRAILLE_MODE;

enum
{
    BRAILLE_BUILD_PIXEL,
    BRAILLE_BUILD_LINE,
};

int braille_build = BRAILLE_BUILD_PIXEL;

enum
{
    DRAWING_MODE_LINES,
    DRAWING_MODE_DOTS,
};

int drawing_mode = DRAWING_MODE_LINES;

int max_value = -1;

char arena[DEPTH * MAX_COLS] = { };

void arena_set(int row, int col, char chr)
{
    if (row >= 0 && row < DEPTH && col >= 0 && col < MAX_COLS)
    {
        arena[row * MAX_COLS + col] = chr;
    }
}

char arena_get(int row, int col)
{
    if (row >= 0 && row < DEPTH && col >= 0 && col < MAX_COLS)
    {
        return arena[row * MAX_COLS + col];
    }
    else
    {
        return 0;
    }
}

int filler(char cell)
{
    return cell == '#' || cell == '%' || cell == ' ';
}

int values[DEPTH][MAX_TRACES] = {[0 ... DEPTH - 1][0 ... MAX_TRACES - 1] = -1, };

int debug_quiet(const char *fmt, ...)
{
    (void)fmt;

    return 0;
}

typeof(printf) * debug_printf = debug_quiet;

void dump_arena(void)
{
    for (int row = 0; row < DEPTH; ++row)
    {
        for (int col = 0; col <= max_value; ++col)
        {
            debug_printf("%c", arena_get(row, col));
        }
        debug_printf(":%03d", row);

        debug_printf("\n");
    }
}

struct trace
{
    int active;
    char name[NAME_LENGTH];
    int pace;
    unsigned long last_label_line_count_hold;
    unsigned long last_label_line_count;
};

struct trace traces[MAX_TRACES] = { };

void update_last_label_line_count(void)
{
    for (int trace_index = 0; trace_index < MAX_TRACES; ++trace_index)
    {
        struct trace *trace = traces + trace_index;

        if (trace->active)
        {
            trace->last_label_line_count = trace->last_label_line_count_hold;
        }
        else
        {
            trace->last_label_line_count = trace->last_label_line_count_hold = 0;
        }
    }
}

int width = 0;
char leader[NAME_LENGTH] = { };
char tailer[NAME_LENGTH] = { };

int make_braille_cells(char braille[][BRAILLE_CHARS], int row_index)
{
    int horizontal_cells = (max_value + BRAILLE_CELL_WIDTH - 1) / BRAILLE_CELL_WIDTH;

    int horizontal_cell = 0;
    for (horizontal_cell = 0; horizontal_cell <= horizontal_cells; ++horizontal_cell)
    {
        memset(braille[horizontal_cell], 0, sizeof(braille[horizontal_cell]));

        int top_left_x = horizontal_cell * BRAILLE_CELL_WIDTH;
        int top_left_y = row_index;

        struct
        {
            int x;
            int y;
        } offset[] = {
            {0, 0},
            {0, 1},
            {0, 2},
            {0, 3},
            {1, 0},
            {1, 1},
            {1, 2},
            {1, 3},
        };

        unsigned int cell = 0;
        for (unsigned int bit = 0; bit < sizeof(offset) / sizeof(offset[0]); ++bit)
        {
            int arena_x = top_left_x + offset[bit].x;
            int arena_y = top_left_y + offset[bit].y;

            cell |= !filler(arena_get((arena_y % DEPTH), arena_x)) << bit;
        }

        unsigned char mapped_cell = 0; /* See https://en.wikipedia.org/wiki/Braille_Patterns */

        mapped_cell |= cell >> 0 & 0x07;
        mapped_cell |= cell >> 1 & 0x38;
        mapped_cell |= cell << 3 & 0x40;
        mapped_cell |= cell << 0 & 0x80;

        if (!mapped_cell)
        {
            continue;
        }

        braille[horizontal_cell][0] = 0xe2;
        braille[horizontal_cell][1] = 0xa0 + (mapped_cell >> 6);
        braille[horizontal_cell][2] = 0x80 + (mapped_cell & 0x3f);
    }

    return horizontal_cell;
}

int make_labels(char *labels, unsigned long line_count, int row_index)
{
    int ret = 0;
//  line_count = mode == BRAILLE_MODE ? line_count * BRAILLE_CELL_HEIGHT : line_count;

    for (int trace_index = 0; trace_index < MAX_TRACES; ++trace_index)
    {
        struct trace *trace = traces + trace_index;

        if (!trace->active || !trace->name[0])
        {
            continue;
        }

        int offset = -1;
        int row_index_from = row_index + DEPTH;

        row_index_from -= mode == BRAILLE_MODE ? BRAILLE_CELL_HEIGHT : 0;
        row_index_from -= drawing_mode == DRAWING_MODE_DOTS ? 0 : 1;

        for (int index = row_index_from % DEPTH;; index = (index + 1) % DEPTH)
        {
            offset = offset < values[index][trace_index] ? values[index][trace_index] : offset;
            if (index == row_index)
            {
                break;
            }
        }

        if (!trace->pace || line_count >= trace->last_label_line_count + trace->pace)
        {
            trace->last_label_line_count_hold = line_count;

            if (offset < 0)
            {
                continue;
            }

            offset = mode == BRAILLE_MODE ? (offset + 1) / 2 : offset;

            int length = strlen(trace->name);
            int start = offset + 2;

            memcpy(labels + start, trace->name, length);
            int end = start + length;

            ret = ret < end ? end : ret;
        }
    }

    return ret;
}

int main(int argc, char *argv[])
{
    unsigned long line_count = 0;
    int row_index = 0;

    memset(arena, '%', sizeof arena);

    for (int opt = -1; (opt = getopt(argc, argv, "clvd")) != -1;)
    {
        switch (opt)
        {
            case 'c':
                mode = CHARACTER_MODE;
                break;
            case 'l':
                braille_build = BRAILLE_BUILD_LINE;
                break;
            case 'v':
                debug_printf = printf;
                break;
            case 'd':
                drawing_mode = DRAWING_MODE_DOTS;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    char input[0x400] = { };

    while (fgets(input, sizeof input - 1, stdin))
    {
        char *token = 0;
        int trace_index = 0;

        for (int index = 0; index < MAX_TRACES; ++index)
        {
            values[row_index][index] = -1;
        }

        for (;;)
        {
            if (!(token = strtok(token ? 0 : input, " \t\r\n")))
            {
                break;
            }

            if (1 == sscanf(token, "^%" NAME_LENGTH_FMT "s", leader))
            {
                debug_printf("leader:%s\n", leader);
            }
            else if (1 == sscanf(token, "$%" NAME_LENGTH_FMT "s", tailer))
            {
                debug_printf("tailer:%s\n", tailer);
            }
            else if (1 == sscanf(token, "*%d", &width))
            {
                debug_printf("width:%d\n", width);
                max_value = width;
            }
            else
            {
                int value = -1;
                char name[NAME_LENGTH] = { };
                int pace = 0;
                int scanned = sscanf(token, "%d:%" NAME_LENGTH_FMT "[^:]:%d", &value, name, &pace);

                switch (scanned)
                {
                    case 3:
                        traces[trace_index].pace = pace;
                        debug_printf("pace:%d\n", pace);
                    case 2:
                        memcpy(traces[trace_index].name, name, strlen(name));
                        debug_printf("name:%s\n", name);
                    case 1:
                        values[row_index][trace_index] = value;
                        traces[trace_index].active = 1;
                        debug_printf("trace_index:%d value:%d\n", trace_index, value);
                        ++trace_index;
                }
            }
        }

        int this_row_index = row_index;
        int last_row_index = (row_index + DEPTH - 1) % DEPTH;

        for (int trace_index = 0; trace_index < MAX_TRACES; ++trace_index)
        {
            if (traces[trace_index].active)
            {
                int value = values[this_row_index][trace_index];
                if (value < MAX_COLS)
                {
                    if (value > max_value)
                    {
                        max_value = value;
                    }

                    if (value >= 0)
                    {
                        arena_set(this_row_index, value, 'a' + trace_index);
                        values[this_row_index][trace_index] = value;

                        if (drawing_mode == DRAWING_MODE_LINES && values[last_row_index][trace_index] > 0)
                        {
                            if (values[this_row_index][trace_index] > values[last_row_index][trace_index] + 1)
                            {
                                for (int col = values[last_row_index][trace_index] + 1; col < values[this_row_index][trace_index]; ++col)
                                {
                                    arena_set(this_row_index, col, 'a' + trace_index);
                                }
                            }
                            else if (values[last_row_index][trace_index] > values[this_row_index][trace_index] + 1)
                            {
                                for (int col = values[this_row_index][trace_index] + 1; col < values[last_row_index][trace_index]; ++col)
                                {
                                    arena_set(this_row_index, col, 'a' + trace_index);
                                }
                            }
                        }
                    }
                }
            }
        }

        memset(arena + (row_index + BRAILLE_CELL_HEIGHT) % DEPTH * MAX_COLS, '#', MAX_COLS);

        dump_arena();

        if (mode == CHARACTER_MODE)
        {
            if (leader[0])
            {
                printf("%s|", leader);
            }

            char labels[MAX_COLS + NAME_LENGTH];
            memset(labels, ' ', sizeof labels);
            int labels_length = make_labels(labels, line_count, row_index);

            max_value = max_value < labels_length ? labels_length : max_value;

            char data_line[MAX_COLS + 1] = { };
            int index = 0;
            for (index = 0; index <= max_value; ++index)
            {
                char cell = arena_get(row_index, index);
                data_line[index] = filler(cell) ? labels[index] : '*';
            }

            printf("%s", data_line);

            static int max_cols = -1;
            max_cols = max_cols < index ? index : max_cols;

            for (; index < max_cols; ++index)
            {
                printf(" ");
            }

            if (tailer[0])
            {
                printf("|%s", tailer);
            }
            printf("\n");
            update_last_label_line_count();
        }
        else
        {
            char braille_cells[MAX_COLS / BRAILLE_CELL_WIDTH][BRAILLE_CHARS];
            int output = 0;

            if (braille_build == BRAILLE_BUILD_LINE)
            {
                output = line_count && !((line_count + 1) % BRAILLE_CELL_HEIGHT);
            }
            else
            {
                output = 1;
                if (line_count % BRAILLE_CELL_HEIGHT)
                {
                    printf("\r");
                }
                else
                {
                    printf("\n");
                    update_last_label_line_count();
                }
            }

            if (output)
            {
                if (leader[0])
                {
                    printf("%s|", leader);
                }

                int from_row = (row_index / BRAILLE_CELL_HEIGHT * BRAILLE_CELL_HEIGHT) % DEPTH;
                int cells = make_braille_cells(braille_cells, from_row);

                char labels[MAX_COLS + NAME_LENGTH] = {[0 ... MAX_COLS + NAME_LENGTH - 1] = ' ' };
                memset(labels, ' ', sizeof labels);
                int labels_length = make_labels(labels, line_count, row_index);

                int cols = cells < labels_length ? labels_length : cells;

                static int max_cols = -1;
                max_cols = max_cols < cols ? cols : max_cols;

                int index = 0;
                for (index = 0; index < cols; ++index)
                {
                    if (index < cells && braille_cells[index][0])
                    {
                        for (int offset = 0; offset < BRAILLE_CHARS; ++offset)
                        {
                            printf("%c", braille_cells[index][offset]);
                        }
                    }
                    else
                    {
                        printf("%c", labels[index]);
                    }
                }

                for (; index < max_cols; ++index)
                {
                    printf(" ");
                }

                if (tailer[0])
                {
                    printf("|%s", tailer);
                }

                if (braille_build == BRAILLE_BUILD_LINE)
                {
                    printf("\n");
                    update_last_label_line_count();
                }
            }

            fflush(stdout);
        }

        memset(input, 0, sizeof input);
        ++line_count;

        row_index = (row_index + 1) % DEPTH;
    }
    printf("\n");
    fflush(stdout);

    return 0;
}
