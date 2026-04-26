#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <fstream>
#include <functional>

using namespace std;

void init_csv( const string& filename ) {
    ofstream file( filename );
    if ( file.is_open() )
    {
        file << "Size,Threads,Time_ms\n";
    }
}

void append_to_csv( const string& filename, int n, int threads, double time_ms ) {
    ofstream file( filename, ios::app );
    if ( file.is_open() )
    {
        file << n << "," << threads << "," << fixed << setprecision( 6 ) << time_ms << "\n";
    }
}

void print_result( const string& method, int n, long long time_us, double speedup = 1.0 ) {
    cout << left << setw( 18 ) << method
         << right << setw( 12 ) << to_string(n) + "x" + to_string(n)
         << setw( 15 ) << time_us
         << setw( 15 ) << fixed << setprecision( 2 ) << time_us / 1000.0
         << setw( 15 ) << fixed << setprecision( 2 ) << speedup << "x" << endl;
}

long long add_without_parallelism( const vector<vector<int>>& matrix_A, const vector<vector<int>>& matrix_B, vector<vector<int>>& matrix_C ) {
    int n = matrix_A.size();
    auto start = chrono::high_resolution_clock::now();

    for ( int i = 0; i < n; ++i )
    {
        for ( int j = 0; j < n; ++j )
        {
            matrix_C[i][j] = matrix_A[i][j] + matrix_B[i][j];
        }
    }

    auto end = chrono::high_resolution_clock::now();

    volatile int anti_optimization_sink = matrix_C[n-1][n-1];

    return chrono::duration_cast<chrono::microseconds>(end - start).count();
}

void process_chunk( const vector<vector<int>>& matrix_A, const vector<vector<int>>& matrix_B, vector<vector<int>>& matrix_C, int start_row, int end_row ) {
    int n = matrix_A.size();
    for ( int i = start_row; i < end_row; ++i )
    {
        for ( int j = 0; j < n; ++j )
        {
            matrix_C[i][j] = matrix_A[i][j] + matrix_B[i][j];
        }
    }
}

long long add_with_parallelism( const vector<vector<int>>& matrix_A, const vector<vector<int>>& matrix_B, vector<vector<int>>& matrix_C, int num_threads )
{
    int n = matrix_A.size();
    vector<thread> threads;

    auto start = chrono::high_resolution_clock::now();

    int chunk_size = n / num_threads;
    int remainder = n % num_threads;
    int current_start = 0;

    for ( int t = 0; t < num_threads; ++t )
    {
        int current_end = current_start + chunk_size + ( t < remainder ? 1 : 0 );

        if ( current_start < current_end ) {
            threads.emplace_back( process_chunk, cref( matrix_A ), cref( matrix_B ), ref( matrix_C ), current_start, current_end );
        }
        current_start = current_end;
    }

    for ( auto& th : threads )
    {
        th.join();
    }

    auto end = chrono::high_resolution_clock::now();

    volatile int anti_optimization_sink = matrix_C[n-1][n-1];

    return chrono::duration_cast<chrono::microseconds>(end - start).count();
}

int main() {

    init_csv( "results.csv" );

    vector<int> sizes = { 10, 100, 500, 1000, 2000, 5000, 10000, 20000, 40000 };
    vector<int> thread_counts = { 2, 4, 8, 16, 32, 64, 128, 256, 512 };

    for ( int n : sizes )
    {
        cout << string( 75, '=' ) << endl;
        cout << left << setw( 18 ) << "Method / Threads"
             << right << setw( 10 ) << "Size"
             << setw( 20 ) << "Time (mcs)"
             << setw( 15 ) << "Time (mns)"
             << setw( 15 ) << "SpeedUp" << endl;
        cout << string( 75, '-' ) << endl;

        random_device rd;
        mt19937 gen( rd() );
        uniform_int_distribution<> dis( 1, 1000 );
        
        vector<vector<int>> matrix_A( n, vector<int>(n) );
        vector<vector<int>> matrix_B( n, vector<int>(n) );
        for ( int i = 0; i < n; ++i )
        {
            for ( int j = 0; j < n; ++j )
            {
                matrix_A[i][j] = dis( gen );
                matrix_B[i][j] = dis( gen );
            }
        }

        vector<vector<int>> matrix_C_seq( n, vector<int>(n) );
        long long time_seq = add_without_parallelism( matrix_A, matrix_B, matrix_C_seq );
        append_to_csv( "results.csv", n, 1, time_seq / 1000.0 );
        print_result( "Sequential (1)", n, time_seq, 1.0 );
        cout << string( 75, '-' ) << endl;


        for ( int threads : thread_counts ) {
            vector<vector<int>> matrix_C_par( n, vector<int>(n) );
            long long time_par = add_with_parallelism( matrix_A, matrix_B, matrix_C_par, threads );

            append_to_csv( "results.csv", n, threads, time_par / 1000.0 );

            double speedup = static_cast<double>( time_seq ) / time_par;

            print_result( "Parallel (" + to_string( threads ) + ")", n, time_par, speedup );
        }
        cout << endl;
    }

    return 0;
}