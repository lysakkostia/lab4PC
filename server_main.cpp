#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include "protocol.h"

using namespace std;


void process_task( int n, int num_threads, const vector<int>& A,
                   const vector<int>& B, vector<int>& C,
                   atomic<StatusType>& status )
{
    status = STATUS_IN_PROGRESS;

    vector<thread> worker_threads;
    int chunk_size = ( n * n ) / num_threads;
    int remainder = ( n * n ) % num_threads;
    int current_start = 0;

    for ( int t = 0; t < num_threads; ++t )
    {
        int current_end = current_start + chunk_size + ( t < remainder ? 1 : 0 );
        worker_threads.emplace_back( [&]( int start, int end )
        {
            for ( int i = start; i < end; ++i )
            {
                C[i] = A[i] + B[i];
            }
        }, current_start, current_end );
        current_start = current_end;
    }

    for ( auto& th : worker_threads ) th.join();
    status = STATUS_DONE;
}

void handle_client( SOCKET client_socket )
{
    cout << "[Client Thread] Handling new connection" << endl;

    ConfigPayload config = { 0, 0 };
    vector<int> matrix_A, matrix_B, matrix_C;
    atomic<StatusType> current_status{ STATUS_OK };

    while ( true )
    {
        MessageHeader header;
        int bytes = recv( client_socket, ( char* )&header, sizeof( header ), 0 );
        if ( bytes <= 0 ) break;

        header.command = ntohl( header.command );
        header.data_length = ntohl( header.data_length );

        if ( header.command == CMD_SEND_CONFIG )
        {
            recv( client_socket, ( char* )&config, sizeof( config ), 0 );
            config.matrix_size = ntohl( config.matrix_size );
            config.num_threads = ntohl( config.num_threads );

            int total_elements = config.matrix_size * config.matrix_size;
            matrix_A.resize( total_elements );
            matrix_B.resize( total_elements );
            matrix_C.resize( total_elements );
            cout << "Config received: " << config.matrix_size << "x" << config.matrix_size << endl;
        }
        else if ( header.command == CMD_SEND_DATA_A )
            {
            recv( client_socket, ( char* )matrix_A.data(), header.data_length, 0 );
            cout << "Matrix A received" << endl;
        }
        else if ( header.command == CMD_SEND_DATA_B )
            {
            recv( client_socket, ( char* )matrix_B.data(), header.data_length, 0 );
            cout << "Matrix B received" << endl;
        }
        else if ( header.command == CMD_START_TASK )
        {
            thread( process_task, config.matrix_size, config.num_threads,
                ref( matrix_A ), ref( matrix_B ), ref( matrix_C ), ref( current_status ) ).detach();
            cout << "Processing started..." << endl;
        }
        else if ( header.command == CMD_GET_STATUS )
            {
            uint32_t s = htonl( ( uint32_t )current_status.load() );
            send( client_socket, ( char* )&s, sizeof( s ), 0 );
        }
        else if ( header.command == CMD_GET_RESULT )
        {
            if ( current_status == STATUS_DONE )
            {
                send( client_socket, ( char* )matrix_C.data(), matrix_C.size() * sizeof( int ), 0 );
                cout << "Result sent to client" << endl;
            }
        }
    }
    closesocket( client_socket );
}

int main() {
    WSADATA wsa;
    WSAStartup( MAKEWORD( 2, 2 ), &wsa );

    SOCKET server_socket = socket( AF_INET, SOCK_STREAM, 0 );
    sockaddr_in addr = { AF_INET, htons( 8080 ), INADDR_ANY };

    bind( server_socket, ( sockaddr* )&addr, sizeof( addr ) );
    listen( server_socket, SOMAXCONN );

    cout << "Server running on port 8080..." << endl;

    while ( true ) {
        SOCKET client = accept( server_socket, NULL, NULL );
        if ( client != INVALID_SOCKET ) {
            thread( handle_client, client ).detach();
        }
    }

    closesocket( server_socket );
    WSACleanup();
    return 0;
}