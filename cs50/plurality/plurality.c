#include <cs50.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

// Max number of candidates
#define MAX 9

// Candidates have name and vote count
typedef struct
{
    string name;
    int votes;
} candidate;



// Array of candidates
candidate candidates[MAX];

// Number of candidates
int candidate_count;

// Function prototypes
bool vote(string name);
void bubble_sort(candidate *arr, int length);
void print_winner(void);

int main(int argc, string argv[])
{
    // Check for invalid usage
    if (argc < 2)
    {
        printf("Usage: plurality [candidate ...]\n");
        return 1;
    }

    // Populate array of candidates
    candidate_count = argc - 1;
    if (candidate_count > MAX)
    {
        printf("Maximum number of candidates is %i\n", MAX);
        return 2;
    }
    for (int i = 0; i < candidate_count; i++)
    {
        candidates[i].name = argv[i + 1];
        candidates[i].votes = 0;
    }

    int voter_count = get_int("Number of voters: ");

    // Loop over all voters
    for (int i = 0; i < voter_count; i++)
    {
        string name = get_string("Vote: ");

        // Check for invalid vote
        if (!vote(name))
        {
            printf("Invalid vote.\n");
        }
    }

    // Display winner of election
    print_winner();
}

// Update vote totals given a new vote
bool vote(string name)
{
    for (int i = 0; i < candidate_count; i++)
    {
        if(strcasecmp(candidates[i].name, name) == 0)
        {
            candidates[i].votes++;
            return true;
        }

    }
    // TODO
    return false;
}

void bubble_sort(candidate *arr, int length)
{
    for (int i = 0; i < length - 1; i++)
    {
        for (int j = 0; j < length - 1 - i; j++)
        {
            if (arr[j].votes > arr[j + 1].votes)
            {
                candidate tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

// Print the winner (or winners) of the election
void print_winner(void)
{
    bubble_sort(candidates, candidate_count);
    for (int i = candidate_count - 1; i > - 1; i--)
    {

        if(i == candidate_count - 1)
        {
            printf("%s: %i\n", candidates[i].name, candidates[i].votes);

        }
        else
        {
            if(candidates[i].votes == candidates[i + 1].votes)
            {
                printf("%s: %i\n", candidates[i].name, candidates[i].votes);
            }
        }

    }
        // TODO

}
