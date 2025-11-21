#ifndef BUS_H
#define BUS_H

#define MAX_SEATS 40 
#define MAX_BUSES 10 
#define MAX_BOOKINGS 100
#define MAX_NAME_LEN 50

// Structure to hold bus information and dynamic seat status
struct Bus {
    int id;
    char name[MAX_NAME_LEN]; 
    char source[MAX_NAME_LEN];
    char destination[MAX_NAME_LEN];
    int price; // Includes price
    int total_seats;
    int seats_available; 
    int seats_status[MAX_SEATS]; // 0=Available, 1=Booked
};

// Structure to hold booking details
struct Booking {
    int ticket_id;
    char name[MAX_NAME_LEN];
    int bus_id;
    int seat_number;
    char source[MAX_NAME_LEN]; 
    char destination[MAX_NAME_LEN]; 
    int price; 
    int status; // 1=Booked, 0=Cancelled
};

// Extern declarations for global data
extern struct Bus buses[MAX_BUSES];
extern struct Booking bookings[MAX_BOOKINGS];
extern int next_bus_id;
extern int next_ticket_id;
extern int booking_count;

// Function prototypes
void initializeBuses();
int findBusIndex(int busId);
int addBus(char name[], char src[], char dest[], int total_seats);
void getBusDataJSON(char* buffer);
int bookSeat(int busId, int seatNumber, char name[]);
int cancelBooking(int ticketId);
void getAllBookingsJSON(char* buffer);
void getBookingByTicketJSON(char* buffer, int ticketId);
void saveBookings();
void loadBookings();

#endif // BUS_H
