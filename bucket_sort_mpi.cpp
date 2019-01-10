#include <mpi.h>

#include "bucket_sort_includes.hpp"

using namespace std;

// Constants(ish)
const long N = 1000 * 32;
int RANK_SIZE;
int MY_RANK;
#define BLOCK_SIZE (N/RANK_SIZE)

// Global vars
vector<long> section;
vector<long> sorted;

// Functions
void init_array();
int find_bucket(long val);
void find_displacements(long* section, int* bucket_displacements);
void check_arrays();

int main(void){
    MPI_Init(NULL, NULL);
    // Find Rank and size
    MPI_Comm_rank(MPI_COMM_WORLD, &MY_RANK);
    MPI_Comm_size(MPI_COMM_WORLD, &RANK_SIZE);
    init_array();

    // Sort local section and find divisions
    sort(section.begin(), section.end());
    vector<int> bucket_displacements(RANK_SIZE);
    find_displacements(&section[0], &bucket_displacements[0]);
    vector<int> counts;
    for(int i = 0; i < RANK_SIZE - 1; i++){
        counts.push_back(bucket_displacements[i + 1] - bucket_displacements[i]);
    }
    counts.push_back(section.size() - bucket_displacements[RANK_SIZE - 1]);

    // Send and receive buckets
    int* recv_counts = new int[RANK_SIZE];
    MPI_Alltoall(&counts[0], 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);
    int* recv_displacements = new int[RANK_SIZE];
    recv_displacements[0] = 0;
    for(int i = 1; i < RANK_SIZE; i++){
        recv_displacements[i] = recv_counts[i] + recv_displacements[i - 1];
    }
    sorted.resize(recv_displacements[RANK_SIZE - 1] + recv_displacements[RANK_SIZE - 2]);
    MPI_Alltoallv(&section[0], &counts[0], &bucket_displacements[0], MPI_LONG, &sorted[0], recv_counts, recv_displacements, MPI_LONG, MPI_COMM_WORLD);

    // Complete sort
    sort(sorted.begin(), sorted.end());

    // Check array
    check_arrays();

    // Clean up
    MPI_Finalize();
}

void init_array(){
    // Initialize the array
    section.reserve(BLOCK_SIZE);
    default_random_engine generator;
    uniform_int_distribution<long> distribution(numeric_limits<long>::lowest(), numeric_limits<long>::max());
    for(long i = 0; i < BLOCK_SIZE; i++){
        section.push_back(distribution(generator));
    }
}

void find_displacements(long* section, int* bucket_displacements){
    int curr_bucket = 0;
    for(long i = 0; i < BLOCK_SIZE; i++){
        int correct_bucket = find_bucket(section[i]);
        if(correct_bucket >= curr_bucket){
            bucket_displacements[correct_bucket] = i;
            curr_bucket = correct_bucket + 1;
        }
    }
}

long get_max_sorted(){
    return sorted.back();
}
long get_min_sorted(){
    return sorted.front();
}
void check_arrays(){
    long left_max = numeric_limits<long>::max();
    long right_min = numeric_limits<long>::lowest();
    long my_max = get_max_sorted();
    long my_min = get_min_sorted();
    int left_rank = MY_RANK == 0 ? MPI_PROC_NULL : MY_RANK - 1;
    int right_rank = MY_RANK == RANK_SIZE - 1 ? MPI_PROC_NULL : MY_RANK + 1;
    MPI_Status left_status, right_status;
    MPI_Sendrecv(&my_min, 1, MPI_LONG, left_rank, 0, &right_min, 1, MPI_LONG, right_rank, 0, MPI_COMM_WORLD, &left_status);
    MPI_Sendrecv(&my_max, 1, MPI_LONG, right_rank, 0, &left_max, 1, MPI_LONG, left_rank, 0, MPI_COMM_WORLD, &right_status);

    if(left_max >= my_min){
        cout << "Rank " << MY_RANK << " Sort Failed, " << left_max << " >= " << my_min << endl;
        return;
    }
    if(right_min <= my_max){
        cout << "Rank " << MY_RANK << " Sort Failed, " << right_min << "<=" << my_max << endl;
        return;
    }
}


int find_bucket(long val){
    long step = (numeric_limits<long>::max() / RANK_SIZE - numeric_limits<long>::lowest() / RANK_SIZE);
    for(long i = 0; i < RANK_SIZE; i++){
        if((val >= numeric_limits<long>::lowest() + (i * step)) && (val < numeric_limits<long>::lowest() + ((i+1) * step))){
            return (int)i;
        }
    }
    return RANK_SIZE - 1;
}
