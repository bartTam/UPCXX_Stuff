#include <mpi.h>

#include "bucket_sort_includes.hpp"

using namespace std;

// Constants(ish)
const long N = 1000 * 32;
#define WORLD (MPI::COMM_WORLD)
//int RANK_SIZE;
#define RANK_SIZE (MPI::COMM_WORLD.Get_rank())
//int MY_RANK;
#define MY_RANK (MPI::COMM_WORLD.Get_size())
#define BLOCK_SIZE (N/RANK_SIZE)

const long SORT_MAX = numeric_limits<long>::max();
const long SORT_MIN = numeric_limits<long>::lowest();

// Global vars
vector<long> section;
vector<long> sorted;

// Functions
void init_array();
int find_bucket(long val);
void find_displacements(long* section, int* bucket_displacements);
bool check_arrays();

int main(void){
    MPI::Init();
    // Find Rank and size
    //MPI_Comm_rank(MPI_COMM_WORLD, &MY_RANK);
    //MPI_Comm_size(MPI_COMM_WORLD, &RANK_SIZE);
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

    cout << "Send and receive buckets" << endl;
    // Send and receive buckets
    vector<int> recv_counts(RANK_SIZE);
    //int* recv_counts = new int[RANK_SIZE];
    WORLD.Alltoall(&counts[0], 1, MPI::INT, &recv_counts[0], 1, MPI::INT);
    vector<int> recv_displacements(RANK_SIZE);
    //int* recv_displacements = new int[RANK_SIZE];
    recv_displacements[0] = 0;
    long total_counts = recv_counts[0];
    for(int i = 1; i < RANK_SIZE; i++){
        recv_displacements[i] = recv_counts[i] + recv_displacements[i - 1];
        total_counts += recv_counts[i];
    }
    sorted.resize(total_counts);
    WORLD.Alltoallv(&section[0], &counts[0], &bucket_displacements[0], MPI::LONG, &sorted[0], &recv_counts[0], &recv_displacements[0], MPI::LONG);

    cout << "Complete sort" << endl;
    // Complete sort
    sort(sorted.begin(), sorted.end());

    // Check array
    int all_passed, my_passed = check_arrays();
    WORLD.Reduce(&my_passed, &all_passed, 1, MPI::BOOL, MPI::LAND, 0);
    if(MY_RANK == 0){
        if(all_passed){
            cout << "Sort completed correctly" << endl;
        } else {
            cout << "Sort did not complete correctly" << endl;
        }
    }

    // Clean up
    MPI::Finalize();
}

void init_array(){
    // Initialize the array
    section.reserve(BLOCK_SIZE);
    default_random_engine generator;
    uniform_int_distribution<long> distribution(SORT_MIN, SORT_MAX);
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
bool check_arrays(){
    long left_max = SORT_MIN;
    long right_min = SORT_MAX;
    long my_max = get_max_sorted();
    long my_min = get_min_sorted();
    int left_rank = MY_RANK == 0 ? MPI::PROC_NULL : MY_RANK - 1;
    int right_rank = MY_RANK == RANK_SIZE - 1 ? MPI::PROC_NULL : MY_RANK + 1;
    MPI::Status left_status, right_status;
    WORLD.Sendrecv(&my_min, 1, MPI::LONG, left_rank, 0, &right_min, 1, MPI::LONG, right_rank, 0, left_status);
    WORLD.Sendrecv(&my_max, 1, MPI::LONG, right_rank, 0, &left_max, 1, MPI::LONG, left_rank, 0, right_status);

    if(left_max >= my_min){
        cout << "Rank " << MY_RANK << " Sort Failed, " << left_max << " >= " << my_min << endl;
        return false;
    }
    if(right_min <= my_max){
        cout << "Rank " << MY_RANK << " Sort Failed, " << right_min << "<=" << my_max << endl;
        return false;
    }
    return true;
}


int find_bucket(long val){
    long step = (SORT_MAX / RANK_SIZE - SORT_MIN / RANK_SIZE);
    for(long i = 0; i < RANK_SIZE; i++){
        if((val >= SORT_MIN + (i * step)) && (val < SORT_MIN + ((i+1) * step))){
            return (int)i;
        }
    }
    return RANK_SIZE - 1;
}
