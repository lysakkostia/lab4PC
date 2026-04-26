#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include "protocol.h"

using namespace std;

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

    ConfigPayload config;
    config.matrix_size = 0;
    config.num_threads = 0;

    vector<int> matrix_A, matrix_B, matrix_C;
    atomic<StatusType> current_status{ STATUS_OK };

  while ( true )
    {
        MessageHeader header;
        if ( !recv_all( client_socket, ( char* )&header, sizeof( header ) ) ) break;

        header.command = ntohl( header.command );
        header.data_length = ntohl( header.data_length );

        if ( header.command == CMD_SEND_CONFIG )
        {
            recv_all( client_socket, ( char* )&config, sizeof( config ) );
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
            recv_all( client_socket, ( char* )matrix_A.data(), header.data_length );
            for ( int& val : matrix_A ) val = ntohl( val );
            cout << "Matrix A received and converted" << endl;
        }
        else if ( header.command == CMD_SEND_DATA_B )
        {
            recv_all( client_socket, ( char* )matrix_B.data(), header.data_length );
            for ( int& val : matrix_B ) val = ntohl( val );
            cout << "Matrix B received and converted" << endl;
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
            send_all( client_socket, ( char* )&s, sizeof( s ) );
        }
        else if ( header.command == CMD_GET_RESULT )
        {
            if ( current_status == STATUS_DONE )
            {
                vector<int> net_C = matrix_C;
                for ( int& val : net_C ) val = htonl( val );

                send_all( client_socket, ( char* )net_C.data(), net_C.size() * sizeof( int ) );
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

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons( 8080 );
    addr.sin_addr.s_addr = INADDR_ANY;

    bind( server_socket, ( sockaddr* )&addr, sizeof( addr ) );
    listen( server_socket, SOMAXCONN );

    cout << "Server running on port 8080..." << endl;

    while ( true )
    {
        SOCKET client = accept( server_socket, NULL, NULL );
        if ( client != INVALID_SOCKET )
        {
            thread( handle_client, client ).detach();
        }
    }

    closesocket( server_socket );
    WSACleanup();
    return 0;
}