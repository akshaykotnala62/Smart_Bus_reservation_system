#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus.h"

// Initialize the global arrays and counters
struct Bus buses[MAX_BUSES];
struct Booking bookings[MAX_BOOKINGS];
int next_bus_id = 101; 
int next_ticket_id = 1;
int booking_count = 0; 

// --- Bus Management Helper Functions ---

#define INIT_BUS(id, busName, src, dest, price, seats) \
    {id, busName, src, dest, price, seats, seats, {0}}

int findBusIndex(int busId) {
    for (int i = 0; i < MAX_BUSES; i++) {
        if (buses[i].id == busId) {
            return i;
        }
    }
    return -1;
}

// --- CORE BUS LOGIC FUNCTIONS ---

void initializeBuses() {
    if (next_bus_id == 101) { 
        struct Bus initialBuses[] = {
            // Price is included here
            INIT_BUS(101, "Radhey Shyam Express", "Haridwar", "Delhi", 550, 40),
            INIT_BUS(102, "Shiv Ganga", "Haridwar", "Dehradun", 250, 30),
            INIT_BUS(103, "Ganga Darshan", "Haridwar", "Rishikesh", 100, 20),
            INIT_BUS(104, "Kumbh Yatra", "Haridwar", "Agra", 700, 35)
        };
        
        for (int i = 0; i < 4; i++) {
            buses[i] = initialBuses[i];
        }
        next_bus_id = 105;
    }
}

int addBus(char name[], char src[], char dest[], int total_seats) {
    if (total_seats <= 0 || total_seats > MAX_SEATS) {
        return -2; 
    }

    for (int i = 0; i < MAX_BUSES; i++) {
        if (buses[i].id == 0) { 
            buses[i].id = next_bus_id;
            strncpy(buses[i].name, name, MAX_NAME_LEN - 1); buses[i].name[MAX_NAME_LEN - 1] = '\0';
            strncpy(buses[i].source, src, MAX_NAME_LEN - 1); buses[i].source[MAX_NAME_LEN - 1] = '\0';
            strncpy(buses[i].destination, dest, MAX_NAME_LEN - 1); buses[i].destination[MAX_NAME_LEN - 1] = '\0';
            
            buses[i].price = 0; // Default price can be set here if not supplied
            buses[i].total_seats = total_seats;
            buses[i].seats_available = total_seats;
            
            memset(buses[i].seats_status, 0, sizeof(int) * MAX_SEATS);

            next_bus_id++;
            return buses[i].id;
        }
    }
    return -1; 
}

void getBusDataJSON(char* buffer) {
    char data[8192] = "[\r\n";
    int first = 1;

    for (int i = 0; i < MAX_BUSES; i++) {
        if (buses[i].id != 0) {
            if (!first) {
                strcat(data, ",\r\n");
            }
            
            char busJson[512];
            // Price is included in the JSON output
            sprintf(busJson,
                    "{\"id\": %d, \"name\": \"%s\", \"source\": \"%s\", \"destination\": \"%s\", \"price\": %d, \"total_seats\": %d, \"seats_available\": %d, \"seats_status\": [",
                    buses[i].id, buses[i].name, buses[i].source, buses[i].destination,
                    buses[i].price, 
                    buses[i].total_seats, buses[i].seats_available);

            strcat(data, busJson);

            // Add seats_status array
            for (int j = 0; j < buses[i].total_seats; j++) {
                char seatStatus[5];
                sprintf(seatStatus, "%d", buses[i].seats_status[j]);
                strcat(data, seatStatus);
                if (j < buses[i].total_seats - 1) {
                    strcat(data, ",");
                }
            }
            
            strcat(data, "]}\n");
            first = 0;
        }
    }
    strcat(data, "]");
    strcpy(buffer, data);
}

int bookSeat(int busId, int seatNumber, char name[]) {
    int busIndex = findBusIndex(busId);
    if (busIndex == -1) return -1; 

    if (seatNumber < 1 || seatNumber > buses[busIndex].total_seats) return -2; 
    
    if (buses[busIndex].seats_status[seatNumber - 1] == 1) return -3; 

    buses[busIndex].seats_status[seatNumber - 1] = 1;
    buses[busIndex].seats_available--;

    if (booking_count < MAX_BOOKINGS) {
        bookings[booking_count].ticket_id = next_ticket_id;
        strncpy(bookings[booking_count].name, name, MAX_NAME_LEN - 1); bookings[booking_count].name[MAX_NAME_LEN - 1] = '\0';
        bookings[booking_count].bus_id = busId;
        bookings[booking_count].seat_number = seatNumber;
        strncpy(bookings[booking_count].source, buses[busIndex].source, MAX_NAME_LEN - 1); bookings[booking_count].source[MAX_NAME_LEN - 1] = '\0';
        strncpy(bookings[booking_count].destination, buses[busIndex].destination, MAX_NAME_LEN - 1); bookings[booking_count].destination[MAX_NAME_LEN - 1] = '\0';
        bookings[booking_count].price = buses[busIndex].price;
        bookings[booking_count].status = 1; 
        booking_count++;
        return next_ticket_id++;
    }
    return -9; 
}

