#include <cs50.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

// Max voters and candidates
#define MAX_VOTERS 100
#define MAX_CANDIDATES 9

// preferences[i][j] is jth preference for voter i
int preferences[MAX_VOTERS][MAX_CANDIDATES];

// Candidates have name, vote count, eliminated status
typedef struct
{
    string name;
    int votes;
    bool eliminated;
} candidate;

// Array of candidates
candidate candidates[MAX_CANDIDATES];

// Numbers of voters and candidates
int voter_count;
int candidate_count;

// Function prototypes
bool vote(int voter, int rank, string name);
void tabulate(void);
bool print_winner(void);
int find_min(void);
bool is_tie(int min);
void eliminate(int min);
void bubble_sort(candidate *arr, int length);

int main(int argc, string argv[])
{
    // Check for invalid usage
    if (argc < 2)
    {
        printf("Usage: runoff [candidate ...]\n");
        return 1;
    }

    // Populate array of candidates
    candidate_count = argc - 1;
    if (candidate_count > MAX_CANDIDATES)
    {
        printf("Maximum number of candidates is %i\n", MAX_CANDIDATES);
        return 2;
    }
    for (int i = 0; i < candidate_count; i++)
    {
        candidates[i].name = argv[i + 1];
        candidates[i].votes = 0;
        candidates[i].eliminated = false;
    }

    voter_count = get_int("Number of voters: ");
    if (voter_count > MAX_VOTERS)
    {
        printf("Maximum number of voters is %i\n", MAX_VOTERS);
        return 3;
    }

    // Keep querying for votes
    for (int i = 0; i < voter_count; i++)
    {

        // Query for each rank
        for (int j = 0; j < candidate_count; j++)
        {
            string name = get_string("Rank %i: ", j + 1);

            // Record vote, unless it's invalid
            if (!vote(i, j, name))
            {
                printf("Invalid vote.\n");
                return 4;
            }

        }

        printf("\n");
    }

    // Keep holding runoffs until winner exists
    while (true)
    {
        // Calculate votes given remaining candidates
        tabulate();

        // Check if election has been won
        bool won = print_winner();
        if (won)
        {
            break;
        }

        // Eliminate last-place candidates
        int min = find_min();
        bool tie = is_tie(min);

        // If tie, everyone wins
        if (tie)
        {
            printf("Tie! \n");
            for (int i = 0; i < candidate_count; i++)
            {
                if (!candidates[i].eliminated)
                {

                    printf("%s\n", candidates[i].name);
                }
            }
            break;
        }

        // Eliminate anyone with minimum number of votes
        eliminate(min);

        // Reset vote counts back to zero
        for (int i = 0; i < candidate_count; i++)
        {
            candidates[i].votes = 0;
        }
    }
    return 0;
}

// Record preference if vote is valid
bool vote(int voter, int rank, string name)
{
    // TODO
    bool found = false;
    int index = 0;
    for(int i = 0; i < candidate_count; i++)
    {
        if (strcmp(candidates[i].name, name) == 0)
        {
            found = true;
            index = i;
            break;
        }
    }
    if(found)
    {
        preferences[voter][rank] = index;
        return true;

    }
    else
    {
        return false;
    }


}

// Tabulate votes for non-eliminated candidates
void tabulate(void)
{
    // TODO
    for(int i = 0; i < voter_count; i++)
    {
        for (int j = 0; j < candidate_count; j++)
        {
            int most_ranked_candidate_index = preferences[i][j];
            if(candidates[most_ranked_candidate_index].eliminated == false)
            {
                candidates[most_ranked_candidate_index].votes++;
                break;
            }
        }
    }

    for(int k = 0; k < candidate_count; k++)
    {
        printf("%s - %i\n", candidates[k].name, candidates[k].votes);
    }

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

candidate sorted_candidates[MAX_CANDIDATES];

// Print the winner of the election, if there is one
bool print_winner(void)
{

    for (int i = 0; i < candidate_count; i++)
    {
        sorted_candidates[i] = candidates[i];
    }
    bubble_sort(sorted_candidates, candidate_count);
    float majority = (float) sorted_candidates[candidate_count - 1].votes / (float) voter_count;
    if(majority > 0.5)
    {
        printf("Winner: \n");
        printf("%s\n", sorted_candidates[candidate_count - 1].name);
        return true;
    }
    // TODO
    return false;
}

// Return the minimum number of votes any remaining candidate has
int find_min(void)
{
    // TODO
    for (int i = 0; i < candidate_count; i++)
    {
        if(sorted_candidates[i].eliminated == false)
        {
            return sorted_candidates[i].votes;

        }

    }
    return 0;
}

// Return true if the election is tied between all candidates, false otherwise
bool is_tie(int min)
{
    // TODO
    for (int i = 0; i < candidate_count; i++)
    {
     if(candidates[i].votes != min)
     {
        return false;
     }

    }
    return true;
}

// Eliminate the candidate (or candidates) in last place
void eliminate(int min)
{
    // TODO
    for (int i = 0; i < candidate_count; i++)
    {
     if(candidates[i].votes == min)
     {
        candidates[i].eliminated = true;
     }

    }

}
