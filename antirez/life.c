#include <stdio.h>
#include <unistd.h>

#define GRID_COLS 20
#define GRID_ROWS 20
#define GRID_CELLS (GRID_COLS * GRID_ROWS)
#define ALIVE '*'
#define DEAD '.'
/* Translate the specified x, y grid point into index in the linear array. This function implements wrapping, so both negative and positive coordinates that are out of the grid will wrap around. */
int cell_to_index(int x, int y)
{
    int newX = x;
    int newY = y;
    if (newX < 0)
    {
        if ((-newX) < GRID_COLS)
        {
            newX = GRID_COLS + newX;
        }
        else
        {
            newX = GRID_COLS - ((-newX) % GRID_COLS);
        }
    }
    if (newY < 0)
    {
        if ((-newY) < GRID_ROWS)
        {
            newY = GRID_ROWS + newY;
        }
        else
        {
            newY = GRID_ROWS - ((-newY) % GRID_ROWS);
        }
    }
    if (newX >= GRID_COLS)
    {
        newX = x % GRID_COLS;
    }
    if (newY >= GRID_ROWS)
    {
        newY = y % GRID_ROWS;
    }
    // printf("%d, %d", newX, newY);
    // printf("\n");
    return newX + GRID_COLS * newY;
}
/* The function sets the specified cell at x, y to the specified state. */
void set_cell(char *grid, int x, int y, char state)
{
    grid[cell_to_index(x, y)] = state;
    return;
}
/* The function returns the state of the grid at x, y. */
char get_cell(char *grid, int x, int y)
{
    return grid[cell_to_index(x, y)];
}

void init_grid(char *grid, char state)
{
    for (int i = 0; i < GRID_ROWS; i++)
    {
        for (int j = 0; j < GRID_COLS; j++)
        {
            set_cell(grid, j, i, state);
        }
    }
    return;
}
void print_grid(char *grid)
{
    printf("\x1b[3J\x1b[H\x1b[2J");
    for (int i = 0; i < GRID_ROWS; i++)
    {
        for (int j = 0; j < GRID_COLS; j++)
        {
            printf("%c", get_cell(grid, j, i));
        }
        printf("\n");
    }
    return;
}
/* Return the number of living cells neighbors of x, y */
int count_living_neighbors(char *grid, int x, int y)
{
    int count = 0;
    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            if (i == 0 && j == 0)
                continue;
            if (get_cell(grid, x + j, y + i) == ALIVE)
            {
                count++;
            }
        }
    }
    return count;
}
/* Compute the new state of Game of Life according to its rules. */
void compute_new_state(char *old, char *new)
{
    for (int i = 0; i < GRID_ROWS; i++)
    {
        for (int j = 0; j < GRID_COLS; j++)
        {
            int neighbors = count_living_neighbors(old, j, i);
            if (get_cell(old, j, i) == ALIVE)
            {
                if (neighbors == 2 || neighbors == 3)
                {
                    set_cell(new, j, i, ALIVE);
                }
                else
                {
                    set_cell(new, j, i, DEAD);
                }
            }
            else
            {
                if (neighbors == 3)
                {
                    set_cell(new, j, i, ALIVE);
                }
                else
                {
                    set_cell(new, j, i, DEAD);
                }
            }
        }
    }
}

int main(void)
{
    char old_grid[GRID_CELLS];
    char new_grid[GRID_CELLS];
    init_grid(old_grid, DEAD);
    set_cell(old_grid, 10, 10, ALIVE);
    set_cell(old_grid, 9, 10, ALIVE);
    set_cell(old_grid, 11, 10, ALIVE);
    set_cell(old_grid, 11, 9, ALIVE);
    set_cell(old_grid, 10, 8, ALIVE);
    char *old = old_grid;
    char *new = new_grid;
    char *tmp = NULL;
    while (1)
    {
        compute_new_state(old, new);
        print_grid(new);
        usleep(100000);
        tmp = old;
        old = new;
        new = tmp;
        // compute_new_state(new_grid, old_grid);
        // print_grid(old_grid);
        // usleep(100000);
    }
    // printf("%d\n", count_living_neighbors(old_grid, 10, 10));
    // cell_to_index(-3, 0);
    // cell_to_index(0, -27);
    return 0;
}