void saveBookings() {
    FILE *fp = fopen("bookings.txt", "w");
    if (fp == NULL) return;
    
    fprintf(fp, "%d %d %d\n", next_bus_id, next_ticket_id, booking_count);
    
    for (int i = 0; i < booking_count; i++) {
        fprintf(fp, "%d|%s|%d|%d|%s|%s|%d|%d\n", bookings[i].ticket_id, bookings[i].name, bookings[i].bus_id, 
                bookings[i].seat_number, bookings[i].source, bookings[i].destination, bookings[i].price, bookings[i].status);
    }
    fclose(fp);
}

void loadBookings() {
    FILE *fp = fopen("bookings.txt", "r");
    if (fp == NULL) {
        next_bus_id = 101; 
        next_ticket_id = 1; 
        return;
    }

    if (fscanf(fp, "%d %d %d\n", &next_bus_id, &next_ticket_id, &booking_count) != 3) {
        next_bus_id = 101; next_ticket_id = 1; booking_count = 0;
        fclose(fp); return;
    }
    
    char line[512];
    for (int i = 0; i < booking_count && i < MAX_BOOKINGS; i++) {
        if (fgets(line, sizeof(line), fp) != NULL) {
            sscanf(line, "%d|%[^|]|%d|%d|%[^|]|%[^|]|%d|%d", 
                   &bookings[i].ticket_id, bookings[i].name, &bookings[i].bus_id, 
                   &bookings[i].seat_number, bookings[i].source, bookings[i].destination, 
                   &bookings[i].price, &bookings[i].status);
            
            if (bookings[i].status == 1) {
                int bus_idx = findBusIndex(bookings[i].bus_id);
                if (bus_idx != -1 && bookings[i].seat_number >= 1 && bookings[i].seat_number <= buses[bus_idx].total_seats) {
                    buses[bus_idx].seats_status[bookings[i].seat_number - 1] = 1;
                    buses[bus_idx].seats_available--;
                }
            }
        }
    }
    fclose(fp);
}

int cancelBooking(int ticketId) {
    for(int i = 0; i < booking_count; i++) {
        if (bookings[i].ticket_id == ticketId && bookings[i].status == 1) {
            bookings[i].status = 0; 
            int busIndex = findBusIndex(bookings[i].bus_id);
            if (busIndex != -1) {
                buses[busIndex].seats_status[bookings[i].seat_number - 1] = 0;
                buses[busIndex].seats_available++;
            }
            return 1;
        }
    }
    return -1; 
}

void getAllBookingsJSON(char* buffer) { 
    char data[8192] = "[\r\n";
    int first = 1;

    for (int i = 0; i < booking_count; i++) {
        if (bookings[i].ticket_id != 0) {
            if (!first) {
                strcat(data, ",\r\n");
            }
            
            char bookingJson[512];
            // Uses escaped quotes \" for string values
            sprintf(bookingJson,
                    "{\"ticket_id\": %d, \"name\": \"%s\", \"bus_id\": %d, \"seat_number\": %d, \"source\": \"%s\", \"destination\": \"%s\", \"price\": %d, \"status\": \"%s\"}",
                    bookings[i].ticket_id, bookings[i].name, bookings[i].bus_id, 
                    bookings[i].seat_number, bookings[i].source, bookings[i].destination,
                    bookings[i].price, bookings[i].status == 1 ? "Booked" : "Cancelled");

            strcat(data, bookingJson);
            first = 0;
        }
    }
    strcat(data, "\r\n]");
    strcpy(buffer, data);
}

void getBookingByTicketJSON(char* buffer, int ticketId) {
    for (int i = 0; i < booking_count; i++) {
        if (bookings[i].ticket_id == ticketId) {
            // Uses escaped quotes \" for string values
            sprintf(buffer, 
                "{\"ticket_id\": %d, \"name\": \"%s\", \"bus_id\": %d, \"seat_number\": %d, \"source\": \"%s\", \"destination\": \"%s\", \"price\": %d, \"status\": \"%s\"}",
                bookings[i].ticket_id, bookings[i].name, bookings[i].bus_id, 
                bookings[i].seat_number, bookings[i].source, bookings[i].destination,
                bookings[i].price, bookings[i].status == 1 ? "Booked" : "Cancelled");
            return;
        }
    }
    strcpy(buffer, "{}");
}
