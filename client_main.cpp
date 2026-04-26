#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <winsock2.h>
#include "protocol.h"

using namespace std;

void init_csv( const string& filename )
{
    ofstream file( filename );
    if ( file.is_open() ) file << "Size,Threads,Time_ms\n";
}

void append_to_csv( const string& filename, int n, int threads, double time_ms )
{
    ofstream file( filename, ios::app );
    if ( file.is_open() ) file << n << "," << threads << "," << fixed << setprecision( 6 ) << time_ms << "\n";
}

void print_result( const string& method, int n, long long time_us, double speedup = 1.0 )
{
    cout << left << setw( 18 ) << method
         << right << setw( 12 ) << to_string(n) + "x" + to_string(n)
         << setw( 15 ) << time_us
         << setw( 15 ) << fixed << setprecision( 2 ) << time_us / 1000.0
         << setw( 15 ) << fixed << setprecision( 2 ) << speedup << "x" << endl;
}



bool send_all(SOCKET s, const char* buffer, int length)
{
    int total_sent = 0;
    while (total_sent < length)
    {
        int bytes = send(s, buffer + total_sent, length - total_sent, 0);
        if (bytes <= 0) return false;
        total_sent += bytes;
    }
    return true;
}

bool recv_all(SOCKET s, char* buffer, int length)
{
    int total_received = 0;
    while (total_received < length)
    {
        int bytes = recv(s, buffer + total_received, length - total_received, 0);
        if (bytes <= 0) return false;
        total_received += bytes;
    }
    return true;
}



void send_msg( SOCKET s, uint32_t cmd, void* data = nullptr, uint32_t len = 0 )
{
    MessageHeader header;
    header.command = htonl( cmd );
    header.data_length = htonl( len );
    send_all( s, ( const char* )&header, sizeof( header ) );
    if ( len > 0 && data != nullptr ) send_all( s, ( const char* )data, len );
}

long long run_remote_benchmark( int n, int threads, const vector<int>& A, const vector<int>& B, vector<int>& C )
{
    SOCKET s = socket( AF_INET, SOCK_STREAM, 0 );
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons( 8080 );
    addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    if ( connect( s, ( sockaddr* )&addr, sizeof( addr ) ) == SOCKET_ERROR )
    {
        cerr << "Connection failed for size " << n << endl;
        return -1;
    }

    ConfigPayload config;
    config.matrix_size = htonl( n );
    config.num_threads = htonl( threads );

    send_msg( s, CMD_SEND_CONFIG, &config, sizeof( config ) );
    send_msg( s, CMD_SEND_DATA_A, (void*)A.data(), A.size() * sizeof( int ) );
    send_msg( s, CMD_SEND_DATA_B, (void*)B.data(), B.size() * sizeof( int ) );

    auto start_time = chrono::high_resolution_clock::now();

    send_msg( s, CMD_START_TASK );

    while ( true )
    {
        send_msg( s, CMD_GET_STATUS );
        uint32_t status;
        recv( s, ( char* )&status, sizeof( status ), 0 );
        if ( ntohl( status ) == STATUS_DONE ) break;
        Sleep( 5 );
    }

    auto end_time = chrono::high_resolution_clock::now();

    send_msg( s, CMD_GET_RESULT );
    recv_all( s, ( char* )C.data(), C.size() * sizeof( int ) );

    closesocket( s );

    return chrono::duration_cast<chrono::microseconds>( end_time - start_time ).count();
}

int main() {
    WSADATA wsa;
    WSAStartup( MAKEWORD( 2, 2 ), &wsa );

    init_csv( "results.csv" );

    vector<int> sizes = { 10, 100, 500, 1000, 2000, 5000, 10000, 20000 };
    vector<int> thread_counts = { 2, 4, 8, 16, 32 };

    for ( int n : sizes ) {
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

        vector<int> matrix_A( n * n ), matrix_B( n * n ), matrix_C( n * n );
        for ( int i = 0; i < n * n; ++i )
        {
            matrix_A[i] = dis( gen );
            matrix_B[i] = dis( gen );
        }


        long long time_seq = run_remote_benchmark( n, 1, matrix_A, matrix_B, matrix_C );
        if(time_seq == -1) break;

        append_to_csv( "results.csv", n, 1, time_seq / 1000.0 );
        print_result( "Remote Seq (1)", n, time_seq, 1.0 );
        cout << string( 75, '-' ) << endl;


        for ( int threads : thread_counts )
        {
            long long time_par = run_remote_benchmark( n, threads, matrix_A, matrix_B, matrix_C );
            append_to_csv( "results.csv", n, threads, time_par / 1000.0 );

            double speedup = static_cast<double>( time_seq ) / time_par;
            print_result( "Remote Par (" + to_string( threads ) + ")", n, time_par, speedup );
        }
        cout << endl;
    }

    WSACleanup();
    return 0;
}