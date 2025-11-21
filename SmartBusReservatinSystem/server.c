#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "bus.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080 // Standardized Port 8080
#define BUFFER_SIZE 4096

// Function to send a full HTTP response with CORS headers
void sendHttpResponse(SOCKET client, const char* status, const char* content_type, const char* body) {
    char header[BUFFER_SIZE];
    int body_len = (int)strlen(body);
    
    // Proper HTTP 1.1 Response Structure with CORS
    sprintf(header, 
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n" // Includes Authorization header for full compatibility
            "\r\n", 
            status, content_type, body_len);

    send(client, header, (int)strlen(header), 0);
    send(client, body, body_len, 0);
}

// Simple utility to extract form-urlencoded value (Requires pre-allocated output buffer)
char* getFormValue(const char* body, const char* key, char* output, size_t max_len) {
    char search_key[100];
    sprintf(search_key, "%s=", key);
    
    const char* start = strstr(body, search_key);
    if (start) {
        start += strlen(search_key);
        const char* end = strchr(start, '&');
        size_t len = (end) ? (size_t)(end - start) : strlen(start);
        
        if (len >= max_len) len = max_len - 1;

        strncpy(output, start, len);
        output[len] = '\0';
        return output;
    }
    output[0] = '\0';
    return NULL;
}

// Handler for incoming API requests
void handleApiRequest(SOCKET client, const char* method, const char* path, const char* body) {
    char response_body[BUFFER_SIZE] = "";
    char content_type[50] = "application/json";
    const char* status = "200 OK";

    // Handle CORS pre-flight requests
    if (strcmp(method, "OPTIONS") == 0) {
        sendHttpResponse(client, "204 No Content", content_type, "");
        return;
    }

    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/api/buses") == 0) {
            getBusDataJSON(response_body);
        } else if (strcmp(path, "/api/bookings") == 0) {
            getAllBookingsJSON(response_body);
        } else if (strncmp(path, "/api/ticket/", 12) == 0) {
            int ticketId = atoi(path + 12);
            getBookingByTicketJSON(response_body, ticketId);
            if (strcmp(response_body, "{}") == 0) {
                status = "404 Not Found";
                strcpy(response_body, "{\"success\": false, \"message\": \"Ticket not found\"}");
            }
        } else {
            status = "404 Not Found";
            strcpy(response_body, "{\"success\": false, \"message\": \"API Endpoint not found\"}");
        }
    } 
    else if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/api/add_bus") == 0) {
            char name[MAX_NAME_LEN], src[MAX_NAME_LEN], dest[MAX_NAME_LEN], seats_str[10];
            int total_seats = 0;

            getFormValue(body, "busName", name, MAX_NAME_LEN);
            getFormValue(body, "from", src, MAX_NAME_LEN);
            getFormValue(body, "to", dest, MAX_NAME_LEN);
            getFormValue(body, "totalSeats", seats_str, 10);
            total_seats = atoi(seats_str);

            int new_id = addBus(name, src, dest, total_seats);

            if (new_id > 0) {
                sprintf(response_body, "{\"success\": true, \"message\": \"Bus added successfully\", \"id\": %d}", new_id);
            } else if (new_id == -2) {
                status = "400 Bad Request";
                strcpy(response_body, "{\"success\": false, \"message\": \"Invalid number of seats.\"}");
            } else {
                status = "500 Internal Server Error";
                strcpy(response_body, "{\"success\": false, \"message\": \"Max buses reached.\"}");
            }
        }
        else if (strcmp(path, "/api/book") == 0) {
            char name[MAX_NAME_LEN], busId_str[10], seatNum_str[10];
            int busId = 0, seatNumber = 0;

            getFormValue(body, "busId", busId_str, 10);
            getFormValue(body, "name", name, MAX_NAME_LEN);
            getFormValue(body, "seatNumber", seatNum_str, 10); 
            
            busId = atoi(busId_str);
            seatNumber = atoi(seatNum_str);

            int ticket_id = bookSeat(busId, seatNumber, name);
            if (ticket_id > 0) {
                saveBookings();
                sprintf(response_body, "{\"success\": true, \"message\": \"Booking successful\", \"ticket_id\": %d}", ticket_id);
            } else if (ticket_id == -1) {
                status = "404 Not Found";
                strcpy(response_body, "{\"success\": false, \"message\": \"Bus not found.\"}");
            } else if (ticket_id == -2) {
                status = "400 Bad Request";
                strcpy(response_body, "{\"success\": false, \"message\": \"Invalid seat number.\"}");
            } else if (ticket_id == -3) {
                status = "409 Conflict";
                strcpy(response_body, "{\"success\": false, \"message\": \"Seat is already booked.\"}");
            } else {
                status = "500 Internal Server Error";
                strcpy(response_body, "{\"success\": false, \"message\": \"Booking failed.\"}");
            }
        }
        else if (strcmp(path, "/api/cancel") == 0) {
            char ticketId_str[10];
            int ticketId = 0;

            getFormValue(body, "ticketId", ticketId_str, 10);
            ticketId = atoi(ticketId_str);

            if (cancelBooking(ticketId) == 1) {
                saveBookings();
                sprintf(response_body, "{\"success\": true, \"message\": \"Ticket T%d cancelled successfully.\", \"ticket_id\": %d}", ticketId, ticketId);
            } else {
                status = "404 Not Found";
                strcpy(response_body, "{\"success\": false, \"message\": \"Ticket not found or already cancelled.\"}");
            }
        }
        else {
            status = "404 Not Found";
            strcpy(response_body, "{\"success\": false, \"message\": \"API Endpoint not found\"}");
        }
    }
    else {
        status = "405 Method Not Allowed";
        strcpy(response_body, "{\"success\": false, \"message\": \"Method Not Allowed.\"}");
    }

    sendHttpResponse(client, status, content_type, response_body);
}


// Main server loop
void handleClient(SOCKET client) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        char method[10], path[256];
        char* body = strstr(buffer, "\r\n\r\n");
        if (body) {
            *body = '\0'; 
            body += 4;    
        }

        if (sscanf(buffer, "%s %s", method, path) == 2) {
            if (strncmp(path, "/api/", 5) == 0) {
                handleApiRequest(client, method, path, body ? body : "");
            } else {
                sendHttpResponse(client, "404 Not Found", "text/plain", "API calls only supported.");
            }
        } else {
            sendHttpResponse(client, "400 Bad Request", "text/plain", "Invalid Request.");
        }
    }

    closesocket(client);
}

int main() {
    loadBookings(); 
    initializeBuses(); 

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        return 1;
    }

    int optval = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
        printf("setsockopt failed with error code: %d\n", WSAGetLastError());
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(server, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed with error code: %d\n", WSAGetLastError());
        printf("HINT: Error 10048 means the port is already in use. Try closing the previous process.\n");
        closesocket(server);
        WSACleanup();
        return 1;
    }

    listen(server, 5);
    printf("Server running on http://localhost:%d\n", PORT);
    printf("Ready to handle API requests...\n");

    while (1) {
        SOCKET client;
        struct sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);

        client = accept(server, (struct sockaddr*)&clientAddr, &clientSize);
        if (client == INVALID_SOCKET) {
            printf("Accept failed with error code: %d\n", WSAGetLastError());
            continue;
        }

        handleClient(client);
    }

    closesocket(server);
    WSACleanup();
    return 0;
}
