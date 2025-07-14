#include <stdio.h>
#include <unistd.h>

#define GRID_COLS 20
#define GRID_ROWS 20
#define GRID_CELLS (GRID_COLS * GRID_ROWS)
#define ALIVE '*'
#define DEAD '.'
/*

   0 1 2
0  * * *
1  * * *
2  * * *

int index = row * 3 + column

*/
int cell_to_index(int r, int c)
{
    if (c >= GRID_COLS)
    {
        c = c % GRID_COLS;
    }
    if (r >= GRID_COLS)
    {
        r = r % GRID_ROWS;
    }
    if (c < 0)
    {
        c = GRID_COLS - (-c % GRID_COLS);
    }
    if (r < 0)
    {
        r = GRID_ROWS - (-r % GRID_ROWS);
    }
    return (r * GRID_COLS + c);
}
char getCell(char *grid, int r, int c)
{
    return grid[cell_to_index(r, c)];
}

void setCell(char *grid, int r, int c, char state)
{
    grid[cell_to_index(r, c)] = state;
}
void print_grid(char *grid)
{
    printf("\x1b[3J\x1b[H\x1b[2J");
    for (char row = 0; row < GRID_ROWS; row++)
    {
        for (char column = 0; column < GRID_COLS; column++)
        {
            printf("%c", getCell(grid, row, column));
        }
        printf("\n");
    }
}
void set_grid(char *grid, char state)
{
    for (char row = 0; row < GRID_ROWS; row++)
    {
        for (char column = 0; column < GRID_COLS; column++)
        {
            setCell(grid, row, column, state);
        }
    }
}
int count_living_neighbors(char *grid, char r, char c)
{
    int num_alive = 0;
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            // if (r + i < 0 || c + j < 0)
            //     continue;
            if (i == 0 && j == 0)
                continue;
            if (getCell(grid, r + i, c + j) == ALIVE)
            {
                num_alive++;
            }
        }
    }
    return num_alive;
}
void compute_new_state(char *old, char *new)
{
    for (char row = 0; row < GRID_ROWS; row++)
    {
        for (char column = 0; column < GRID_COLS; column++)
        {
            int n_alive = count_living_neighbors(old, row, column);
            int new_state = DEAD;
            if (getCell(old, row, column) == ALIVE)
            {
                if (n_alive == 2 || n_alive == 3)
                {
                    new_state = ALIVE;
                }
            }
            else
            {
                if (n_alive == 3)
                {
                    new_state = ALIVE;
                }
            }
            setCell(new, row, column, new_state);
        }
    }
}

int main(void)
{
    char old_grid[GRID_CELLS];
    char new_grid[GRID_CELLS];
    set_grid(old_grid, DEAD);
    setCell(old_grid, 10, 10, ALIVE);
    setCell(old_grid, 10, 9, ALIVE);
    setCell(old_grid, 10, 11, ALIVE);
    setCell(old_grid, 9, 11, ALIVE);
    setCell(old_grid, 8, 10, ALIVE);
    while (1)
    {
        compute_new_state(old_grid, new_grid);
        print_grid(new_grid);
        usleep(100000);
        compute_new_state(new_grid, old_grid);
        print_grid(old_grid);
        usleep(100000);
    }
    return 0;
}