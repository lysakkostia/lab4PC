#include <iostream>
#include <vector>
#include <winsock2.h>
#include "protocol.h"

using namespace std;

void send_msg( SOCKET s, uint32_t cmd, void* data, uint32_t len ) {
    MessageHeader header;
    header.command = htonl( cmd );
    header.data_length = htonl( len );

    send( s, ( char* )&header, sizeof( header ), 0 );
    if ( len > 0 ) send( s, ( char* )data, len, 0 );
}

int main() {
    WSADATA wsa;
    WSAStartup( MAKEWORD( 2, 2 ), &wsa );

    SOCKET s = socket( AF_INET, SOCK_STREAM, 0 );
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons( 8080 );
    addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    if ( connect( s, ( sockaddr* )&addr, sizeof( addr ) ) == SOCKET_ERROR ) {
        cout << "Connection failed" << endl;
        return 1;
    }

    int n = 5;
    int threads = 4;

    ConfigPayload config;
    config.matrix_size = htonl( n );
    config.num_threads = htonl( threads );

    send_msg( s, CMD_SEND_CONFIG, &config, sizeof( config ) );

    vector<int> A( n * n, 1 ), B( n * n, 2 ), C( n * n, 0 );
    send_msg( s, CMD_SEND_DATA_A, A.data(), A.size() * sizeof( int ) );
    send_msg( s, CMD_SEND_DATA_B, B.data(), B.size() * sizeof( int ) );

    send_msg( s, CMD_START_TASK, NULL, 0 );

    while ( true ) {
        send_msg( s, CMD_GET_STATUS, NULL, 0 );
        uint32_t status;
        recv( s, ( char* )&status, sizeof( status ), 0 );
        status = ntohl( status );

        if ( status == STATUS_DONE ) break;
        cout << "Waiting for server..." << endl;
        Sleep( 500 );
    }

    send_msg( s, CMD_GET_RESULT, NULL, 0 );
    recv( s, ( char* )C.data(), C.size() * sizeof( int ), 0 );

    cout << "Result received. First element: " << C[0] << " (expected 3)" << endl;

    closesocket( s );
    WSACleanup();
    return 0;
}